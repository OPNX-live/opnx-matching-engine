//
// Created by Bob   on 2021/12/9.
//

#include <thread>
#include <unistd.h>
#include <time.h>
#include <fstream>

#include "manager.h"
#include "../pulsar_proxy/pulsar_proxy_c.h"
#include "log.h"
#include "utils.h"
#include "global.h"


//#define __ENABLED_TEST__

IMessage* g_pLogPulsar = nullptr;


Manager::~Manager()
{
    for (auto it = m_mapIMessage.begin(); m_mapIMessage.end() != it; it++)
    {
        IMessage* pIMessage = it->second;
        pIMessage->releaseIMessage();
    }
    m_mapIMessage.clear();
    for (auto it = m_mapEngine.begin(); m_mapEngine.end() != it; it++)
    {
        OPNX::IEngine* pIEngine = it->second;
        pIEngine->releaseIEngine();
    }
    m_mapEngine.clear();
    m_multimapTypeEngine.clear();
    for (auto it = m_mapITriggerOrderManager.begin(); m_mapITriggerOrderManager.end() != it; it++)
    {
        OPNX::ITriggerOrder* pITriggerOrder = it->second;
        pITriggerOrder->releaseITriggerOrder();
    }
    m_mapITriggerOrderManager.clear();
}

void Manager::run(bool enablePulsarLog)
{
    IMessage* pCmdPulsarProxy = nullptr;
    try {

        OPNX::Utils::getJsonValue<int>(m_iLogLevel, m_jsonConfig, "logLevel");
        cfLog.setLogLevel((OPNX::Log::Level)m_iLogLevel);
        OPNX::Utils::getJsonValue<int>(m_iOrderBookCycle, m_jsonConfig, "orderBookCycle");
        OPNX::Utils::getJsonValue<int>(m_iOrderBookDepth, m_jsonConfig, "orderBookDepth");
        OPNX::Utils::getJsonValue<int>(m_iImpliedDepth, m_jsonConfig, "impliedDepth");
        OPNX::Utils::getJsonValue<int>(m_iOrderOutThreadNumber, m_jsonConfig, "orderOutThreadNumber");
        OPNX::Utils::getJsonValue<int>(m_iOrdersOutThreadNumber, m_jsonConfig, "ordersOutThreadNumber");
        OPNX::Utils::getJsonValue<bool>(m_bEnableAuction, m_jsonConfig, "enableAuction");

        if (cfLog.enabledDebug()) {
            m_iSnapshotLogCycle = 100;  // Print a market snapshot every 10 seconds
        } else if (cfLog.enabledInfo()) {
            m_iSnapshotLogCycle = 1200;  // Print a market snapshot every 2 minutes
        } else {
            m_iSnapshotLogCycle = 3000;  // Print a market snapshot every 5 minutes
        }

        unsigned long long initTimestamp = OPNX::Utils::getTimestamp();
        unsigned long long sendMarketsTimestamp = initTimestamp;
        unsigned long long preTimestamp = initTimestamp;
#ifdef __ENABLED_TEST__
        std::thread testThread(Manager::handleThread, this, TEST);
#else
        pCmdPulsarProxy = createIMessage(nullptr, nullptr, nullptr, &m_cmdQueue, m_strPulsarServiceUrl);
        pCmdPulsarProxy->createCommander(m_strReferencePair);
        m_pCmdPulsarProxy = pCmdPulsarProxy;
        if (enablePulsarLog)
        {
            m_pLogPulsar = pCmdPulsarProxy;
            cfLog.info() << "---enablePulsarLog---" << std::endl;
        }

        sleep(5);   // sleep 5 second, send markets CMD

        nlohmann::json jsonMarkets;
        jsonMarkets["action"] = "markets";
        jsonMarkets["pair"] = m_strReferencePair;
        jsonMarkets["timestamp"] = OPNX::Utils::getTimestamp();
        pCmdPulsarProxy->sendCmd(jsonMarkets);
#endif

        std::vector<std::thread> threadPools;

        std::thread orderThread(Manager::handleThread, this, ORDER_IN);
        threadPools.push_back(std::move(orderThread));
        std::thread triggerOrderThread(Manager::handleThread, this, TRIGGER_ORDER_IN);
        threadPools.push_back(std::move(triggerOrderThread));
        std::thread orderBookThread(Manager::handleThread, this, ORDER_BOOK);
        threadPools.push_back(std::move(orderBookThread));
        std::thread orderBestBookThread(Manager::handleThread, this, BEST_BOOK);
        threadPools.push_back(std::move(orderBestBookThread));

        std::thread markPriceThread(Manager::handleThread, this, MARK_PRICE);
        threadPools.push_back(std::move(markPriceThread));
        std::thread lastPriceThread(Manager::handleThread, this, LAST_PRICE);
        threadPools.push_back(std::move(lastPriceThread));
        std::thread spreadSnapshotThread(Manager::handleThread, this, SNAPSHOT);
        threadPools.push_back(std::move(spreadSnapshotThread));

        for (int i = 0; i < m_iOrderOutThreadNumber; i++)
        {
            std::thread orderOutThread(Manager::handleThread, this, ORDER_OUT);
            threadPools.push_back(std::move(orderOutThread));
        }
        for (int i = 0; i < m_iOrdersOutThreadNumber; i++)
        {
            std::thread ordersOutThread(Manager::handleThread, this, ORDERS_OUT);
            threadPools.push_back(std::move(ordersOutThread));
        }

        std::thread pulsarLogThread(Manager::handleThread, this, PULSAR_LOG);
        threadPools.push_back(std::move(pulsarLogThread));

        bool bRecoveryEnd = false;
        bool bSendAuction = false;
        while (m_bThreadRunning)
        {
            try {
                nlohmann::json jsonCmd;
                if (m_cmdQueue.try_pop(jsonCmd))
                {
#ifndef __ENABLED_TEST__
                    handleCmd(pCmdPulsarProxy, jsonCmd);
#else
                    PulsarProxy *pCmdPulsarProxy = nullptr;
                    handleCmd(pCmdPulsarProxy, jsonCmd);
#endif
                }

                unsigned long long timestamp = OPNX::Utils::getTimestamp();
                if (!bRecoveryEnd)
                {
                    bool bAllIsRecovery = true;
                    for (auto it = m_mapIMessage.begin(); m_mapIMessage.end() != it; it++)
                    {
                        bAllIsRecovery = it->second->getRecovery();
                        if (bAllIsRecovery)
                        {
                            break;
                        }
                    }
                    if (!bAllIsRecovery)
                    {
                        bRecoveryEnd = true;
                        m_bRecoveryEnd = true;
                        sendMeStatus("READY");
                    }
                }
                if (5 < timestamp - preTimestamp)
                {
                    preTimestamp = timestamp;
                    heartbeat(bRecoveryEnd);
                }
#ifndef __ENABLED_TEST__
                if (m_mapEngine.empty() && nullptr != pCmdPulsarProxy)
                {
                    if (timestamp - sendMarketsTimestamp >= 300)
                    {
                        pCmdPulsarProxy->sendCmd(jsonMarkets);
                        sendMarketsTimestamp = timestamp;
                    }
                }
#endif

                time_t rawTime;
                struct tm * timeInfo;
                time (&rawTime);
                timeInfo = localtime (&rawTime);
                if (!bSendAuction && 0 == timeInfo->tm_min && m_bEnableAuction)  // Auction on the hour
                {
                    bSendAuction = true;
                    nlohmann::json jsonAuction;
                    jsonAuction["action"] = "auction";
                    jsonAuction["pair"] = m_strReferencePair;
                    jsonAuction["timestamp"] = OPNX::Utils::getTimestamp();
#ifndef __ENABLED_TEST__
                    pCmdPulsarProxy->sendCmd(jsonAuction);
#endif
                }
                else if (0 < timeInfo->tm_min)
                {
                    bSendAuction = false;
                }
            } catch (const std::exception &e) {
                cfLog.error() << "Manager run exception: " << e.what() << std::endl;
            } catch (...) {
                cfLog.fatal() << "Manager run exception!!!" << std::endl;
            }
            sleep(1);
        }

        cfLog.warn() << "Manager::run() m_bThreadRunning is false" << std::endl;
        for (auto &itemThread : threadPools)
        {
            itemThread.join();
        }
    } catch (...) {
        cfLog.fatal() << "Manager run exception!!!" << std::endl;
    }
    m_pCmdPulsarProxy = nullptr;
    if (nullptr != pCmdPulsarProxy)
    {
        pCmdPulsarProxy->releaseIMessage();
    }
    cfLog.warn() << "run() exit" << std::endl;
}

void Manager::exit()
{
    OPNX::CInOutLog cInOutLog("Manager::exit");

    m_bOBThreadRunning = false;  // order book thread exit

    sendMeStatus("NOT_READY");

    sleep(10);    // sleep 10 seconds for pulsar order consumer to process the order pushed by RE

    while (!m_triggerOrderQueue.empty())
    {
        usleep(1000);
    }
    cfLog.warn() << "m_triggerOrderQueue is empty" << std::endl;
    sleep(1);

    while (!m_orderQueue.empty())
    {
        usleep(1000);
    }
    cfLog.warn() << "m_orderQueue is empty" << std::endl;
    sleep(1);

    for (auto it = m_mapIMessage.begin(); m_mapIMessage.end() != it; it++)
    {
        IMessage* pIMessage = it->second;
        pIMessage->exitConsumer();
    }
    cfLog.warn() << "exitConsumer success" << std::endl;
    sleep(1);

    while (!m_orderOutQueue.empty())
    {
        usleep(1000);
    }
    cfLog.warn() << "m_orderOutQueue is empty" << std::endl;
    while (!m_ordersOutQueue.empty())
    {
        usleep(1000);
    }
    cfLog.warn() << "m_ordersOutQueue is empty" << std::endl;

    sleep(1);
    m_bThreadRunning = false;
    m_condition.notify_all();

    cfLog.warn() << "ME manager is exited, ReferencePair: " << m_strReferencePair << ", ServiceUrl: " << m_strPulsarServiceUrl << std::endl;
}

