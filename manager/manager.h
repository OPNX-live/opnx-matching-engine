//
// Created by Bob   on 2021/12/9.
//

#ifndef MATCHING_ENGINE_MANAGER_H
#define MATCHING_ENGINE_MANAGER_H

#include <map>
#include <string>
#include <mutex>

#include "IMessage.h"
#include "threadsafe_queue.h"
#include "thread_queue.h"
#include "IEngine.h"
#include "ITriggerOrder.h"

class Manager: public OPNX::ICallbackManager{

public:
    Manager(const std::string& strPulsarServiceUrl, const std::string& strReferencePair, nlohmann::json& jsonConfig)
    : m_strPulsarServiceUrl(strPulsarServiceUrl)
    , m_strReferencePair(strReferencePair)
    , m_bThreadRunning(true)
    , m_bOBThreadRunning(true)
    , m_bRecoveryEnd(false)
    , m_bEnableAuction(false)
    , m_bSendRecovery(false)
    , m_bEngineEnable(false)
    , m_iLogLevel(4)
    , m_iOrderBookCycle(100)
    , m_iOrderBookDepth(400)
    , m_iImpliedDepth(20)
    , m_iOrderOutThreadNumber(1)
    , m_iOrdersOutThreadNumber(3)
    , m_dSpreadMax(0.125)
    , m_iOrderActiveTime(0)
    , m_iSpreadFrequency(60)
    , m_ullSortId(0)
    , m_ullSendSortId(0)
    , m_iSnapshotLogCycle(100)
    , m_jsonConfig(jsonConfig)
    , m_pCmdPulsarProxy(nullptr)
    , m_pLogPulsar(nullptr){};
    Manager(const Manager &) = delete;
    Manager(Manager &&) = delete;
    Manager &operator=(const Manager &) = delete;
    Manager &operator=(Manager &&) = delete;
    ~Manager();

public:
    // IICallbackManager
    virtual void pulsarOrder(const OPNX::Order& order){
        OPNX::Order newOrder(order);
        m_orderOutQueue.push(newOrder);
    }
    virtual void pulsarOrderList(const std::vector<OPNX::Order>& orders){
        std::vector<OPNX::Order> newOrders(orders);
        m_ordersOutQueue.push(newOrders);
    }
    virtual void triggerOrderToEngine(const OPNX::Order& order){
        OPNX::Order newOrder(order);
        m_orderQueue.push(newOrder);
    }
    virtual void engineOrderToTrigger(const OPNX::Order& order){
        OPNX::Order newOrder(order);
        m_triggerOrderQueue.push(newOrder);
    }
    virtual void bestOrderBookChange(){
        bool isBestChange = true;
        m_bestChangeQueue.push(isBestChange);
    }
    virtual void orderStore(OPNX::Order* pOrder){
//        m_OrderStoreManager.asynWriteOrder(pOrder);
    }
public:
    void run(bool enablePulsarLog = false);
    void exit();

    void sendOrder(const OPNX::Order& order);
    void sendOrderList(const std::vector<OPNX::Order>& orders, unsigned long long ullSortId);

    void sendMeStatus(const std::string& strStatus);

    void heartbeat(bool bRecoveryEnd);
    void sendPulsarLog(const std::string& strLog);
    void openPulsarLog() { m_pLogPulsar = m_pCmdPulsarProxy; };
    void closedPulsarLog() { m_pLogPulsar = nullptr; };

    enum ThreadType {
        ORDER_IN,
        TRIGGER_ORDER_IN,
        MARK_PRICE,
        LAST_PRICE,
        ORDER_OUT,
        ORDERS_OUT,
        ORDER_BOOK,
        BEST_BOOK,
        CMD,
        TEST,
        PULSAR_LOG,
        SNAPSHOT,
    };
private:
    static void handleThread(Manager* pManager, ThreadType threadType);
    void handleOrder();
    void handleTriggerOrder();
    void handleMarkPrice();
    void handleLastPrice();
    void handleOrderOut();
    void handleOrdersOut();
    void handleOrderBook();
    void handleBestOrderBook();
    void handleSpreadSnapshot();
    void handleCmd(IMessage* pCmdIMessage, nlohmann::json jsonCmd);