void Manager::sendMeStatus(const std::string& strStatus)
{
    if (nullptr != m_pCmdPulsarProxy)
    {
        nlohmann::json jsonQueryStatus;
        jsonQueryStatus["action"] = "queryStatus";
        jsonQueryStatus["pair"] = m_strReferencePair;
        jsonQueryStatus["data"] = strStatus;
        jsonQueryStatus["timestamp"] = OPNX::Utils::getTimestamp();
        m_pCmdPulsarProxy->sendCmd(jsonQueryStatus);
    }
}

void Manager::heartbeat(bool bRecoveryEnd)
{
    if (nullptr != m_pCmdPulsarProxy)
    {
        nlohmann::json jsonHeartbeat;
        jsonHeartbeat["action"] = "heartbeat";
        jsonHeartbeat["pair"] = m_strReferencePair;
        jsonHeartbeat["timestamp"] = OPNX::Utils::getTimestamp();
        nlohmann::json jsonData;

        if (bRecoveryEnd)
        {
            jsonData["status"] = "READY";
        }
        else
        {
            jsonData["status"] = "NOT_READY";
        }
        jsonData["orderQueueSize"] = m_orderQueue.size();
//        for (auto item : m_mapEngine)
//        {
//            auto pIEngine = item.second;
//            std::string strType = pIEngine->getType();
//            nlohmann::json jsonArray = nlohmann::json::array();
//            unsigned long long llOrderSize = pIEngine->getOrdersCount();
//            unsigned long long llAskSize = pIEngine->getAskOrderBookSize();
//            unsigned long long llBidSize = pIEngine->getBidOrderBookSize();
//            jsonArray.push_back(llOrderSize);
//            jsonArray.push_back(llAskSize);
//            jsonArray.push_back(llBidSize);
//            jsonData[strType] = jsonArray;
//        }
        jsonHeartbeat["data"] = jsonData;
        m_pCmdPulsarProxy->sendHeartbeat(jsonHeartbeat);
    }
}

void Manager::sendOrder(const OPNX::Order& order)
{
    try {
        std::stringstream ssOrder;
        std::string strJsonOrder = "";
        OPNX::Order::orderToJsonString(order, strJsonOrder);
        // im indicates whether it is a transaction order, and the default value is false
        // ii indicates whether the transaction is implied, and the default value is false
        ssOrder << "{\"pt\":\"Order\",\"im\":false,\"ii\":false,\"ol\":[" << strJsonOrder << "]}";
#ifdef __ENABLED_TEST__
//        cfLog.printInfo() << "ME send order: " << ssOrder.str() << std::endl;
        cfLog.printInfo() << "ME send order: " << order.orderId << " " << order.orderCreated << " " << order.timestamp << " " << OPNX::Utils::getMilliTimestamp() << " " << m_iOrderOutThreadNumber << std::endl;
        return;
#endif
        auto it = m_mapIMessage.find(order.marketId);
        if (m_mapIMessage.end() != it)
        {
            auto *pPulsarProxy = it->second;
            pPulsarProxy->sendOrder(ssOrder.str());
        }
    } catch (...) {
        cfLog.fatal() << "Manager::sendOrder exception!!!" << std::endl;
    }
}
void Manager::sendOrderList(const std::vector<OPNX::Order>& orders, unsigned long long ullSortId)
{
    try {
        auto size = orders.size();
        if (0 >= size) {
            cfLog.warn() << "Manager::sendOrderList orders is empty!!!" << std::endl;
            return;
        }
        std::stringstream ssOrder;
        std::string isImplied = "false";
        if (0 != orders[0].lastMatchedOrderId2)  // is third-party matching order
        {
            isImplied = "true";
        }
        // im indicates whether it is a transaction order. The default value is false
        // ii indicates whether the transaction is implied, and the default value is false
        ssOrder << "{\"pt\":\"Order\",\"im\":true,\"ii\":"<< isImplied << ",\"ol\":[";
        std::unordered_map<unsigned long long, long long > mapLastPrice;
        for (int i = 0; i < size; i++)
        {
            mapLastPrice.emplace(std::pair<unsigned long long, long long >(orders[i].marketId, orders[i].lastMatchPrice));
            std::string strJsonOrder = "";
            OPNX::Order::orderToJsonString(orders[i], strJsonOrder);
            ssOrder << strJsonOrder;
            if (i < size-1)
            {
                ssOrder << ",";
            }
        }
        ssOrder << "]}";

        m_lastPriceQueue.push(mapLastPrice);

#ifdef __ENABLED_TEST__
//        cfLog.printInfo() << "ME send orders: " << ssOrder.str() << std::endl;
        cfLog.printInfo() << "ME send orders: " << orders[0].orderId << " " << orders[0].orderCreated << " " << orders[0].timestamp << " " << OPNX::Utils::getMilliTimestamp() << " " << m_iOrdersOutThreadNumber << std::endl;
        return;
#endif

        while (true)
        {
            if (ullSortId == m_ullSendSortId + 1)
            {
                auto it = m_mapIMessage.find(orders[0].marketId);
                if (m_mapIMessage.end() != it)
                {
                    auto *pPulsarProxy = it->second;
                    pPulsarProxy->sendOrders(ssOrder.str());
                }
                m_ullSendSortId++;
                break;
            }
        }
    } catch (...) {
        cfLog.fatal() << "Manager::sendOrder exception!!!" << std::endl;
    }
}

void Manager::handleThread(Manager* pManager, ThreadType threadType)
{
    if (nullptr != pManager)
    {
        switch (threadType) {
            case ORDER_IN:
            {
                pManager->handleOrder();
                break;
            }
            case TRIGGER_ORDER_IN:
            {
                pManager->handleTriggerOrder();
                break;
            }
            case MARK_PRICE:
            {
                pManager->handleMarkPrice();
                break;
            }
            case LAST_PRICE:
            {
                pManager->handleLastPrice();
                break;
            }
            case ORDER_OUT:
            {
                pManager->handleOrderOut();
                break;
            }
            case ORDERS_OUT:
            {
                pManager->handleOrdersOut();
                break;
            }
            case ORDER_BOOK:
            {
                pManager->handleOrderBook();
                break;
            }
            case BEST_BOOK:
            {
                pManager->handleBestOrderBook();
                break;
            }
            case CMD:
            {
                break;
            }
            case TEST:
            {
                pManager->testHandle();
                break;
            }
            case PULSAR_LOG:
            {
                pManager->pulsarLogHandle();
                break;
            }
            case SNAPSHOT:
            {
                pManager->handleSpreadSnapshot();
                break;
            }
        }
    }
}

void Manager::handleOrder()
{
    try {
        cfLog.printInfo() << "------Manager::handleOrder is running ------" << std::endl;
        while (m_bThreadRunning)
        {
            try {
                if (!m_bEngineEnable)
                {
                    usleep(10);
                    continue;
                }
                OPNX::Order order;
                if (m_orderQueue.wait_and_pop(order))
                {
                    if (OPNX::Order::CANCEL == order.action && 0 == order.orderId)
                    {
                        if (0 != order.marketId)
                        {
                            auto it = m_mapEngine.find(order.marketId);
                            if (m_mapEngine.end() != it)
                            {
                                auto pIEngine = it->second;
                                cfLog.info() << pIEngine->getMarketCode() << " Begin handleOrder cancel all, unmapSearchOrder size: " << pIEngine->getOrdersCount() << " order.accountId:" << order.accountId << " order.marketId:" << order.marketId << std::endl;
                                pIEngine->handleOrder(order);
                                cfLog.info() << pIEngine->getMarketCode() << " End handleOrder cancel all, unmapSearchOrder size: " << pIEngine->getOrdersCount() << std::endl;

                            }
                        }
                        else if (0 != order.accountId)
                        {
                            for (auto item : m_mapEngine)
                            {
                                auto pIEngine = item.second;
                                cfLog.info() << pIEngine->getMarketCode() << " Begin handleOrder cancel all, unmapSearchOrder size: " << pIEngine->getOrdersCount() << " order.accountId:" << order.accountId << " order.marketId:" << order.marketId << std::endl;
                                pIEngine->handleOrder(order);
                                cfLog.info() << pIEngine->getMarketCode() << " End handleOrder cancel all, unmapSearchOrder size: " << pIEngine->getOrdersCount() << std::endl;

                            }
                        }
                        m_conditionCancelAll.notify_all();
                    }
                    else
                    {
                        auto it = m_mapEngine.find(order.marketId);
                        if (m_mapEngine.end() != it)
                        {
                            auto pIEngine = it->second;
                            cfLog.info() << pIEngine->getMarketCode() << " Begin handleOrder, Order Queue size: " << m_orderQueue.size() << " order.id:" << order.orderId << std::endl;
                            pIEngine->handleOrder(order);
                            cfLog.info() << pIEngine->getMarketCode() << " End   handleOrder, Order Queue size: " << m_orderQueue.size() << " order.id:" << order.orderId
                                         << ", unmapSearchOrder size: " << pIEngine->getOrdersCount() << ", asks size: " << pIEngine->getAskOrderBookSize() << ", bids size: " << pIEngine->getBidOrderBookSize() << std::endl;

                        }
                    }
                }

            } catch (...) {

            }
        }

        cfLog.printInfo() << "------Manager::handleOrder exit ------" << std::endl;
    } catch (...) {
        cfLog.fatal() << "Manager::handleOrder exception!!!" << std::endl;
    }
}

void Manager::handleTriggerOrder()
{
    try {
        cfLog.printInfo() << "------Manager::handleTriggerOrder is running ------" << std::endl;
        while (m_bThreadRunning)
        {
            try {
                if (!m_bEngineEnable)
                {
                    usleep(10);
                    continue;
                }
                OPNX::Order order;
                if (m_triggerOrderQueue.wait_and_pop(order))
                {
                    if (OPNX::Order::CANCEL == order.action && 0 == order.orderId)
                    {
                        if (0 != order.marketId)
                        {
                            auto it = m_mapITriggerOrderManager.find(order.marketId);
                            if (m_mapITriggerOrderManager.end() != it)
                            {
                                auto pITriggerOrderManager = it->second;
                                cfLog.info() << pITriggerOrderManager->getMarketCode() << " Begin handleTriggerOrder cancel all, unmapSearchOrder size: " << pITriggerOrderManager->getOrdersCount() << " order.accountId:" << order.accountId << " order.marketId:" << order.marketId << std::endl;
                                pITriggerOrderManager->handleOrder(order);
                                cfLog.info() << pITriggerOrderManager->getMarketCode() << " End handleTriggerOrder cancel all, unmapSearchOrder size: " << pITriggerOrderManager->getOrdersCount() << std::endl;

                            }
                        }
                        else if (0 != order.accountId)
                        {
                            for (auto item : m_mapITriggerOrderManager)
                            {
                                auto pITriggerOrderManager = item.second;
                                cfLog.info() << pITriggerOrderManager->getMarketCode() << " Begin handleTriggerOrder cancel all, unmapSearchOrder size: " << pITriggerOrderManager->getOrdersCount() << " order.accountId:" << order.accountId << " order.marketId:" << order.marketId << std::endl;
                                pITriggerOrderManager->handleOrder(order);
                                cfLog.info() << pITriggerOrderManager->getMarketCode() << " End handleTriggerOrder cancel all, unmapSearchOrder size: " << pITriggerOrderManager->getOrdersCount() << std::endl;

                            }
                        }
                        m_orderQueue.push(order);
                    }
                    else
                    {
                        auto it = m_mapITriggerOrderManager.find(order.marketId);
                        if (m_mapITriggerOrderManager.end() != it)
                        {
                            auto pITriggerOrderManager = it->second;
                            cfLog.info() << pITriggerOrderManager->getMarketCode() << " Begin handleTriggerOrder, TriggerOrder Queue size: " << m_triggerOrderQueue.size() << " order.id:" << order.orderId << ", unmapSearchOrder size: " << pITriggerOrderManager->getOrdersCount() << std::endl;
                            pITriggerOrderManager->handleOrder(order);
                            cfLog.info() << pITriggerOrderManager->getMarketCode() << " End   handleTriggerOrder, Order Queue size: " << m_triggerOrderQueue.size() << " order.id:" << order.orderId << ", unmapSearchOrder size: " << pITriggerOrderManager->getOrdersCount() << std::endl;

                        }
                    }
                }

            } catch (...) {

            }
        }

        cfLog.printInfo() << "------Manager::handleTriggerOrder exit ------" << std::endl;
    } catch (...) {
        cfLog.fatal() << "Manager::handleTriggerOrder exception!!!" << std::endl;
    }
}

void Manager::handleMarkPrice()
{
    try {
        cfLog.printInfo() << "------Manager::handleMarkPrice is running ------" << std::endl;
        while (m_bOBThreadRunning)
        {
            try {
                if (!m_bEngineEnable)
                {
                    usleep(10);
                    continue;
                }
                OPNX::TriggerPrice markPrice;
                if (m_markPriceQueue.wait_and_pop(markPrice))
                {
                    cfLog.debug() << "marketId:" << markPrice.marketId << ", markPrice:" << markPrice.price << std::endl;
                    auto it = m_mapITriggerOrderManager.find(markPrice.marketId);
                    if (m_mapITriggerOrderManager.end() != it) {
                        auto pITriggerOrder = it->second;
                        pITriggerOrder->markPriceTriggerOrder(markPrice.price);
                    }
                    if (markPrice.marketId == g_ullPerpMarketId)
                    {
                        g_llPerpMarkPrice = markPrice.price;
                    }
                }

            } catch (...) {

            }
        }

        cfLog.printInfo() << "------Manager::handleMarkPrice exit ------" << std::endl;
    } catch (...) {
        cfLog.fatal() << "anager::handleMarkPrice exception!!!" << std::endl;
    }
}
void Manager::handleLastPrice()
{
    try {
        cfLog.printInfo() << "------Manager::handleLastPrice is running ------" << std::endl;
        while (m_bOBThreadRunning)
        {
            try {
                if (!m_bEngineEnable)
                {
                    usleep(10);
                    continue;
                }
                std::unordered_map<unsigned long long, long long> mapLastPrice;
                if (m_lastPriceQueue.wait_and_pop(mapLastPrice))
                {
                    for (auto item: mapLastPrice)
                    {
                        auto pITriggerOrder = m_mapITriggerOrderManager.at(item.first);
                        pITriggerOrder->lastPriceTriggerOrder(item.second);
                    }
                }

            } catch (...) {

            }
        }

        cfLog.printInfo() << "------Manager::handleLastPrice exit ------" << std::endl;
    } catch (...) {
        cfLog.fatal() << "anager::handleLastPrice exception!!!" << std::endl;
    }
}
void Manager::handleOrderOut()
{
    try {
        cfLog.printInfo() << "------Manager::handleOrderOut is running ------" << std::endl;
        while (m_bThreadRunning)
        {
            try {
                if (!m_bEngineEnable)
                {
                    usleep(10);
                    continue;
                }
                OPNX::Order order;
                if (m_orderOutQueue.wait_and_pop(order))
                {
                    cfLog.debug() << "m_orderOutQueue size: " << m_orderOutQueue.size() << std::endl;
                    sendOrder(order);
                }
            } catch (...) {

            }
        }

        cfLog.printInfo() << "------Manager::handleOrderOut exit ------" << std::endl;
    } catch (...) {
        cfLog.fatal() << "Manager::handleOrderOut exception!!!" << std::endl;
    }
}
void Manager::handleOrdersOut()
{
    try {
        cfLog.printInfo() << "------Manager::handleOrdersOut is running ------" << std::endl;
        unsigned long long ullSortId = 0;
        while (m_bThreadRunning)
        {
            try {
                if (!m_bEngineEnable)
                {
                    usleep(10);
                    continue;
                }
                std::vector<OPNX::Order> orders;
                if (m_ordersOutQueue.wait_and_pop(orders))
                {
                    ullSortId = ++m_ullSortId;
                    cfLog.debug() << "m_ordersOutQueue size: " << m_ordersOutQueue.size() << std::endl;
                    sendOrderList(orders, ullSortId);
                }
            } catch (...) {

            }
        }

        cfLog.printInfo() << "------Manager::handleOrdersOut exit ------" << std::endl;
    } catch (...) {
        cfLog.fatal() << "Manager::handleOrdersOut exception!!!" << std::endl;
    }
}

void Manager::handleOrderBook()
{
    try {
        cfLog.printInfo() << "------Manager::handleOrderBook is running ------" << std::endl;

        std::atomic<unsigned long long> ullAtomicSeqNumber(static_cast<unsigned long long>(time(nullptr)) * 100000);
        std::map<unsigned long long, unsigned long long> mapUllPreSeqNumber;
        while (m_bOBThreadRunning)
        {
            try {
                if (!m_bEngineEnable || !m_bRecoveryEnd)
                {
                    usleep(10);
                    continue;
                }
                unsigned long long ullSequenceNumber = OPNX::Utils::getNumber(ullAtomicSeqNumber);
                for (auto it : m_mapEngine)
                {
                    auto pIEngine = it.second;
                    AskOrderBook askOrderBook;
                    BidOrderBook bidOrderBook;
                    OPNX::OrderBookItem bastSelfBid = pIEngine->getSelfBestBid();
                    pIEngine->getDisplayAskOrderBook(askOrderBook, m_iOrderBookDepth, m_iImpliedDepth, &bastSelfBid);
                    OPNX::OrderBookItem bestAskObItem;
                    if (!askOrderBook.empty())
                    {
                        bestAskObItem.price = askOrderBook.begin()->first;
                        bestAskObItem.quantity = askOrderBook.begin()->second;
                    }
                    pIEngine->getDisplayBidOrderBook(bidOrderBook, m_iOrderBookDepth, m_iImpliedDepth, &bestAskObItem);

                    std::string strMarketCode = pIEngine->getMarketCode();
                    unsigned long long ullMarketId = pIEngine->getMarketId();
                    unsigned long long ullFactor = pIEngine->getFactor();
                    unsigned long long ullQtyFactor = pIEngine->getQtyFactor();

                    if (mapUllPreSeqNumber.end() == mapUllPreSeqNumber.find(ullMarketId))
                    {
                        mapUllPreSeqNumber.emplace(std::pair<unsigned long long, unsigned long long>(ullMarketId, 0));
                    }

                    sendOrderBookSnapshot(ullMarketId, strMarketCode, ullFactor, ullQtyFactor, askOrderBook, bidOrderBook, ullSequenceNumber);
                    if (0 != mapUllPreSeqNumber[ullMarketId])
                    {
                        sendOrderBookDiff(ullMarketId, strMarketCode, ullFactor, ullQtyFactor, askOrderBook, bidOrderBook, mapUllPreSeqNumber[ullMarketId]);
                    }
                    mapUllPreSeqNumber[ullMarketId] = ullSequenceNumber;
//                    if (0 != mapUllPreSeqNumber[ullMarketId])
//                    {
//                        if (sendOrderBookDiff(ullMarketId, strMarketCode, ullFactor, askOrderBook, bidOrderBook, mapUllPreSeqNumber[ullMarketId]))
//                        {
//                            mapUllPreSeqNumber[ullMarketId] = ullSequenceNumber;
//                        }
//                    }
//                    else
//                    {
//                        mapUllPreSeqNumber[ullMarketId] = ullSequenceNumber;
//                    }

                    m_unmapAskOrderBook[ullMarketId] = askOrderBook;
                    m_unmapBidOrderBook[ullMarketId] = bidOrderBook;
                }
            } catch (...) {
                cfLog.fatal() << "Manager::handleOrderBook handle order book exception" << std::endl;
            }
#ifdef __ENABLED_TEST__
            usleep(10000000);
#else
            usleep(1000*m_iOrderBookCycle);
#endif
        }

        cfLog.printInfo() << "------Manager::handleOrderBook exitg ------" << std::endl;
    } catch (...) {
        cfLog.fatal() << "Manager::handleOrderBook exception!!!" << std::endl;
    }
}