    void testHandle();
    void testRandomOrder();

    void handleMarketsInfo(nlohmann::json jsonMarkets);
    void addMarketsInfo(nlohmann::json jsonMarkets, bool bIsRecovery=false);
    void deleteMarketsInfo(nlohmann::json jsonMarkets);
    void impliedEngine();
    void clearOrder();

    inline bool checkOrder(OPNX::Order& order);

    void sendOrderBookSnapshot(unsigned long long ullMarketId, const std::string& strMarketCode, unsigned long long ullFactor, unsigned long long ullQtyFactor, AskOrderBook& askOrderBook, BidOrderBook& bidOrderBook, unsigned long long ullSequenceNumber);
    bool sendOrderBookDiff(unsigned long long ullMarketId, const std::string& strMarketCode, unsigned long long ullFactor, unsigned long long ullQtyFactor, const AskOrderBook& askOrderBook, const BidOrderBook& bidOrderBook, unsigned long long ullSequenceNumber);
    void sendOrderBookBest();

    void pulsarLogHandle();
private:
    std::string m_strPulsarServiceUrl;
    std::string m_strReferencePair;
    nlohmann::json m_jsonConfig;
    volatile bool m_bThreadRunning;
    volatile bool m_bOBThreadRunning;
    volatile bool m_bRecoveryEnd;
    bool m_bEnableAuction;   // default disable
    bool m_bSendRecovery;
    int m_iLogLevel;
    int m_iOrderBookCycle;  // Order book cycle in milliseconds
    int m_iOrderBookDepth;
    int m_iImpliedDepth;
    int m_iOrderOutThreadNumber;
    int m_iOrdersOutThreadNumber;
    double m_dSpreadMax;
    int m_iOrderActiveTime;   // time difference, Accurate to milliseconds
    int m_iSpreadFrequency;
    std::map<unsigned long long, IMessage*> m_mapIMessage;
    std::map<unsigned long long, OPNX::IEngine*> m_mapEngine;
    std::map<unsigned long long, OPNX::ITriggerOrder*> m_mapITriggerOrderManager;
    std::multimap<std::string, OPNX::IEngine*> m_multimapTypeEngine;
    OPNX::OrderQueue<OPNX::Order> m_orderQueue;
    OPNX::OrderQueue<OPNX::Order> m_triggerOrderQueue;
    OPNX::OrderQueue<OPNX::TriggerPrice> m_markPriceQueue;
    OPNX::OrderQueue<nlohmann::json>  m_cmdQueue;
    OPNX::OrderQueue<bool>  m_bestChangeQueue;
    OPNX::OrderQueue<std::string>  m_pulsarLogQueue;

    OPNX::OrderQueue<OPNX::Order> m_orderOutQueue;
    OPNX::OrderQueue<std::vector<OPNX::Order>> m_ordersOutQueue;
    OPNX::OrderQueue<std::unordered_map<unsigned long long, long long >> m_lastPriceQueue;   // key is marketId, value is lastMatchPrice

    std::unordered_map<unsigned long long, AskOrderBook>  m_unmapAskOrderBook;
    std::unordered_map<unsigned long long, BidOrderBook>  m_unmapBidOrderBook;
    std::unordered_map<unsigned long long, OPNX::OrderBookItem>  m_unmapAskObItem;
    std::unordered_map<unsigned long long, OPNX::OrderBookItem>  m_unmapBidObItem;

    std::condition_variable m_condition;
    volatile bool m_bEngineEnable;
    std::condition_variable m_conditionCancelAll;
    std::atomic<unsigned long long> m_ullSortId;
    volatile unsigned long long m_ullSendSortId;
    int m_iSnapshotLogCycle;
    IMessage* m_pCmdPulsarProxy;
    IMessage* m_pLogPulsar;
};


#endif //MATCHING_ENGINE_MANAGER_H