void Manager::handleBestOrderBook()
{
    try {
        cfLog.printInfo() << "------Manager::handleBestOrderBook is running ------" << std::endl;

        std::mutex waitMutex;
        while (m_bOBThreadRunning)
        {
            try {
                if (!m_bEngineEnable || !m_bRecoveryEnd)
                {
                    usleep(10);
                    continue;
                }

//                std::unique_lock<std::mutex> lock(waitMutex);
//                m_condition.wait(lock);
                bool isBestChange = false;
                if (m_bestChangeQueue.wait_and_pop(isBestChange))
                {
                    sendOrderBookBest();
                }

            } catch (...) {
                cfLog.fatal() << "Manager::handleBestOrderBook handle order book exception" << std::endl;
            }
        }

        cfLog.printInfo() << "------Manager::handleBestOrderBook exit ------" << std::endl;
    } catch (...) {
        cfLog.fatal() << "Manager::handleBestOrderBook exception!!!" << std::endl;
    }
}

void Manager::handleSpreadSnapshot()
{
    /***
    1. midPrice = （bestBid + bestAsk）/ 2.0
    2. spread = abs(limitPrice - midPrice) / midPrice
    3. spread < m_dSpreadMax
    4. JSON package format for collecting data:
    {
        "marketCode":"ETH-USDT-SWAP-LIN",
        "spreadMax":0.1,
        "asks":[price, spread, [[account_id, qty],[account_id, qty]]],
        "bids":[price, spread, [account_id, qty],[account_id, qty]],
        "timestamp":1686537509
    }
     5. topic: persistent://OPNX-V1/ME-WS/SNAPSHOTS
    ***/
    try {
        cfLog.printInfo() << "------Manager::handleSpreadSnapshot is running ------" << std::endl;

        PulsarProxy* pulsarProxy = (PulsarProxy*)  m_pCmdPulsarProxy;
        pulsar_producer_t* pulsarProducer = pulsarProxy->createProducer(IMessage::SPREAD_SNAPSHOT, "persistent://OPNX-V1/ME-WS/SNAPSHOTS", "ME-WS-SNAPSHOT-" + m_strReferencePair);

        while (m_bOBThreadRunning)
        {
            try {
                if (!m_bEngineEnable || !m_bRecoveryEnd)
                {
                    usleep(10);
                    continue;
                }
                for (auto it : m_mapEngine)
                {
                    auto pIEngine = it.second;

                    std::string strMarketType = pIEngine->getType();
                    if ("REPO" == strMarketType)
                    {
                        continue;
                    }

                    std::string strMarketCode = pIEngine->getMarketCode();
                    unsigned long long ullFactor = pIEngine->getFactor();

                    unsigned long long llTimestamp = OPNX::Utils::getMilliTimestamp();

                    nlohmann::json jsonSnapshot;
                    nlohmann::json jsonAsks = nlohmann::json::array();
                    nlohmann::json jsonBids = nlohmann::json::array();

                    jsonSnapshot["marketCode"] = strMarketCode;
                    jsonSnapshot["spreadMax"] = m_dSpreadMax;
                    jsonSnapshot["timestamp"] = llTimestamp;

                    OPNX::OrderBookItem bastSelfBid = pIEngine->getSelfBestBid();
                    OPNX::OrderBookItem bastSelfAsk = pIEngine->getSelfBestAsk();
                    if ((0 != bastSelfBid.price && 0 != bastSelfBid.quantity)
                    && (0 != bastSelfAsk.price && 0 != bastSelfAsk.quantity))
                    {
                        double midPrice = (bastSelfBid.price + bastSelfAsk.price) / 2.0;

                        auto bidOrderBook = pIEngine->getBidOrderBook();
                        auto askOrderBook = pIEngine->getAskOrderBook();

                        auto itAsk = askOrderBook->begin();
                        while (askOrderBook->end() != itAsk)
                        {
                            long long limitPrice = itAsk->first;
                            double spread = (((double)limitPrice - midPrice) / midPrice) * 100;
//                            if (0 > spread) {
//                                cfLog.info() << "ask 1: " << spread << "=(abs(" << limitPrice << " - " << midPrice << ") / " << midPrice << ") * 100" << std::endl;
//                                cfLog.info() << "ask 1: " << (limitPrice - midPrice) << "  " << (((double)limitPrice - midPrice) / midPrice) << " " << ((((double)limitPrice - midPrice) / midPrice) * 100) << std::endl;
//                            }
                            spread = OPNX::Utils::doubleAccuracy(spread, 5);
                            if (spread < m_dSpreadMax)
                            {
                                auto SortOB = itAsk->second;
                                auto itOrder = SortOB.sortMapOrder.begin();

                                double price = static_cast<double>(limitPrice)/ullFactor;

                                nlohmann::json jsonAccountList = nlohmann::json::array();
                                while (SortOB.sortMapOrder.end() != itOrder)
                                {
                                    if (llTimestamp - itOrder->second->orderCreated > m_iOrderActiveTime)
                                    {
                                        nlohmann::json jsonAccount = nlohmann::json::array();
                                        unsigned long long accountId = static_cast<double>(itOrder->second->accountId);
                                        double quantity = static_cast<double>(itOrder->second->quantity)/ullFactor;
                                        jsonAccount.push_back(accountId);
                                        jsonAccount.push_back(quantity);
                                        jsonAccountList.push_back(jsonAccount);
                                    }
                                    itOrder++;
                                }
                                if (!jsonAccountList.empty())
                                {
                                    nlohmann::json jsonSpread = nlohmann::json::array();
                                    jsonSpread.push_back(price);
                                    jsonSpread.push_back(spread);
                                    jsonSpread.push_back(jsonAccountList);
                                    jsonAsks.push_back(jsonSpread);
                                }
                            }
                            itAsk++;
                        }
                        auto itBid = bidOrderBook->begin();
                        while (bidOrderBook->end() != itBid)
                        {
                            long long limitPrice = itBid->first;
                            double spread = ((midPrice - (double)limitPrice) / midPrice) * 100;
//                            if (0 > spread) {
//                                cfLog.info() << "bid 1: " << spread << "=(abs(" << midPrice << " - " << limitPrice << ") / " << midPrice << ") * 100" << std::endl;
//                                cfLog.info() << "bid 1: " << (midPrice - limitPrice) << " " << ((midPrice - (double)limitPrice) / midPrice) << " " << (((midPrice - (double)limitPrice) / midPrice) * 100) << std::endl;
//                            }
                            spread = OPNX::Utils::doubleAccuracy(spread, 5);
                            if (spread < m_dSpreadMax)
                            {
                                auto SortOB = itBid->second;
                                auto itOrder = SortOB.sortMapOrder.begin();

                                double price = static_cast<double>(limitPrice)/ullFactor;

                                nlohmann::json jsonAccountList = nlohmann::json::array();
                                while (SortOB.sortMapOrder.end() != itOrder)
                                {
                                    if (llTimestamp - itOrder->second->orderCreated > m_iOrderActiveTime)
                                    {
                                        nlohmann::json jsonAccount = nlohmann::json::array();
                                        unsigned long long accountId = static_cast<double>(itOrder->second->accountId);
                                        double quantity = static_cast<double>(itOrder->second->quantity)/ullFactor;
                                        jsonAccount.push_back(accountId);
                                        jsonAccount.push_back(quantity);
                                        jsonAccountList.push_back(jsonAccount);
                                    }
                                    itOrder++;
                                }
                                if (!jsonAccountList.empty())
                                {
                                    nlohmann::json jsonSpread = nlohmann::json::array();
                                    jsonSpread.push_back(price);
                                    jsonSpread.push_back(spread);
                                    jsonSpread.push_back(jsonAccountList);
                                    jsonBids.push_back(jsonSpread);
                                }
                            }
                            itBid++;
                        }

                    }
                    jsonSnapshot["asks"] = jsonAsks;
                    jsonSnapshot["bids"] = jsonBids;

                    std::string strJsonData = jsonSnapshot.dump();
                    pulsarProxy->sendMsg(pulsarProducer, strJsonData);

                    cfLog.info() << "SpreadSnapshot: " << strJsonData << std::endl;
                }
            } catch (...) {
                cfLog.fatal() << "Manager::handleSpreadSnapshot handle order book exception" << std::endl;
            }
            unsigned long long llSeconds = OPNX::Utils::getTimestamp();
            int iRemainder = llSeconds % m_iSpreadFrequency;
            int sleepTime = m_iSpreadFrequency - iRemainder + random() % m_iSpreadFrequency;
//            cfLog.info() << "sleepTime: " << sleepTime << std::endl;
            sleep(sleepTime);
        }

        if (nullptr != pulsarProducer)
        {
            pulsar_producer_close(pulsarProducer);
            pulsar_producer_free(pulsarProducer);
        }
        cfLog.printInfo() << "------Manager::handleSpreadSnapshot exitg ------" << std::endl;
    } catch (...) {
        cfLog.fatal() << "Manager::handleSpreadSnapshot exception!!!" << std::endl;
    }
}

void Manager::handleCmd(IMessage* pCmdIMessage, nlohmann::json jsonCmd)
{
    try {
        if (jsonCmd.contains("action"))
        {
            std::string strAction = jsonCmd["action"].get<std::string>();
            std::string strPair = "";
            OPNX::Utils::getJsonValue<std::string>(strPair, jsonCmd, "pair");
            std::string strPair2 = strPair;
            auto pos = strPair2.find("/");
            if (std::string::npos != pos)
            {
                strPair2.replace(pos, 1,  "-");
            }
            if (m_strReferencePair == strPair || m_strReferencePair == strPair2 || "" == strPair)
            {
                if ("markets" == strAction)
                {
                    handleMarketsInfo(jsonCmd["data"]);

                    if (!m_bSendRecovery && !m_mapEngine.empty())
                    {
                        nlohmann::json jsonMarkets;
                        jsonMarkets["action"] = "recovery";
                        jsonMarkets["pair"] = m_strReferencePair;
                        jsonMarkets["timestamp"] = OPNX::Utils::getTimestamp();
                        if (nullptr != pCmdIMessage)
                        {
                            pCmdIMessage->sendCmd(jsonMarkets);
                        }
                        m_bSendRecovery = true;
                    }
                }
                else if ("addMarkets" == strAction)
                {
                    addMarketsInfo(jsonCmd["data"]);
                }
                else if ("deleteMarkets" == strAction)
                {
                    deleteMarketsInfo(jsonCmd["data"]);
                }
                else if ("orderBookCycle" == strAction)
                {
                    OPNX::Utils::getJsonValue<int>(m_iOrderBookCycle, jsonCmd, "data");
                }
                else if ("orderBookDepth" == strAction)
                {
                    OPNX::Utils::getJsonValue<int>(m_iOrderBookDepth, jsonCmd, "data");
                }
                else if ("impliedDepth" == strAction)
                {
                    OPNX::Utils::getJsonValue<int>(m_iImpliedDepth, jsonCmd, "data");
                }
                else if ("logLevel" == strAction)
                {
                    OPNX::Utils::getJsonValue(m_iLogLevel, jsonCmd, "data");
                    cfLog.setLogLevel((OPNX::Log::Level)m_iLogLevel);

                    if (cfLog.enabledDebug()) {
                        m_iSnapshotLogCycle = 100;  // Print a market snapshot every 10 seconds
                    } else if (cfLog.enabledInfo()) {
                        m_iSnapshotLogCycle = 1200;  // Print a market snapshot every 2 minutes
                    } else {
                        m_iSnapshotLogCycle = 3000;  // Print a market snapshot every 5 minutes
                    }
                }
                else if ("addOrderConsumer" == strAction)
                {
                    int iCount = 1;
                    OPNX::Utils::getJsonValue(iCount, jsonCmd, "data");
                    for (auto msg: m_mapIMessage)
                    {
                        msg.second->addOrderConsumer(iCount);
                    }
                }
                else if ("exit" == strAction)
                {
                    exit();
                }
                else if (0 == strAction.compare("makerFees"))
                {
                    for (auto it = m_mapEngine.begin(); m_mapEngine.end() != it; it++)
                    {
                        OPNX::IEngine* pIEngine = it->second;
                        long long llMakerFees = 3;
                        OPNX::Utils::getJsonValue<long long>(llMakerFees, jsonCmd, "data");
                        pIEngine->setMakerFees(llMakerFees);
                    }
                }
                else if (0 == strAction.compare("clearOrder"))
                {
                    clearOrder();
                    nlohmann::json jsonMarkets;
                    jsonMarkets["action"] = "recovery";
                    jsonMarkets["pair"] = m_strReferencePair;
                    jsonMarkets["timestamp"] = OPNX::Utils::getTimestamp();
                    if (nullptr != pCmdIMessage)
                    {
                        pCmdIMessage->sendCmd(jsonMarkets);
                    }
                }
                else if (0 == strAction.compare("cancelAll"))
                {
                    if (jsonCmd.contains("data"))
                    {
                        OPNX::Order cancelAllOrder;
                        OPNX::Utils::getJsonValue<unsigned long long>(cancelAllOrder.accountId, jsonCmd["data"], "accountId");
                        OPNX::Utils::getJsonValue<unsigned long long>(cancelAllOrder.marketId, jsonCmd["data"], "marketId");
                        OPNX::Utils::getJsonValue<unsigned long long>(cancelAllOrder.clientOrderId, jsonCmd["data"], "clientOrderId");
                        cancelAllOrder.action = OPNX::Order::CANCEL;
                        if (!m_mapITriggerOrderManager.empty() && !m_mapEngine.empty())
                        {
                            m_triggerOrderQueue.push(cancelAllOrder);

                            std::mutex mtx;
                            std::unique_lock<std::mutex> lck(mtx);
                            m_conditionCancelAll.wait(lck);

                            if (nullptr != pCmdIMessage)
                            {
                                pCmdIMessage->sendCmd(jsonCmd);
                            }
                        }
                    }
                }
                else if ("queryStatus" == strAction)
                {
                    bool bAllIsRecovery = true;
                    for (auto it = m_mapIMessage.begin(); m_mapIMessage.end() != it; it++)
                    {
                        bAllIsRecovery = it->second->getRecovery();
                        if (bAllIsRecovery)
                        {
                            break;
                        }
                    }
                    std::string strStatus = "NOT_READY";
                    if (!bAllIsRecovery)
                    {
                        strStatus = "READY";
                    }
                    sendMeStatus(strStatus);
                }
                else if ("enableAuction" == strAction)
                {
                    OPNX::Utils::getJsonValue<bool>(m_bEnableAuction, jsonCmd, "data");
                }
                else if ("enablePulsarLog" == strAction)
                {
                    bool enablePulsarLog = true;
                    OPNX::Utils::getJsonValue<bool>(enablePulsarLog, jsonCmd, "data");
                    if (enablePulsarLog)
                    {
                        openPulsarLog();
                        cfLog.info() << "---enablePulsarLog---" << std::endl;
                    }
                    else
                    {
                        closedPulsarLog();
                        cfLog.info() << "---disablePulsarLog---" << std::endl;
                    }
                }
                else if ("disablePulsarLog" == strAction)
                {
                    closedPulsarLog();
                    cfLog.info() << "---disablePulsarLog---" << std::endl;
                }
                else if ("orderGroupCount" == strAction)
                {
                    int iOrderGroupCount = OPNX::ORDER_COUNT;
                    OPNX::Utils::getJsonValue<int>(iOrderGroupCount, jsonCmd, "data");
                    for (auto engine: m_mapEngine)
                    {
                        engine.second->setOrderGroupCount(iOrderGroupCount);
                    }
                    cfLog.info() << "---orderGroupCount---" << iOrderGroupCount << std::endl;
                }
                else if ("spreadSnapshot" == strAction)
                {
                    OPNX::Utils::getJsonValue<double>(m_dSpreadMax, jsonCmd["data"], "spreadMax");
                    OPNX::Utils::getJsonValue<int>(m_iOrderActiveTime, jsonCmd["data"], "orderActiveTime");
                    OPNX::Utils::getJsonValue<int>(m_iSpreadFrequency, jsonCmd["data"], "frequency");
                    cfLog.info() << "---spreadSnapshot---" << m_dSpreadMax << " " << m_iOrderActiveTime << " " << m_iSpreadFrequency << std::endl;
                }
            }
        }
        else
        {
            cfLog.error() << "unknown cmd: " << jsonCmd.dump() << std::endl;
        }
    } catch (...) {
        cfLog.fatal() << "Manager::handleCmd exception!!!" << std::endl;
    }
}

void Manager::testHandle()
{
    try {
        nlohmann::json  jsonTestData;
        std::string strTestFile("./test_data.json");
        std::ifstream ifstTest(strTestFile);
        ifstTest >> jsonTestData;
        ifstTest.close();

        std::atomic<unsigned long long> ullNumber(static_cast<unsigned long long>(100000));
        nlohmann::json jsonMarkets = jsonTestData["markets"];
        m_cmdQueue.push(jsonMarkets);
        bool isRandomOrder = true;
        while (m_bThreadRunning)
        {
//            try {
//                nlohmann::json  jsonTestOrder;
//                std::string strFile("./test_orders.json");
//                std::fstream fst(strFile);
//                fst >> jsonTestOrder;
//                bool isHandled = false;
//                OPNX::Utils::getJsonValue<bool>(isHandled, jsonTestOrder, "isHandled");
//                if (!isHandled)
//                {
//                    nlohmann::json jsonOrders = jsonTestOrder["orders"];
//                    for (int i = 0; i < jsonOrders.size(); i++)
//                    {
//                        nlohmann::json jsonOrder = jsonOrders.at(i);
//                        if (!jsonOrder.contains("id"))
//                        {
//                            jsonOrder["id"] = OPNX::Utils::getNumber(ullNumber);
//                        }
//                        OPNX::Order order;
//                        OPNX::Order::jsonToOrder(jsonOrder, order);
//                        m_orderQueue.push(order);
//                    }
//                    jsonTestOrder["isHandled"] = true;
//
//                    fst.close();
//                    fst.open("./test_orders.json", std::ios::out | std::ios::trunc);
//                    fst << jsonTestOrder.dump(4);
//                }
//                fst.close();
//            } catch (...) {
//                cfLog.fatal() << "Manager::testHandle data handle exception!!!" << std::endl;
//            }
            if (isRandomOrder)
            {
                sleep(30);
                testRandomOrder();
                isRandomOrder = false;
            }
            sleep(1);
        }
    } catch (...) {
        cfLog.fatal() << "Manager::testHandle exception!!!" << std::endl;
    }
}
void Manager::testRandomOrder()
{
    nlohmann::json  jsonOrderTemplate;
    std::string strTestFile("./test_orders_template.json");
    std::ifstream ifstTest(strTestFile);
    ifstTest >> jsonOrderTemplate;
    ifstTest.close();

    std::unordered_map<std::string, OPNX::Order> mapOrderTemplate;

    for (auto it: m_mapEngine)
    {
        auto pIEngine = it.second;

        std::string strType = pIEngine->getType();

        OPNX::Order order;

        if (jsonOrderTemplate.contains(strType))
        {
            OPNX::Order::jsonToOrder(jsonOrderTemplate[strType], order);
            mapOrderTemplate.emplace(std::pair<std::string, OPNX::Order>(strType, order));
        }
    }

    unsigned long long ullId = 0;
    unsigned long long ullTime = OPNX::Utils::getMilliTimestamp();
    for (int i = 0; i < 10000000; i++)
    {
        for (auto it: m_mapEngine)
        {
            auto pIEngine = it.second;
            std::string strType = pIEngine->getType();

            long lRandom = random();

            auto item = mapOrderTemplate.find(strType);
            if (mapOrderTemplate.end() != item)
            {
                OPNX::Order order(item->second);

                order.marketId = pIEngine->getMarketId();
                order.orderId = ++ullId;
                order.orderCreated = ullTime;

                if ("REPO" == strType)
                {
                    order.price = (lRandom%10)*100;
                }
                else if ("SPREAD" == strType)
                {
                    order.price = (lRandom%200)*100000000;
                }
                else
                {
                    order.price += (lRandom%200)*100000000;
                }

                if (0 == lRandom%2)
                {
                    order.side = OPNX::Order::BUY;
                    if ("REPO" == strType)
                    {
                        order.price = -order.price;
                    }
                }
                else
                {
                    order.side = OPNX::Order::SELL;
                }
                if (0 == lRandom%10)
                {
                    int iRandom = lRandom%300;
                    if (0 < iRandom) {
                        order.quantity = order.quantity*iRandom;
                        order.displayQuantity = order.quantity;
                    }
                }

                m_orderQueue.push(order);

            }
        }
    }
}

void Manager::handleMarketsInfo(nlohmann::json jsonMarkets)
{
    OPNX::CInOutLog cInOutLog("handleMarketsInfo");
    try {
        m_bEngineEnable = false;   // disable engine

        //first run Recovery is ture
        bool bIsRecovery = true;
        if (!m_mapEngine.empty())   // not the first time
        {
            bIsRecovery = false;
        }

        addMarketsInfo(jsonMarkets, bIsRecovery);

        sleep(5);  // Wait for other handle to finish

        std::vector<unsigned long long> vecMarketId;
        for (int i = 0; i < jsonMarkets.size(); i++)
        {
            nlohmann::json jsonMarket = jsonMarkets.at(i);
            unsigned long long ullMarketId = 0;
            OPNX::Utils::getJsonValue<unsigned long long>(ullMarketId, jsonMarket, "marketId");
            vecMarketId.push_back(ullMarketId);
        }
        // remove unwanted engine
        if (m_mapEngine.size() > jsonMarkets.size())
        {
            nlohmann::json jsonDeleteMarkets = nlohmann::json::array();
            for (auto it: m_mapEngine)
            {
                if (vecMarketId.end() == std::find(vecMarketId.begin(), vecMarketId.end(), it.first))
                {
                    OPNX::IEngine* pIEngine = it.second;
                    nlohmann::json jsonMarket;
                    pIEngine->getMarketInfo(jsonMarket);
                    jsonDeleteMarkets.push_back(jsonMarket);
                }
            }
            deleteMarketsInfo(jsonDeleteMarkets);
        }

        m_bEngineEnable = true;   // enable engine
    } catch (...) {
        cfLog.fatal() << "Manager::handleMarketsInfo exception!!!" << std::endl;
    }
}

void Manager::addMarketsInfo(nlohmann::json jsonMarkets, bool bIsRecovery)
{
    try {
        std::vector<unsigned long long> vecMarketId;
        for (int i = 0; i < jsonMarkets.size(); i++)
        {
            nlohmann::json jsonMarket = jsonMarkets.at(i);
            unsigned long long ullMarketId = 0;
            OPNX::Utils::getJsonValue<unsigned long long>(ullMarketId, jsonMarket, "marketId");
            std::string strMarketCode = "";
            OPNX::Utils::getJsonValue<std::string>(strMarketCode, jsonMarket, "marketCode");

            auto engine = m_mapEngine.find(ullMarketId);
            if (m_mapEngine.end() == engine)
            {
                CallbackOrder  orderOut = [&](const OPNX::Order& order){OPNX::Order newOrder(order); m_orderOutQueue.push(newOrder);};
                CallbackOrder  orderToEngine = [&](const OPNX::Order& order){OPNX::Order newOrder(order); m_orderQueue.push(newOrder);};
                CallbackOrder  orderToTriggerManager = [&](const OPNX::Order& order){OPNX::Order newOrder(order); m_triggerOrderQueue.push(newOrder);};
                CallbackOrderList ordersOut = [&](const std::vector<OPNX::Order>& orders){
                    std::vector<OPNX::Order> newOrders(orders);
                    m_ordersOutQueue.push(newOrders);
//                    auto size = orders.size();
//                    if (OPNX::ORDER_COUNT + 10 > size) // Indicates that the order split has been done in the engine
//                    {
//                        m_ordersOutQueue.push(orders);
//                    }
//                    else
//                    {
//                        auto times = size / OPNX::ORDER_COUNT + 1;
//                        int iBegin = 0;
//                        int iEnd = 0;
//                        for (int i = 1; i <= times; i++)
//                        {
//                            iEnd = OPNX::ORDER_COUNT*i;
//                            if (size < iEnd)
//                            {
//                                iEnd = size;
//                            }
//                            std::vector<OPNX::Order> subOrders(orders.begin()+iBegin, orders.begin()+iEnd);
//                            m_ordersOutQueue.push(subOrders);
//                            iBegin = iEnd;
//                            if (iBegin >= size)
//                            {
//                                break;
//                            }
//                        }
//                    }
                };
                CallbackBestOrderBook callbackBestOrderBook = [&](){ bool isBestChange = true; m_bestChangeQueue.push(isBestChange); };
                OPNX::IEngine* pIEngine = createIEngine(jsonMarket, (OPNX::ICallbackManager*)this);
                m_mapEngine.insert(std::pair<unsigned long long, OPNX::IEngine*>(ullMarketId, pIEngine));
                OPNX::ITriggerOrder* pITriggerOrder = createTriggerOrder(jsonMarket, (OPNX::ICallbackManager*)this);
                m_mapITriggerOrderManager.insert(std::pair<unsigned long long, OPNX::ITriggerOrder*>(ullMarketId, pITriggerOrder));

                std::string strType = pIEngine->getType();
                m_multimapTypeEngine.insert(std::pair<std::string, OPNX::IEngine*>(strType, pIEngine));
                if ("PERP" == strType)
                {
                    g_ullPerpMarketId = ullMarketId;
                }
                else if ("REPO" == strType)
                {
                    g_ullRepoMarketId = ullMarketId;
                }

                OPNX::Utils::setNodeId(ullMarketId);

                cfLog.info() << "addMarketsInfo: " << strMarketCode << std::endl;
            }
            else
            {
                engine->second->setMarketInfo(jsonMarket);
                cfLog.info() << "addMarketsInfo update: " << strMarketCode << std::endl;
            }

#ifndef __ENABLED_TEST__
            auto message = m_mapIMessage.find(ullMarketId);
            if (m_mapIMessage.end() == message)
            {
                IMessage* pPulsarProxy = createIMessage(&m_orderQueue, &m_triggerOrderQueue, &m_markPriceQueue, nullptr, m_strPulsarServiceUrl);
                pPulsarProxy->setRecovery(bIsRecovery);
                pPulsarProxy->createMarketAll(jsonMarket);
                m_mapIMessage.insert(std::pair<unsigned long long, IMessage*>(ullMarketId, pPulsarProxy));
            }
            else
            {
                message->second->setMarketInfo(jsonMarket);
            }
#endif

            vecMarketId.push_back(ullMarketId);
        }

        for (auto engine: m_mapEngine)
        {
            engine.second->updateImpliedMakerFees();
        }

        impliedEngine();

    } catch (const std::exception &e) {
        cfLog.error() << "addMarketsInfo exception: " << e.what() << std::endl;
    } catch (...) {
        cfLog.fatal() << "Manager::addMarketsInfo exception!!!" << std::endl;
    }
}
void Manager::deleteMarketsInfo(nlohmann::json jsonMarkets)
{
    m_bEngineEnable = false;   // disable engine
    try {
        // remove unwanted engine
        if (0 < jsonMarkets.size()) {

            for (int i = 0; i < jsonMarkets.size(); i++) {
                nlohmann::json jsonMarket = jsonMarkets.at(i);
                unsigned long long ullMarketId = 0;
                OPNX::Utils::getJsonValue<unsigned long long>(ullMarketId, jsonMarket, "marketId");

                auto itPulsar = m_mapIMessage.find(ullMarketId);
                if (m_mapIMessage.end() != itPulsar) {
                    IMessage *pIMessage = itPulsar->second;
                    pIMessage->releaseIMessage();
                    m_mapIMessage.erase(itPulsar);
                }

                auto itEngine = m_mapEngine.find(ullMarketId);
                if (m_mapEngine.end() != itEngine) {
                    OPNX::IEngine *pIEngine = itEngine->second;
                    for (auto item: m_mapEngine) {
                        item.second->eraseImplier(pIEngine);
                    }
                    for (auto item = m_multimapTypeEngine.begin(); item != m_multimapTypeEngine.end(); item++) {
                        if (itEngine->second == item->second) {
                            m_multimapTypeEngine.erase(item);
                            break;
                        }
                    }
                    pIEngine->releaseIEngine();
                    m_mapEngine.erase(itEngine);
                }

                auto itTrigger = m_mapITriggerOrderManager.find(ullMarketId);
                if (m_mapITriggerOrderManager.end() != itTrigger) {
                    OPNX::ITriggerOrder *pITriggerOrder = itTrigger->second;
                    pITriggerOrder->releaseITriggerOrder();
                    m_mapITriggerOrderManager.erase(itTrigger);
                }
                std::string strMarketCode = "";
                OPNX::Utils::getJsonValue<std::string>(strMarketCode, jsonMarket, "marketCode");
                cfLog.info() << "deleteMarketsInfo: " << strMarketCode << std::endl;
            }
        }

    } catch (const std::exception &e) {
        cfLog.error() << "deleteMarketsInfo exception: " << e.what() << std::endl;
    } catch (...) {
        cfLog.fatal() << "Manager::deleteMarketsInfo exception!!!" << std::endl;
    }
    m_bEngineEnable = true;   // enable engine
}

void Manager::impliedEngine()
{
    try {
        // implied
        if (m_jsonConfig.contains("implied"))
        {
            nlohmann::json jsonImplied = m_jsonConfig["implied"];
            for (auto engine: m_mapEngine)
            {
                std::string strType = engine.second->getType();
                std::string strMarketCode = engine.second->getMarketCode();
                std::string strReferencePair = engine.second->getReferencePair();
                std::string strExpiryTime = engine.second->getExpiryTime();
                long long miniTick = engine.second->getMiniTick();
                unsigned long long ullFactor = engine.second->getFactor();


                if (jsonImplied.contains(strType))
                {
                    nlohmann::json arrayImplied = jsonImplied[strType];
                    for (auto arrayItem : arrayImplied)
                    {
                        std::string strLeg1 = "";
                        std::string strLeg2 = "";
                        OPNX::Utils::getJsonValue<std::string>(strLeg1, arrayItem, "leg1");
                        OPNX::Utils::getJsonValue<std::string>(strLeg2, arrayItem, "leg2");
                        nlohmann::json jsonExclude = arrayItem["exclude"];
                        int iType = arrayItem["type"].get<int>();
                        bool bIsExclude = false;
                        for (auto item: jsonExclude)
                        {
                            std::string strItem = item.get<std::string>();
                            if (strItem == strReferencePair)
                            {
                                bIsExclude = true;
                                break;
                            }
                        }
                        if (!bIsExclude)
                        {
                            std::vector<OPNX::IEngine*> vecLeg1Engine;
                            std::vector<OPNX::IEngine*> vecLeg2Engine;
                            for (auto it: m_multimapTypeEngine)
                            {
                                if (0 == strLeg1.compare(it.first))
                                {
                                    vecLeg1Engine.push_back(it.second);
                                }
                                if (0 == strLeg2.compare(it.first))
                                {
                                    vecLeg2Engine.push_back(it.second);
                                }
                            }
                            for (auto leg1: vecLeg1Engine)
                            {
                                for (auto leg2: vecLeg2Engine)
                                {
                                    std::string strExpiryTime1 = leg1->getExpiryTime();
                                    std::string strExpiryTime2 = leg2->getExpiryTime();
                                    if ("" != strExpiryTime && (strExpiryTime != strExpiryTime1 && strExpiryTime != strExpiryTime2))
                                    {
                                        continue;
                                    }
                                    if ("" != strExpiryTime1 && (strExpiryTime1 != strExpiryTime && strExpiryTime1 != strExpiryTime2))
                                    {
                                        continue;
                                    }
                                    if ("" != strExpiryTime2 && (strExpiryTime2 != strExpiryTime && strExpiryTime2 != strExpiryTime1))
                                    {
                                        continue;
                                    }
                                    if ("" == strExpiryTime && strExpiryTime1 != strExpiryTime2)
                                    {
                                        continue;
                                    }
                                    OPNX::Implier implier(leg1, leg2, (OPNX::Implier::ImpliedType)iType, miniTick, ullFactor);
                                    engine.second->setImplier(implier);
                                }
                            }
                        }
                    }
                }
            }
        }
    } catch (...) {
        cfLog.fatal() << "Manager::impliedEngine exception!!!" << std::endl;
    }
}

void Manager::clearOrder()
{
    try {

        for (auto it = m_mapIMessage.begin(); m_mapIMessage.end() != it; it++)
        {
            IMessage* pIMessage = it->second;
            pIMessage->setRecovery(true);
        }
        while (!m_orderQueue.empty())
        {
            usleep(1000);
        }
        while (!m_orderOutQueue.empty())
        {
            usleep(1000);
        }
        while (!m_ordersOutQueue.empty())
        {
            usleep(1000);
        }
        for (auto it : m_mapITriggerOrderManager) {
            auto pITriggerOrderManager = it.second;
            pITriggerOrderManager->clearOrder();
        }
        for (auto it : m_mapEngine) {
            auto pIEngine = it.second;
            pIEngine->clearOrder();
        }

    } catch (const std::exception &e) {
        cfLog.error() << "Manager::clearOrder exception: " << e.what() << std::endl;
    } catch (...) {
        cfLog.fatal() << "Manager::clearOrder exception!!!" << std::endl;
    }
}

inline bool Manager::checkOrder(OPNX::Order& order)
{
    bool checkResult = false;
    while (!checkResult)
    {
        if (OPNX::Order::AUCTION == order.timeCondition)  // don't check AUCTION order
        {
            checkResult = true;
            break;
        }
        if (0 == order.quantity && 0 == order.amount
        && (OPNX::Order::NEW == order.action || OPNX::Order::AMEND == order.action || OPNX::Order::RECOVERY == order.action))
        {
            order.status = OPNX::Order::REJECT_QUANTITY_AND_AMOUNT_ZERO;
            break;
        }
        if (0 < order.quantity && 0 < order.amount)
        {
            order.status = OPNX::Order::REJECT_QUANTITY_AND_AMOUNT_LARGER_ZERO;
            break;
        }
        if (OPNX::Order::STOP_LIMIT == order.type || OPNX::Order::STOP_MARKET == order.type)
        {
            if (OPNX::Order::NONE == order.stopCondition)
            {
                order.status = OPNX::Order::REJECT_STOP_CONDITION_IS_NONE;
                break;
            }
            if (OPNX::Order::MAX_PRICE == order.triggerPrice)
            {
                order.status = OPNX::Order::REJECT_STOP_TRIGGER_PRICE_IS_NONE;
                break;
            }
        }
        if (0 < order.quantity && order.quantity < order.displayQuantity)
        {
            order.status = OPNX::Order::REJECT_DISPLAY_QUANTITY_LARGER_THAN_QUANTITY;
            break;
        }

        if (0 >= order.displayQuantity && 0 == order.amount)
        {
            order.status = OPNX::Order::REJECT_DISPLAY_QUANTITY_ZERO;
            break;
        }
        if (OPNX::Order::LIMIT == order.type && OPNX::Order::MAX_PRICE == order.price)
        {
            order.status = OPNX::Order::REJECT_LIMIT_ORDER_WITH_MARKET_PRICE;
            break;
        }
        checkResult = true;
    }
    if (!checkResult)
    {
        sendOrder(order);
    }
    return checkResult;
}

void Manager::sendOrderBookSnapshot(unsigned long long ullMarketId, const std::string& strMarketCode, unsigned long long ullFactor, unsigned long long ullQtyFactor, AskOrderBook& askOrderBook, BidOrderBook& bidOrderBook, unsigned long long ullSequenceNumber)
{
    if (cfLog.enabledTrace())
    {
        OPNX::CInOutLog cInOutLog("sendOrderBookSnapshot");
    }
    try {
        nlohmann::json jsonOrderBook;
        nlohmann::json jsonAsks = nlohmann::json::array();
        nlohmann::json jsonBids = nlohmann::json::array();

//        jsonOrderBook["instrumentId"] = strMarketCode;
        jsonOrderBook["marketCode"] = strMarketCode;
        jsonOrderBook["seqNum"] = ullSequenceNumber;

        std::stringstream ssSnapshot;
        ssSnapshot << strMarketCode << " Snapshot ask:[";

        int iSize = 0;
        for (auto it = askOrderBook.begin(); askOrderBook.end() != it; it++)
        {
            if (iSize >= m_iOrderBookDepth)
            {
                askOrderBook.erase(it, askOrderBook.end());
                break;
            }
            double price = static_cast<double>(it->first)/ullFactor;
            double quantity = static_cast<double>(it->second)/ullQtyFactor;
//            std::vector<double> details {price, quantity, 0, 0};
            std::vector<double> details {price, quantity};
            if (iSize < 20)
            {
                ssSnapshot << "[" << price << "," << quantity << "]";
            }
            jsonAsks.push_back(std::move(details));
            iSize++;
        }
        ssSnapshot << "],bid:[";
        iSize = 0;
        for (auto it = bidOrderBook.begin(); bidOrderBook.end() != it; it++)
        {
            if (iSize >= m_iOrderBookDepth)
            {
                bidOrderBook.erase(it, bidOrderBook.end());
                break;
            }
            double price = static_cast<double>(it->first)/ullFactor;
            double quantity = static_cast<double>(it->second)/ullQtyFactor;
//            std::vector<double> details {price, quantity, 0, 0};
            std::vector<double> details {price, quantity};
            if (iSize < 20)
            {
                ssSnapshot << "[" << price << "," << quantity << "]";
            }
            jsonBids.push_back(std::move(details));
            iSize++;
        }
        unsigned long long ullTimestamp = OPNX::Utils::getMilliTimestamp();
        ssSnapshot << "],timestamp: " << ullTimestamp;
#ifdef __ENABLED_TEST__
        if (!jsonAsks.empty() || !jsonBids.empty())
        {
#endif
            std::string strData = jsonAsks.dump() + jsonBids.dump();
            unsigned int res32 = OPNX::Utils::crc32b((unsigned char*)strData.c_str());

            jsonOrderBook["asks"]          = std::move(jsonAsks);
            jsonOrderBook["bids"]          = std::move(jsonBids);
            jsonOrderBook["timestamp"]     = ullTimestamp;
            jsonOrderBook["checksum"]      = res32;
#ifndef __ENABLED_TEST__

            auto it = m_mapIMessage.find(ullMarketId);
        if (m_mapIMessage.end() != it)
        {
            auto *pPulsarProxy = it->second;
            pPulsarProxy->sendOrderBookSnapshot(jsonOrderBook);
        }
        if (0 == ullSequenceNumber%m_iSnapshotLogCycle) {
            cfLog.printInfo() << ssSnapshot.str() << std::endl;
        }

#else

            if (0 == ullSequenceNumber%m_iSnapshotLogCycle) {
                cfLog.printInfo() << ssSnapshot.str() << std::endl;
            }
//            cfLog.info() << "MD snapshot: " << jsonOrderBook.dump() << std::endl;
#endif
#ifdef __ENABLED_TEST__
        }
#endif

    } catch (...) {
        cfLog.fatal() << "Manager::creatOrderBookSnapshot exception!!!" << std::endl;
    }
}
bool Manager::sendOrderBookDiff(unsigned long long ullMarketId, const std::string& strMarketCode, unsigned long long ullFactor, unsigned long long ullQtyFactor, const AskOrderBook& askOrderBook, const BidOrderBook& bidOrderBook, unsigned long long ullSequenceNumber)
{
    if (cfLog.enabledTrace())
    {
        OPNX::CInOutLog cInOutLog("sendOrderBookDiff");
    }
    bool bRes = false;
    try {
        AskOrderBook askDiffOrderBook;
        BidOrderBook bidDiffOrderBook;

        for (auto it = askOrderBook.begin(); askOrderBook.end() != it; it++)
        {
            auto preIt = m_unmapAskOrderBook[ullMarketId].find(it->first);
            if (it->second != preIt->second)
            {
                askDiffOrderBook[it->first] = it->second;
            }
        }
        for (auto preIt = m_unmapAskOrderBook[ullMarketId].begin(); m_unmapAskOrderBook[ullMarketId].end() != preIt; preIt++)
        {
            auto it = askOrderBook.find(preIt->first);
            if (askOrderBook.end() == it)
            {
                askDiffOrderBook[preIt->first] = 0;
            }
        }
        for (auto it = bidOrderBook.begin(); bidOrderBook.end() != it; it++)
        {
            auto preIt = m_unmapBidOrderBook[ullMarketId].find(it->first);
            if (it->second != preIt->second)
            {
                bidDiffOrderBook[it->first] = it->second;
            }
        }
        for (auto preIt = m_unmapBidOrderBook[ullMarketId].begin(); m_unmapBidOrderBook[ullMarketId].end() != preIt; preIt++)
        {
            auto it = bidOrderBook.find(preIt->first);
            if (bidOrderBook.end() == it)
            {
                bidDiffOrderBook[preIt->first] = 0;
            }
        }

        nlohmann::json jsonOrderBook;
        nlohmann::json jsonAsks = nlohmann::json::array();
        nlohmann::json jsonBids = nlohmann::json::array();

        jsonOrderBook["marketCode"] = strMarketCode;
        jsonOrderBook["seqNum"] = ullSequenceNumber;

        for (auto it = askDiffOrderBook.begin(); askDiffOrderBook.end() != it; it++)
        {
            std::vector<double> details {static_cast<double>(it->first)/ullFactor, static_cast<double>(it->second)/ullQtyFactor};
            jsonAsks.push_back(std::move(details));
        }
        for (auto it = bidDiffOrderBook.begin(); bidDiffOrderBook.end() != it; it++)
        {
            std::vector<double> details {static_cast<double>(it->first)/ullFactor, static_cast<double>(it->second)/ullQtyFactor};
            jsonBids.push_back(std::move(details));
        }

//        if (!jsonAsks.empty() || !jsonBids.empty())
        {
            std::string strData = jsonAsks.dump() + jsonBids.dump();
            unsigned int res32 = OPNX::Utils::crc32b((unsigned char*)strData.c_str());
            jsonOrderBook["asks"]          = std::move(jsonAsks);
            jsonOrderBook["bids"]          = std::move(jsonBids);
            jsonOrderBook["timestamp"]     = OPNX::Utils::getMilliTimestamp();
            jsonOrderBook["checksum"]      = res32;
#ifndef __ENABLED_TEST__

            auto it = m_mapIMessage.find(ullMarketId);
            if (m_mapIMessage.end() != it)
            {
                auto *pPulsarProxy = it->second;
                pPulsarProxy->sendOrderBookDiff(jsonOrderBook);
            }
#else
            cfLog.info() << "MD diff: " << jsonOrderBook << std::endl;
#endif
            bRes = true;

        }
    } catch (...) {
        cfLog.fatal() << "Manager::creatOrderBookDiff exception!!!" << std::endl;
    }
    return bRes;
}

void Manager::sendOrderBookBest()
{
    if (cfLog.enabledTrace())
    {
        OPNX::CInOutLog cInOutLog("sendOrderBookBest");
    }
    try {

        for (auto it : m_mapEngine)
        {
            auto pIEngine = it.second;

            OPNX::OrderBookItem bastSelfBid = pIEngine->getSelfBestBid();
            OPNX::OrderBookItem askObItem = pIEngine->getBestAsk(&bastSelfBid);
            OPNX::OrderBookItem bidObItem = pIEngine->getBestBid(&askObItem);

            unsigned long long ullMarketId = pIEngine->getMarketId();

            auto itAsk = m_unmapAskObItem.find(ullMarketId);
            auto itBid = m_unmapBidObItem.find(ullMarketId);
            if ((m_unmapAskObItem.end() != itAsk && askObItem == itAsk->second
                && m_unmapBidObItem.end() != itBid && bidObItem == itBid->second )
                || (m_unmapAskObItem.end() == itAsk && 0 == askObItem.displayQuantity
                    && m_unmapBidObItem.end() == itBid && 0 == bidObItem.displayQuantity))
            {
                continue;
            }

            std::string strMarketCode = pIEngine->getMarketCode();
            unsigned long long ullFactor = pIEngine->getFactor();
            unsigned long long ullQtyFactor = pIEngine->getQtyFactor();

            nlohmann::json jsonOrderBook;
            nlohmann::json jsonAsk = nlohmann::json::array();
            nlohmann::json jsonBid = nlohmann::json::array();
            jsonAsk.push_back(static_cast<double>(askObItem.price)/ullFactor);
            jsonAsk.push_back(static_cast<double>(askObItem.displayQuantity)/ullQtyFactor);
            jsonBid.push_back(static_cast<double>(bidObItem.price)/ullFactor);
            jsonBid.push_back(static_cast<double>(bidObItem.displayQuantity)/ullQtyFactor);

            std::string strData = jsonAsk.dump() + jsonBid.dump();
            unsigned int res32 = OPNX::Utils::crc32b((unsigned char*)strData.c_str());
            jsonOrderBook["ask"]          = std::move(jsonAsk);
            jsonOrderBook["bid"]          = std::move(jsonBid);
            jsonOrderBook["timestamp"]     = OPNX::Utils::getMilliTimestamp();
            jsonOrderBook["checksum"]      = res32;
            jsonOrderBook["marketCode"] = strMarketCode;
#ifndef __ENABLED_TEST__

            if (m_mapIMessage.end() != m_mapIMessage.find(ullMarketId))
            {
                auto pPulsarProxy = m_mapIMessage.at(ullMarketId);
                pPulsarProxy->sendOrderBookBest(jsonOrderBook);
            }
#else
            cfLog.info() << "MD best: " << jsonOrderBook.dump() << std::endl;
#endif


            m_unmapAskObItem[ullMarketId] = askObItem;
            m_unmapBidObItem[ullMarketId] = bidObItem;
        }
    } catch (...) {
        cfLog.fatal() << "Manager::creatOrderBookDiff exception!!!" << std::endl;
    }
}

void Manager::sendPulsarLog(const std::string& strLog)
{
    if (nullptr != m_pLogPulsar)
    {
        m_pulsarLogQueue.push(strLog);
//        m_pLogPulsar->sendPulsarLog(strLog);
    }
}
void Manager::pulsarLogHandle()
{
    try {
        cfLog.printInfo() << "------Manager::pulsarLogHandle is running ------" << std::endl;

        while (m_bThreadRunning)
        {
            std::string strLog = "";
            if (m_pulsarLogQueue.try_pop(strLog))
            {
                if (nullptr != m_pLogPulsar)
                {
                    m_pLogPulsar->sendPulsarLog(strLog);
                }
            }
            else {
                usleep(1);
            }
        }
    } catch (...) {
        cfLog.fatal() << "Manager::pulsarLogHandle exception!!!" << std::endl;
    }
}
