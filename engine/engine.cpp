//
// Created by Bob   on 2021/12/24.
//

#include "engine.h"
#include "log.h"
#include "global.h"


OPNX::IEngine* createIEngine(const nlohmann::json& jsonMarketInfo, OPNX::ICallbackManager* pCallbackManager)
{
    return Engine::createEngine(jsonMarketInfo, pCallbackManager);
}

OPNX::IEngine* Engine::createEngine(const nlohmann::json& jsonMarketInfo, OPNX::ICallbackManager* pCallbackManager)
{
    return static_cast<OPNX::IEngine*>(new Engine(jsonMarketInfo, pCallbackManager));
}
Engine::~Engine()
{

}

void Engine::handleOrder(OPNX::Order& order)
{
    OPNX::CInOutLog cInOutLog("handleOrder");

    try {
        if (OPNX::Order::AUCTION == order.timeCondition)
        {
            unsigned long long  quantity = 0;
            std::vector<OPNX::Order> vecCancelOrder;
            auto it = m_unmapSearchOrder.begin();
            while (it != m_unmapSearchOrder.end()) {
                OPNX::Order oldOrder(it->second);
                if (order.source == oldOrder.source) {
                    order.side =  oldOrder.side;
                    quantity += oldOrder.remainQuantity;
                    oldOrder.status = OPNX::Order::CANCELED_BY_PRE_AUCTION;
                    m_pCallbackManager->pulsarOrder(oldOrder);
                    it = m_unmapSearchOrder.erase(it);

                    OPNX::Order newOrder(oldOrder);
                    newOrder.remainQuantity = 0;
                    updateOrderBook(newOrder, oldOrder);
                }
                else
                {
                    it++;
                }
            }
            order.quantity = quantity;
            order.remainQuantity = quantity;
            order.displayQuantity = order.quantity;
            if (OPNX::Order::BUY == order.side)
            {
                order.price = order.upperBound;
            } else {
                order.price = order.lowerBound;
            }
            if (0 == quantity) {
                order.status = OPNX::Order::CANCELED_BY_NO_AUCTION;
                m_pCallbackManager->pulsarOrder(order);
                return;
            }
        }
        if (OPNX::Order::NEW == order.action)
        {
            handleNewOrder(order);
        }
        else if (OPNX::Order::AMEND == order.action)
        {
            handleAmendOrder(order);
        }
        else if (OPNX::Order::CANCEL == order.action)
        {
            handleCancelOrder(order);
        }
        else if (OPNX::Order::RECOVERY == order.action)
        {
            order.action = OPNX::Order::NEW;
            saveToSearchOrder(order);
        }
        else
        {
            cfLog.error() << "order action is error !!!" << std::endl;
        }

    } catch (...) {
        cfLog.fatal() << "Engine::handleOrder exception!!!" << std::endl;
    }

}

void Engine::handleNewOrder(OPNX::Order& order)
{
    OPNX::CInOutLog cInOutLog(m_strMarketCode + " handleNewOrder");

    try {
        // handle FOK order
        if (OPNX::Order::FOK == order.timeCondition && 0 == order.amount)
        {
            OPNX::CInOutLog cInOutLog(m_strMarketCode + " OPNX::Order::FOK == order.timeCondition");

            OPNX::OrderBookItem bestAskItem;
            OPNX::OrderBookItem bestBidItem;
            bestAskItem = getBestAsk();
            bestBidItem = getBestBid(&bestAskItem);

            unsigned long long ullAmount = 0;
            if (OPNX::Order::BUY == order.side)
            {
                ullAmount = getAskMatchableAmount(order);
                ullAmount += getImpliedAskMatchableAmount(order.price, bestBidItem);
            }
            else  // OPNX::Order::SELL == order.side
            {
                ullAmount = getBidMatchableAmount(order);
                ullAmount += getImpliedBidMatchableAmount(order.price, bestAskItem);
            }
            if (ullAmount < order.quantity)
            {
                order.status = OPNX::Order::CANCELED_BY_FOK;
                m_pCallbackManager->pulsarOrder(order);
                return;
            }

//        cfLog.debug() << "ullAmount: " << ullAmount << ", order.quantity: " << order.quantity << std::endl;
        }
        // handle MAKER_ONLY order or MAKER_ONLY_REPRICE order
        if (OPNX::Order::MAKER_ONLY == order.timeCondition || OPNX::Order::MAKER_ONLY_REPRICE == order.timeCondition)
        {
            OPNX::OrderBookItem bestAskItem;
            OPNX::OrderBookItem bestBidItem;
            bestAskItem = getBestAsk();
            bestBidItem = getBestBid(&bestAskItem);

            if (OPNX::Order::BUY == order.side)
            {
                if (bestAskItem.price <= order.price && 0 < bestAskItem.quantity)
                {
                    if (OPNX::Order::MAKER_ONLY == order.timeCondition)
                    {
                        order.status = OPNX::Order::CANCELED_BY_MAKER_ONLY;
                        m_pCallbackManager->pulsarOrder(order);
                        return;
                    }
                    order.price = bestAskItem.price - m_miniTick;
                }
            }
            else  // OPNX::Order::SELL == order.side
            {
                if (bestBidItem.price >= order.price && 0 < bestBidItem.quantity)
                {
                    if (OPNX::Order::MAKER_ONLY == order.timeCondition)
                    {
                        order.status = OPNX::Order::CANCELED_BY_MAKER_ONLY;
                        m_pCallbackManager->pulsarOrder(order);
                        return;
                    }
                    order.price = bestBidItem.price + m_miniTick;
                }
            }
            saveToSearchOrder(order);
            m_pCallbackManager->pulsarOrder(order);
            return;
        }

        std::vector<OPNX::Order> vecMatchedOrder;        // Store two-party matching orders
        std::vector<OPNX::Order> vecThirdMatchedOrder;   // Store third-party matching orders
        unsigned long long ullInitRemainQuantity = order.remainQuantity;
        unsigned long long ullMatchableQuantity = 0;
        if (0 < order.amount)
        {
            ullMatchableQuantity = amountToMatchableQuantity(order.remainAmount, order.side);
            if (0 >= ullMatchableQuantity)
            {
                order.status = OPNX::Order::CANCELED_ALL_BY_IOC;
                m_pCallbackManager->pulsarOrder(order);
                return;
            }
        }
        else
        {
            ullMatchableQuantity = order.remainQuantity;
        }

        cfLog.debug() << m_strMarketCode << " handleNewOrder: matching begin" << std::endl;

        OPNX::SortOrderBook* pMakerOrders = nullptr;
        OPNX::OrderBookItem bestAskObItem;
        OPNX::OrderBookItem bestBidObItem;
        bool bIsSTP = false;
        while (0 < ullMatchableQuantity)
        {
            OPNX::OrderBookItem bestObItem;
            bestAskObItem = getBestAsk();
            bestBidObItem = getBestBid(&bestAskObItem);
            OPNX::Implier *pImplier = nullptr;
            unsigned long long ullMatchQuantity = 0;
            if (OPNX::Order::BUY == order.side)
            {
                pMakerOrders = getBestAskSortOrderBook();
                if (nullptr != pMakerOrders && !bIsSTP)
                {
                    bestObItem = pMakerOrders->obItem;
                }
                for (int i = 0; i < m_vecImpliers.size(); i++)
                {
                    OPNX::OrderBookItem impliedObItem = m_vecImpliers[i].getBestAsk(bestBidObItem);
                    if (0 == bestObItem.quantity || (bestObItem.price > impliedObItem.price && 0 != impliedObItem.quantity))
                    {
                        bestObItem = impliedObItem;
                        pImplier = &(m_vecImpliers[i]);
                    }
                }
                if (0 < bestObItem.quantity && order.price >= bestObItem.price)
                {
                    ullMatchQuantity = std::min(ullMatchableQuantity, bestObItem.quantity);
                }
                else
                {
                    break;
                }
            }
            else  // OPNX::Order::SELL == order.side
            {
                pMakerOrders = getBestBidSortOrderBook();
                if (nullptr != pMakerOrders && !bIsSTP)
                {
                    bestObItem = pMakerOrders->obItem;
                }
                for (int i = 0; i < m_vecImpliers.size(); i++)
                {
                    OPNX::OrderBookItem impliedObItem = m_vecImpliers[i].getBestBid(bestAskObItem);
                    if (0 == bestObItem.quantity || (bestObItem.price < impliedObItem.price && 0 != impliedObItem.quantity))
                    {
                        bestObItem = impliedObItem;
                        pImplier = &(m_vecImpliers[i]);
                    }
                }
                if (0 < bestObItem.quantity && order.price <= bestObItem.price)
                {
                    ullMatchQuantity = std::min(ullMatchableQuantity, bestObItem.quantity);
                }
                else
                {
                    break;
                }
            }

            cfLog.debug() << m_strMarketCode << " handleNewOrder: ullMatchQuantity: " << ullMatchQuantity << ", bestObItem.price: " << bestObItem.price << std::endl;
            if (0 >= ullMatchQuantity)
            {
                break;
            }

            if (nullptr == pImplier)
            {
                OPNX::CInOutLog cInOutLog(m_strMarketCode + " handleNewOrder: nullptr == pImplier");
                // Because pMakerOrders may be deleted in the matchOrder() function, the for loop exit condition cannot be judged by pMakerOrders
                while (0 < ullMatchQuantity)
                {
                    pMakerOrders = nullptr;
                    if (OPNX::Order::BUY == order.side)
                    {
                        pMakerOrders = getBestAskSortOrderBook();
                    }
                    else
                    {
                        pMakerOrders = getBestBidSortOrderBook();
                    }
                    if (nullptr != pMakerOrders && bestObItem.price == pMakerOrders->obItem.price)
                    {
                        OPNX::CInOutLog cInOutLog(m_strMarketCode + " handleNewOrder: nullptr != pMakerOrders");
                        unsigned long long ullMatchId = OPNX::Utils::getMatchId();
                        auto it = pMakerOrders->sortMapOrder.begin();
                        if (pMakerOrders->sortMapOrder.end() != it)
                        {
                            OPNX::Order* pMakerOrder = it->second;

                            if (OPNX::Order::STP_NONE != order.selfTradeProtectionType)
                            {
                                if (order.accountId == pMakerOrder->accountId)
                                {
                                    switch (order.selfTradeProtectionType) {
                                        case OPNX::Order::STP_TAKER: {
                                            if (OPNX::Order::FOK == order.timeCondition)
                                            {
                                                // There is a statistics section on the transaction volume in front of the FOK order.
                                                // If there is a self closing transaction after entering the matching cycle, it will be skipped
                                                bIsSTP = true;
                                                do {
                                                    it++;
                                                    if (pMakerOrders->sortMapOrder.end() != it)
                                                    {
                                                        pMakerOrder = it->second;
                                                    }
                                                    else {
                                                        break;
                                                    }
                                                } while (order.accountId == pMakerOrder->accountId);
                                                if (pMakerOrders->sortMapOrder.end() == it || order.accountId == pMakerOrder->accountId)
                                                {
                                                    break;
                                                }
                                            } else {
                                                order.status = OPNX::Order::CANCELED_BY_SELF_TRADE_PROTECTION;
                                                m_pCallbackManager->pulsarOrder(order);

                                                if (!vecMatchedOrder.empty())
                                                {
                                                    m_pCallbackManager->pulsarOrderList(vecMatchedOrder);
                                                    vecMatchedOrder.clear();
                                                }
                                                if (!vecThirdMatchedOrder.empty())
                                                {
                                                    m_pCallbackManager->pulsarOrderList(vecThirdMatchedOrder);
                                                    vecThirdMatchedOrder.clear();
                                                }
                                                return;
                                            }
                                        }
                                        case OPNX::Order::STP_MAKER: {
                                            pMakerOrder->status = OPNX::Order::CANCELED_BY_SELF_TRADE_PROTECTION;
                                            m_pCallbackManager->pulsarOrder(*pMakerOrder);
                                            OPNX::Order stpNewOrder(*pMakerOrder);
                                            stpNewOrder.remainQuantity = 0;
                                            updateOrderBook(stpNewOrder, *pMakerOrder);
                                            continue;
                                        }
                                        case OPNX::Order::STP_BOTH: {
                                            if (OPNX::Order::FOK == order.timeCondition)
                                            {
                                                // There is a statistics section on the transaction volume in front of the FOK order.
                                                // If there is a self closing transaction after entering the matching cycle, it will be skipped
                                                bIsSTP = true;
                                                do {
                                                    it++;
                                                    if (pMakerOrders->sortMapOrder.end() != it)
                                                    {
                                                        pMakerOrder = it->second;
                                                    }
                                                    else {
                                                        break;
                                                    }
                                                } while (order.accountId == pMakerOrder->accountId);
                                                if (pMakerOrders->sortMapOrder.end() == it || order.accountId == pMakerOrder->accountId)
                                                {
                                                    break;
                                                }
                                            } else {
                                                order.status = OPNX::Order::CANCELED_BY_SELF_TRADE_PROTECTION;
                                                m_pCallbackManager->pulsarOrder(order);
                                                pMakerOrder->status = OPNX::Order::CANCELED_BY_SELF_TRADE_PROTECTION;
                                                m_pCallbackManager->pulsarOrder(*pMakerOrder);
                                                OPNX::Order stpNewOrder(*pMakerOrder);
                                                stpNewOrder.remainQuantity = 0;
                                                updateOrderBook(stpNewOrder, *pMakerOrder);

                                                if (!vecMatchedOrder.empty())
                                                {
                                                    m_pCallbackManager->pulsarOrderList(vecMatchedOrder);
                                                    vecMatchedOrder.clear();
                                                }
                                                if (!vecThirdMatchedOrder.empty())
                                                {
                                                    m_pCallbackManager->pulsarOrderList(vecThirdMatchedOrder);
                                                    vecThirdMatchedOrder.clear();
                                                }
                                                return;
                                            }
                                        }
                                        case OPNX::Order::STP_NONE:
                                            break;
                                    }
                                }
                            }
                            unsigned long long quantity = getOrderMatchableQuantity(pMakerOrder);
                            unsigned long long ullMinMatchQuantity = std::min(ullMatchQuantity, quantity);
                            if (0 < ullMinMatchQuantity)
                            {
                                matchOrder(vecMatchedOrder, order, pMakerOrder, bestObItem.price, ullMinMatchQuantity, ullMatchId, nullptr, true, m_bIsRepo);
                                ullMatchQuantity -= ullMinMatchQuantity;
                                ullMatchableQuantity -= ullMinMatchQuantity;
                            }
                            else
                            {
                                logErrorOrder("order book is error: ", *pMakerOrder);
                                if (0 >= pMakerOrder->remainQuantity)
                                {
                                    OPNX::Order newOrder;
                                    memcpy(&newOrder, pMakerOrder, sizeof(OPNX::Order));
                                    updateOrderBook(newOrder, newOrder);
                                }
                                break;
                            }
                        }
                        else
                        {
                            if (pMakerOrders->sortMapOrder.empty())
                            {
                                OPNX::CAutoMutex autoMutex(m_spinMutexOrderBook);
                                auto itAsk = m_askOrderBook.begin();
                                if (m_askOrderBook.end() != itAsk)
                                {
                                    if (itAsk->second.sortMapOrder.empty())
                                    {
                                        m_askOrderBook.erase(itAsk);
                                    }
                                }
                                auto itBid = m_bidOrderBook.begin();
                                if (m_bidOrderBook.end() != itBid)
                                {
                                    if (itBid->second.sortMapOrder.empty())
                                    {
                                        m_bidOrderBook.erase(itBid);
                                    }
                                }
                            }
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }

                    // Split group orders
                    if (OPNX::Order::AUCTION != order.timeCondition && m_iOrderGroupCount <= vecMatchedOrder.size())
                    {
                        m_pCallbackManager->pulsarOrderList(vecMatchedOrder);
                        vecMatchedOrder.clear();
                    }
                }
            }
            else
            {
                OPNX::CInOutLog cInOutLog(m_strMarketCode + " handleNewOrder: nullptr != pImplier");

                std::vector<OPNX::Order> *pVecMatchedOrder = &vecThirdMatchedOrder;
                if (OPNX::Order::AUCTION == order.timeCondition)
                {
                    pVecMatchedOrder = &vecMatchedOrder;
                }

                unsigned long long remainingQuantity = pImplier->matchOrder(*pVecMatchedOrder, order, bestObItem.price, ullMatchQuantity, m_pCallbackManager, m_iOrderGroupCount, m_bIsRepo);
                ullMatchableQuantity -= (ullMatchQuantity - remainingQuantity);
                if (remainingQuantity == ullMatchQuantity)
                {
                    // The quantity of matching before and after is inconsistent
                    cfLog.error() << "The quantity of matching before and after is inconsistent!!!" << std::endl;
                    break;  //
                }
                cfLog.debug() << m_strMarketCode << " handleNewOrder: implied: ullMatchQuantity: " << ullMatchQuantity << ", remainingQuantity: " << remainingQuantity << std::endl;

                //Split group orders
                if (OPNX::Order::AUCTION != order.timeCondition && m_iOrderGroupCount <= vecThirdMatchedOrder.size())
                {
                    m_pCallbackManager->pulsarOrderList(vecThirdMatchedOrder);
                    vecThirdMatchedOrder.clear();
                }
            }

            if (0 < order.amount)
            {
                ullMatchableQuantity = amountToMatchableQuantity(order.remainAmount, order.side);
            }
            // Split group orders
            if (OPNX::Order::AUCTION != order.timeCondition)
            {
                if (m_iOrderGroupCount <= vecMatchedOrder.size()) {
                    m_pCallbackManager->pulsarOrderList(vecMatchedOrder);
                    vecMatchedOrder.clear();
                }

                if (m_iOrderGroupCount <= vecThirdMatchedOrder.size())
                {
                    m_pCallbackManager->pulsarOrderList(vecThirdMatchedOrder);
                    vecThirdMatchedOrder.clear();
                }
            }
        }
        cfLog.debug() << m_strMarketCode << " handleNewOrder: matching end" << std::endl;
//      bool bTriggerTrade = false;
//      long long triggerLastPrice = order.lastMatchPrice;
        if (!vecMatchedOrder.empty())
        {
//        bTriggerTrade = true;
            if (OPNX::Order::AUCTION == order.timeCondition)
            {
                long long lastMatchPrice = vecMatchedOrder[vecMatchedOrder.size() - 1].lastMatchPrice;
                if (OPNX::Order::BUY == order.side)
                {
                    if (lastMatchPrice < 0)
                    {
                        lastMatchPrice = 0;
                    }
                    if (0 != order.remainQuantity)
                    {
                        lastMatchPrice = order.upperBound;
                    }
                }
                else if (OPNX::Order::SELL == order.side)
                {
                    if (lastMatchPrice > 0)
                    {
                        lastMatchPrice = 0;
                    }
                    if (0 != order.remainQuantity)
                    {
                        lastMatchPrice = order.lowerBound;
                    }
                }
                for (int i = 0; i < vecMatchedOrder.size(); i++)
                {
                    vecMatchedOrder[i].lastMatchPrice = lastMatchPrice;
                }
                order.lastMatchPrice = lastMatchPrice;

            }
            m_pCallbackManager->pulsarOrderList(vecMatchedOrder);
            vecMatchedOrder.clear();
        }
        if (!vecThirdMatchedOrder.empty())
        {
            m_pCallbackManager->pulsarOrderList(vecThirdMatchedOrder);
            vecThirdMatchedOrder.clear();
        }
        if (0 < order.remainQuantity || 0 < order.remainAmount)
        {
            order.timestamp = OPNX::Utils::getMilliTimestamp();
            if (OPNX::Order::IOC == order.timeCondition)
            {
                if (0 < order.quantity)
                {
                    if (order.quantity == order.remainQuantity)
                    {
                        order.status = OPNX::Order::CANCELED_ALL_BY_IOC;
                    }
                    else
                    {
                        order.status = OPNX::Order::CANCELED_PARTIAL_BY_IOC;
                    }
                }
                if (0 < order.amount)
                {
                    if (order.amount == order.remainAmount)
                    {
                        order.status = OPNX::Order::CANCELED_ALL_BY_IOC;
                    }
                    else
                    {
                        order.status = OPNX::Order::CANCELED_PARTIAL_BY_IOC;
                    }
                }
                m_pCallbackManager->pulsarOrder(order);
                return;
            }
            else if (OPNX::Order::AUCTION == order.timeCondition)
            {
                if (order.quantity == order.remainQuantity)
                {
                    order.status = OPNX::Order::CANCELED_ALL_BY_AUCTION;
                }
                else
                {
                    order.status = OPNX::Order::CANCELED_PARTIAL_BY_AUCTION;
                }
                m_pCallbackManager->pulsarOrder(order);
                return;
            }
            if (OPNX::Order::MARKET == order.type || OPNX::Order::STOP_MARKET == order.type || OPNX::Order::TAKE_PROFIT_MARKET == order.type)
            {
                if (0 < order.quantity)
                {
                    if (order.quantity == order.remainQuantity)
                    {
                        order.status = OPNX::Order::CANCELED_BY_MARKET_ORDER_NOTHING_MATCH;
                    }
                    else
                    {
                        order.status = OPNX::Order::CANCELED_BY_MARKET_ORDER_NOT_FULL_MATCHED;
                    }
                }
                if (0 < order.amount)
                {
                    if (order.amount == order.remainAmount)
                    {
                        order.status = OPNX::Order::CANCELED_BY_MARKET_ORDER_NOTHING_MATCH;
                    }
                    else
                    {
                        order.status = OPNX::Order::CANCELED_BY_MARKET_ORDER_NOT_FULL_MATCHED;
                    }
                }
                m_pCallbackManager->pulsarOrder(order);
                return;
            }
            order.status = OPNX::Order::OPEN;
            saveToSearchOrder(order);
            if (order.quantity == order.remainQuantity || ullInitRemainQuantity == order.remainQuantity)
            {
                m_pCallbackManager->pulsarOrder(order);
            }
        }

    } catch (...) {
        cfLog.fatal() << "Engine::handleNewOrder exception!!!" << std::endl;
    }
}

void Engine::handleCancelOrder(OPNX::Order& order)
{
    OPNX::CInOutLog cInOutLog(m_strMarketCode + " handleCancelOrder");
    try {

        if (0 != order.orderId)
        {
            auto it = m_unmapSearchOrder.find(order.orderId);
            if (m_unmapSearchOrder.end() == it)
            {
                order.remainQuantity = 0;
                order.status = OPNX::Order::REJECT_CANCEL_ORDER_ID_NOT_FOUND;
                order.timestamp = OPNX::Utils::getMilliTimestamp();
                m_pCallbackManager->pulsarOrder(order);
            }
            else
            {
                auto& oldOrder = it->second;
                oldOrder.action = order.action;
                oldOrder.source = order.source;
                oldOrder.status = OPNX::Order::CANCELED_BY_USER;
                strncpy(&oldOrder.tag[0], &order.tag[0], sizeof(order.tag));
                memcpy(&order, &oldOrder, sizeof(OPNX::Order));
                order.timestamp = OPNX::Utils::getMilliTimestamp();
                OPNX::Order newOrder(oldOrder);
                newOrder.remainQuantity = 0;
                updateOrderBook(newOrder, oldOrder);

                m_pCallbackManager->pulsarOrder(order);
            }
        }
        else   // cancel all orders of accountId
        {
            if (0 != order.accountId)
            {
                auto it = m_unmapSearchOrder.begin();
                while (m_unmapSearchOrder.end() != it)
                {
                    OPNX::Order  oldOrder(it->second);
                    if (order.accountId == oldOrder.accountId)
                    {
                        if (0 == order.clientOrderId || (0 != order.clientOrderId && order.clientOrderId == it->second.clientOrderId))
                        {
                            it = m_unmapSearchOrder.erase(it);

                            OPNX::Order newOrder(oldOrder);
                            newOrder.remainQuantity = 0;
                            updateOrderBook(newOrder, oldOrder);
                        }
                        else
                        {
                            it++;
                        }
                    }
                    else
                    {
                        it++;
                    }
                }
            }
            else
            {
                clearOrder();
            }
        }
    } catch (...) {
        cfLog.fatal() << "Engine::handleCancelOrder exception!!!" << std::endl;
    }
}
void Engine::handleAmendOrder(OPNX::Order& order)
{
    OPNX::CInOutLog cInOutLog(m_strMarketCode + " handleAmendOrder");
    try {
        auto it = m_unmapSearchOrder.find(order.orderId);
        if (m_unmapSearchOrder.end() == it)
        {
            order.remainQuantity = 0;
            order.status = OPNX::Order::REJECT_AMEND_ORDER_ID_NOT_FOUND;
            order.timestamp = OPNX::Utils::getMilliTimestamp();
            m_pCallbackManager->pulsarOrder(order);
        }
        else
        {
            OPNX::Order oldOrder(it->second);
//            if (oldOrder.triggerPrice != order.triggerPrice)
//            {
//                order.status = OPNX::Order::REJECT_AMEND_ORDER_IS_TRIGGERED;
//                m_pCallbackManager->pulsarOrder(order);
//                return;
//            }
            long long remainQuantity = order.quantity - (oldOrder.quantity - oldOrder.remainQuantity);
            if (0 >= remainQuantity)
            {
//                order.status = OPNX::Order::REJECT_AMEND_NEW_QUANTITY_IS_LESS_THAN_MATCHED_QUANTITY;
                oldOrder.status = OPNX::Order::CANCELED_BY_AMEND;
                OPNX::Order newOrder(oldOrder);
                newOrder.remainQuantity = 0;
                updateOrderBook(newOrder, oldOrder);
                m_pCallbackManager->pulsarOrder(newOrder);
                return;
            }
            order.remainQuantity = remainQuantity;

            // If the quantity is same, the quantity remains unchanged and other fields are modified
//            if (oldOrder.quantity == order.quantity) {
//                order.quantity = oldOrder.quantity;
//                order.displayQuantity = oldOrder.displayQuantity;
//                order.remainQuantity = oldOrder.remainQuantity;
//            }
            order.isTriggered = oldOrder.isTriggered;

            if (OPNX::Order::canAmend(order, oldOrder))
            {
                OPNX::Order newOrder;
                memcpy(&newOrder, &oldOrder, sizeof(OPNX::Order));

                newOrder.price = order.price;
                newOrder.triggerPrice = order.triggerPrice;
                newOrder.quantity = order.quantity;
                newOrder.displayQuantity = order.displayQuantity;
                newOrder.remainQuantity = order.remainQuantity;
                newOrder.amount = order.amount;
                newOrder.status = OPNX::Order::OPEN;
                strncpy(&newOrder.tag[0], &order.tag[0], sizeof(order.tag));
                memcpy(&order, &newOrder, sizeof(OPNX::Order));
                order.action = OPNX::Order::AMEND;
                order.timestamp = OPNX::Utils::getMilliTimestamp();
                m_pCallbackManager->pulsarOrder(order);
                // update order book
                updateOrderBook(newOrder, oldOrder);
            }
            else
            {
                order.clientOrderId = oldOrder.clientOrderId;
                order.action = OPNX::Order::AMEND;
                order.lastMatchedOrderId = oldOrder.lastMatchedOrderId;
                order.lastMatchedOrderId2 = oldOrder.lastMatchedOrderId2;
                order.source = oldOrder.source;
                strncpy(&oldOrder.tag[0], &order.tag[0], sizeof(order.tag));

                oldOrder.status = OPNX::Order::CANCELED_BY_AMEND;
                OPNX::Order newOrder(oldOrder);
                newOrder.remainQuantity = 0;
                updateOrderBook(newOrder, oldOrder);

                handleNewOrder(order);

            }

        }
    } catch (...) {
        cfLog.fatal() << "Engine::handleAmendOrder exception!!!" << std::endl;
    }
}
inline OPNX::Order* Engine::saveToSearchOrder(OPNX::Order& order)
{
    OPNX::CInOutLog cInOutLog(m_strMarketCode + " saveToSearchOrder");

    OPNX::Order* pOrder = nullptr;
    try {
        auto item = m_unmapSearchOrder.emplace(std::pair<unsigned long long, OPNX::Order>(order.orderId, order));
        pOrder = &(item.first->second);
        if (item.second)
        {
            saveNewOrder(pOrder);
        }
        else
        {
            std::string strJsonOrder = "";
            OPNX::Order::orderToJsonString(order, strJsonOrder);
            cfLog.error() << "Engine::saveToSearchOrder order existed: " << strJsonOrder << std::endl;
        }
    } catch (...) {
        cfLog.fatal() << "Engine::saveToSearchOrder exception!!!" << std::endl;
    }
    return pOrder;
}
void Engine::saveNewOrder(OPNX::Order* pOrder)
{
    try {
        OPNX::CAutoMutex autoMutex(m_spinMutexOrderBook);
        if (nullptr != pOrder)
        {
            bool bestChanged = checkBestChange(*pOrder);
            pOrder->sortId = OPNX::Utils::getSortId();
            if (OPNX::Order::BUY == pOrder->side)
            {
                auto it = m_bidOrderBook.find(pOrder->price);
                if (m_bidOrderBook.end() != it)
                {
                    unsigned long long ullQuantity = getOrderMatchableQuantity(pOrder);
                    it->second.obItem.quantity = it->second.obItem.quantity + pOrder->remainQuantity;
                    it->second.obItem.displayQuantity = it->second.obItem.displayQuantity + ullQuantity;
                    it->second.sortMapOrder.emplace(std::pair<unsigned long long, OPNX::Order*>(pOrder->sortId, pOrder));
                }
                else
                {
                    unsigned long long ullQuantity = getOrderMatchableQuantity(pOrder);
                    OPNX::SortOrderBook sortOrderBook;
                    sortOrderBook.obItem.price = pOrder->price;
                    sortOrderBook.obItem.quantity = pOrder->remainQuantity;
                    sortOrderBook.obItem.displayQuantity = ullQuantity;
                    sortOrderBook.sortMapOrder.emplace(std::pair<unsigned long long, OPNX::Order*>(pOrder->sortId, pOrder));
                    m_bidOrderBook.emplace(std::pair<long long, OPNX::SortOrderBook>(pOrder->price, std::move(sortOrderBook)));
                }
            }
            else if (OPNX::Order::SELL == pOrder->side)
            {
                auto it = m_askOrderBook.find(pOrder->price);
                if (m_askOrderBook.end() != it)
                {
                    unsigned long long ullQuantity = getOrderMatchableQuantity(pOrder);
                    it->second.obItem.quantity = it->second.obItem.quantity + pOrder->remainQuantity;
                    it->second.obItem.displayQuantity = it->second.obItem.displayQuantity + ullQuantity;
                    it->second.sortMapOrder.emplace(std::pair<unsigned long long, OPNX::Order*>(pOrder->sortId, pOrder));
                }
                else
                {
                    unsigned long long ullQuantity = getOrderMatchableQuantity(pOrder);
                    OPNX::SortOrderBook sortOrderBook;
                    sortOrderBook.obItem.price = pOrder->price;
                    sortOrderBook.obItem.quantity = pOrder->remainQuantity;
                    sortOrderBook.obItem.displayQuantity = ullQuantity;
                    sortOrderBook.sortMapOrder.emplace(std::pair<unsigned long long, OPNX::Order*>(pOrder->sortId, pOrder));
                    m_askOrderBook.emplace(std::pair<long long, OPNX::SortOrderBook>(pOrder->price, std::move(sortOrderBook)));
                }
            }
            if (bestChanged)
            {
                m_pCallbackManager->bestOrderBookChange();
            }
        }
    } catch (const std::exception &e) {
        cfLog.error() << "Engine::saveNewOrder exception: " << e.what() << std::endl;
    } catch (...) {
        cfLog.fatal() << "Engine::saveNewOrder exception!!!" << std::endl;
    }
}

inline void Engine::eraseFromSearchOrderMap(const OPNX::Order& order)
{
    auto it = m_unmapSearchOrder.find(order.orderId);
    if (m_unmapSearchOrder.end() != it)
    {
        m_unmapSearchOrder.erase(it);
    }
}

inline bool Engine::checkBestChange(const OPNX::Order& order)
{
    bool bestChanged = false;
    try {
        if (OPNX::Order::BUY == order.side)
        {
            auto it = m_bidOrderBook.begin();
            if (m_bidOrderBook.end() != it)
            {
                if (it->first <= order.price)
                {
                    bestChanged = true;
                }
            }
            else
            {
                bestChanged = true;
            }
        }
        else
        {
            auto it = m_askOrderBook.begin();
            if (m_askOrderBook.end() != it)
            {
                if (it->first >= order.price)
                {
                    bestChanged = true;
                }
            }
            else
            {
                bestChanged = true;
            }
        }

    } catch (const std::exception &e) {
        cfLog.error() << "Engine::checkBestChange exception: " << e.what() << std::endl;
    } catch (...) {
        cfLog.fatal() << "Engine::checkBestChange exception!!!" << std::endl;
    }
    return bestChanged;
}

void Engine::updateOrderBook(const OPNX::Order& newOrder, const OPNX::Order& oldOrder)
{
    try {
        bool bestChanged = checkBestChange(newOrder);
        if (OPNX::Order::BUY == newOrder.side)
        {
            updateOrderBook<OPNX::OrderBookDescendMap>(m_bidOrderBook, newOrder, oldOrder);
        }
        else
        {
            updateOrderBook<OPNX::OrderBookAscendMap>(m_askOrderBook, newOrder, oldOrder);
        }
        if (bestChanged)
        {
            m_pCallbackManager->bestOrderBookChange();
        }

    } catch (const std::exception &e) {
        cfLog.error() << "Engine::updateOrderBook exception: " << e.what() << std::endl;
    } catch (...) {
        cfLog.fatal() << "Engine::updateOrderBook exception!!!" << std::endl;
    }
}

template<class SortOrderBookMap>
void Engine::updateOrderBook(SortOrderBookMap& sortOrderBookMap, const OPNX::Order& newOrder, const OPNX::Order& oldOrder)
{
    OPNX::CInOutLog cInOutLog(m_strMarketCode + " updateOrderBook");
    OPNX::CAutoMutex autoMutex(m_spinMutexOrderBook);
    try {

        auto itemOrderBook = sortOrderBookMap.find(oldOrder.price);
        if (sortOrderBookMap.end() != itemOrderBook)
        {
            auto item = itemOrderBook->second.sortMapOrder.find(oldOrder.sortId);
            if (itemOrderBook->second.sortMapOrder.end() != item)
            {
                unsigned long long ullOldQuantity = getOrderMatchableQuantity(&oldOrder);
                if (0 == newOrder.remainQuantity)
                {
                    itemOrderBook->second.obItem.quantity -= oldOrder.remainQuantity;
                    itemOrderBook->second.obItem.displayQuantity -= ullOldQuantity;
                    itemOrderBook->second.sortMapOrder.erase(item);
                    eraseFromSearchOrderMap(oldOrder);
                }
                else
                {
                    unsigned long long ullNewQuantity = getOrderMatchableQuantity(&newOrder);
                    if (newOrder.price == oldOrder.price && newOrder.quantity <= oldOrder.quantity)
                    {
                        itemOrderBook->second.obItem.quantity -= (oldOrder.remainQuantity - newOrder.remainQuantity);
                        itemOrderBook->second.obItem.displayQuantity -= (ullOldQuantity - ullNewQuantity);

                        OPNX::Order* pOrder = item->second;
                        memcpy(pOrder, &newOrder, sizeof(OPNX::Order));
                        if (oldOrder.quantity >= oldOrder.displayQuantity && ullNewQuantity == newOrder.displayQuantity) // Iceberg order
                        {
                            itemOrderBook->second.sortMapOrder.erase(item);
                            pOrder->sortId = OPNX::Utils::getSortId();
                            itemOrderBook->second.sortMapOrder.emplace(std::pair<unsigned long long, OPNX::Order*>(pOrder->sortId, pOrder));
                        }
                    }
                    else
                    {
                        itemOrderBook->second.obItem.quantity -= oldOrder.remainQuantity;
                        itemOrderBook->second.obItem.displayQuantity -= ullOldQuantity;

                        OPNX::Order* pOrder = item->second;
                        memcpy(pOrder, &newOrder, sizeof(OPNX::Order));
                        itemOrderBook->second.sortMapOrder.erase(item);
                        saveNewOrder(pOrder);
                    }
                }
            }
            else
            {
                logErrorOrder("order not find in OrderBook: ", oldOrder);
            }
            if (itemOrderBook->second.sortMapOrder.empty())
            {
                sortOrderBookMap.erase(itemOrderBook);
            }
            else if (0 == itemOrderBook->second.obItem.quantity || 0 == itemOrderBook->second.obItem.displayQuantity)
            {
                itemOrderBook->second.obItem.quantity = 0;
                itemOrderBook->second.obItem.displayQuantity = 0;
                for (auto it = itemOrderBook->second.sortMapOrder.begin(); itemOrderBook->second.sortMapOrder.end() != it; it++)
                {
                    OPNX::Order* pOrder = it->second;
                    unsigned long long ullQuantity = getOrderMatchableQuantity(pOrder);

                    itemOrderBook->second.obItem.quantity += pOrder->remainQuantity;
                    itemOrderBook->second.obItem.displayQuantity += ullQuantity;
                }
            }
        }
        else
        {
            logErrorOrder("order not find in OrderBook: ", oldOrder);
        }

    } catch (...) {
        cfLog.fatal() << "Engine::updateOrderBook exception!!!" << std::endl;
    }
}

void Engine::setMarketInfo(const nlohmann::json& jsonMarketInfo)
{
    m_jsonMarketInfo = jsonMarketInfo;
    OPNX::Utils::getJsonValue<std::string>(m_strMarketCode, jsonMarketInfo, "marketCode");
    OPNX::Utils::getJsonValue<std::string>(m_strType, jsonMarketInfo, "type");
    std::string strCycleType = "";
    OPNX::Utils::getJsonValue<std::string>(strCycleType, jsonMarketInfo, "cycleType");
    if ("FUTURE" == m_strType && "SWAP" == strCycleType)
    {
        m_strType = "PERP";
    }
    if ("REPO" == m_strType)
    {
        m_bIsRepo = true;
    }
    OPNX::Utils::getJsonValue<std::string>(m_strReferencePair, jsonMarketInfo, "referencePair");
    OPNX::Utils::getJsonValue<unsigned long long>(m_ullMarketId, jsonMarketInfo, "marketId");
    OPNX::Utils::getJsonValue<unsigned long long>(m_ullFactor, jsonMarketInfo, "factor");
    OPNX::Utils::getJsonValue<unsigned long long>(m_ullQtyFactor, jsonMarketInfo, "qtyFactor");
    OPNX::Utils::getJsonValue<long long>(m_llMakerFees, jsonMarketInfo, "makerFee");
    double dMiniTick = 0.0;
    OPNX::Utils::getJsonValue<double>(dMiniTick, jsonMarketInfo, "tickSize");
    m_miniTick = OPNX::Utils::double2int(dMiniTick*m_ullFactor);

    double dQtyIncrement = 0.0;
    OPNX::Utils::getJsonValue<double>(dQtyIncrement, jsonMarketInfo, "qtyIncrement");
    m_qtyIncrement = OPNX::Utils::double2int(dQtyIncrement*m_ullQtyFactor);

    for (auto it = m_vecImpliers.begin(); it != m_vecImpliers.end(); it++)
    {
        it->setMiniTick(m_miniTick);
    }
}

void Engine::clearOrder()
{
    m_askOrderBook.clear();
    m_bidOrderBook.clear();
    m_unmapSearchOrder.clear();
}

unsigned long long Engine::getAskMatchableAmount(const OPNX::Order& order)
{
    long long limitPrice = order.price;
    unsigned long long ullAmount = 0;
    auto it = m_askOrderBook.begin();
    while (m_askOrderBook.end() != it)
    {
        if (it->first <= limitPrice || OPNX::Order::MARKET == order.type || OPNX::Order::STOP_MARKET == order.type || OPNX::Order::TAKE_PROFIT_MARKET == order.type)
        {
            if (OPNX::Order::STP_NONE == order.selfTradeProtectionType) {
                ullAmount += it->second.obItem.quantity;
            } else {
                for (auto itMap = it->second.sortMapOrder.begin(); itMap != it->second.sortMapOrder.end(); itMap++)
                {
                    if (order.accountId == itMap->second->accountId)
                    {
                        switch (order.selfTradeProtectionType) {
                            case OPNX::Order::STP_TAKER:
                            case OPNX::Order::STP_BOTH: {
                                return ullAmount;
                            }
                            case OPNX::Order::STP_MAKER: {
                                continue;
                            }
                            case OPNX::Order::STP_NONE:
                                break;
                        }
                    }
                    else
                    {
                        ullAmount += itMap->second->quantity;
                    }
                }
            }
        }
        it++;
    }
    return ullAmount;
}
unsigned long long Engine::getBidMatchableAmount(const OPNX::Order& order)
{
    long long limitPrice = order.price;
    unsigned long long ullAmount = 0;
    auto it = m_bidOrderBook.begin();
    while (m_bidOrderBook.end() != it)
    {
        if (it->first >= limitPrice || OPNX::Order::MARKET == order.type || OPNX::Order::STOP_MARKET == order.type || OPNX::Order::TAKE_PROFIT_MARKET == order.type)
        {
            if (OPNX::Order::STP_NONE == order.selfTradeProtectionType) {
                ullAmount += it->second.obItem.quantity;
            } else {
                for (auto itMap = it->second.sortMapOrder.begin(); itMap != it->second.sortMapOrder.end(); itMap++)
                {
                    if (order.accountId == itMap->second->accountId)
                    {
                        switch (order.selfTradeProtectionType) {
                            case OPNX::Order::STP_TAKER:
                            case OPNX::Order::STP_BOTH: {
                                return ullAmount;
                            }
                            case OPNX::Order::STP_MAKER: {
                                continue;
                            }
                            case OPNX::Order::STP_NONE:
                                break;
                        }
                    }
                    else
                    {
                        ullAmount += itMap->second->quantity;
                    }
                }
            }
        }
        it++;
    }
    return ullAmount;
}
unsigned long long Engine::getImpliedAskMatchableAmount(long long limitPrice, const OPNX::OrderBookItem& bestBidItem)
{
    unsigned long long ullAmount = 0;
    for (auto it = m_vecImpliers.begin(); m_vecImpliers.end() != it; it++)
    {
        unsigned long long ullImpliedAmount = it->getAskMatchableAmount(limitPrice, bestBidItem);
        ullAmount += ullImpliedAmount;
    }
    return ullAmount;
}
unsigned long long Engine::getImpliedBidMatchableAmount(long long limitPrice, const OPNX::OrderBookItem& bestAskItem)
{
    unsigned long long ullAmount = 0;
    for (auto it = m_vecImpliers.begin(); m_vecImpliers.end() != it; it++)
    {
        unsigned long long ullImpliedAmount = it->getBidMatchableAmount(limitPrice, bestAskItem);
        ullAmount += ullImpliedAmount;
    }
    return ullAmount;
}
OPNX::OrderBookItem Engine::getSelfBestAsk()
{
    OPNX::CAutoMutex autoMutex(m_spinMutexOrderBook);
    OPNX::OrderBookItem obItem;
    auto it = m_askOrderBook.begin();
    if (m_askOrderBook.end() != it)
    {
        obItem = it->second.obItem;
    }
    return obItem;
}
OPNX::OrderBookItem Engine::getSelfBestBid()
{
    OPNX::CAutoMutex autoMutex(m_spinMutexOrderBook);
    OPNX::OrderBookItem obItem;
    auto it = m_bidOrderBook.begin();
    if (m_bidOrderBook.end() != it)
    {
        obItem = it->second.obItem;
    }
    return obItem;
}
OPNX::OrderBookItem Engine::getBestAsk(const OPNX::OrderBookItem* pBestBidObItem)
{
    OPNX::OrderBookItem obItem = getSelfBestAsk();
    OPNX::OrderBookItem bestBidItem;
    if (nullptr == pBestBidObItem)
    {
        bestBidItem = getSelfBestBid();
    }
    else
    {
        bestBidItem.price = pBestBidObItem->price;
        bestBidItem.quantity = pBestBidObItem->quantity;
    }
    for (auto implier: m_vecImpliers)
    {
        OPNX::OrderBookItem impliedObItem = implier.getBestAsk(bestBidItem);
        if (0 == obItem.quantity || (obItem.price > impliedObItem.price && 0 != impliedObItem.quantity))
        {
            obItem = impliedObItem;
        } else if (obItem.price == impliedObItem.price) {
            obItem.quantity += impliedObItem.quantity;
            obItem.displayQuantity += impliedObItem.displayQuantity;
        }
    }
    return obItem;
}
OPNX::OrderBookItem Engine::getBestBid(const OPNX::OrderBookItem* pBestAskObItem)
{
    OPNX::OrderBookItem obItem = getSelfBestBid();
    OPNX::OrderBookItem bestAskItem = getSelfBestAsk();
    if (nullptr == pBestAskObItem)
    {
        bestAskItem = getSelfBestAsk();
    }
    else
    {
        bestAskItem.price = pBestAskObItem->price;
        bestAskItem.quantity = pBestAskObItem->quantity;
    }
    for (auto implier: m_vecImpliers)
    {
        OPNX::OrderBookItem impliedObItem = implier.getBestBid(bestAskItem);
        if (0 == obItem.quantity || (obItem.price < impliedObItem.price && 0 != impliedObItem.quantity))
        {
            obItem = impliedObItem;
        } else if (obItem.price == impliedObItem.price) {
            obItem.quantity += impliedObItem.quantity;
            obItem.displayQuantity += impliedObItem.displayQuantity;
        }
    }
    return obItem;
}

OPNX::Order* Engine::getBestAskOrder()
{
    auto sortOrderBook = m_askOrderBook.begin();
    if (m_askOrderBook.end() != sortOrderBook)
    {
        auto it = sortOrderBook->second.sortMapOrder.begin();
        if (sortOrderBook->second.sortMapOrder.end() != it)
        {
            OPNX::Order* pOrder = it->second;
            return pOrder;
        }
    }
    return nullptr;
}
OPNX::Order* Engine::getBestBidOrder()
{
    auto sortOrderBook = m_bidOrderBook.begin();
    if (m_bidOrderBook.end() != sortOrderBook)
    {
        auto it = sortOrderBook->second.sortMapOrder.begin();
        if (sortOrderBook->second.sortMapOrder.end() != it)
        {
            OPNX::Order* pOrder = it->second;
            return pOrder;
        }
    }
    return nullptr;
}
OPNX::SortOrderBook* Engine::getBestAskSortOrderBook()
{
    auto sortOrderMap = m_askOrderBook.begin();
    if (m_askOrderBook.end() != sortOrderMap)
    {
        return &(sortOrderMap->second);
    }
    return nullptr;
}
OPNX::SortOrderBook* Engine::getBestBidSortOrderBook()
{
    auto sortOrderMap = m_bidOrderBook.begin();
    if (m_bidOrderBook.end() != sortOrderMap)
    {
        return &(sortOrderMap->second);
    }
    return nullptr;
}
void Engine::handleBracketOrder(OPNX::Order* pBracketOrder)
{
    if (0 != pBracketOrder->stopLossOrderId || 0 != pBracketOrder->takeProfitOrderId)
    {
        OPNX::Order newOrder;
        memcpy(&newOrder, pBracketOrder, sizeof(OPNX::Order));
        newOrder.action = OPNX::Order::TRIGGER_BRACKET;
        m_pCallbackManager->engineOrderToTrigger(newOrder);
    }
}
void Engine::matchOrder(std::vector<OPNX::Order>& vecMatchedOrder, OPNX::Order& takerOrder, OPNX::Order* pMakerOrder,
                        long long llMatchedPrice, unsigned long long ullMatchQuantity, unsigned long long ullMatchedId,
                        OPNX::Order* pThirdOrder, bool bHandleTakerOrder, bool bHasRepo)
{
    OPNX::CInOutLog cInOutLog(m_strMarketCode + " matchOrder");
    try {
        if (nullptr != pMakerOrder && 0 < ullMatchQuantity)
        {
            unsigned long long ullThirdOrderId = 0;
            long long llLeg2Price = 0;
            if (nullptr != pThirdOrder)
            {
                ullThirdOrderId = pThirdOrder->orderId;
                if (m_bIsRepo || bHasRepo)
                {
                    if (g_ullPerpMarketId == takerOrder.marketId)
                    {
                        llLeg2Price = llMatchedPrice;
                    }
                    else if (g_ullPerpMarketId == pMakerOrder->marketId)
                    {
                        llLeg2Price = pMakerOrder->price;
                    }
                    else if (g_ullPerpMarketId == pThirdOrder->marketId)
                    {
                        llLeg2Price = pThirdOrder->price;
                    }
                }
            }
            else
            {
                if (m_bIsRepo || bHasRepo)
                {
                    llLeg2Price = g_llPerpMarkPrice;
                }
            }
            unsigned long long timestamp = OPNX::Utils::getMilliTimestamp();
            if (bHandleTakerOrder)
            {
                takerOrder.lastMatchQuantity = ullMatchQuantity;
                takerOrder.lastMatchPrice = llMatchedPrice;
                takerOrder.matchedId = ullMatchedId;
                takerOrder.lastMatchedOrderId = pMakerOrder->orderId;
                takerOrder.lastMatchedOrderId2 = ullThirdOrderId;
                takerOrder.matchedType = OPNX::Order::TAKER;
                takerOrder.timestamp = timestamp;
                if (g_ullRepoMarketId == takerOrder.marketId)
                {
                    takerOrder.leg2Price = llLeg2Price;
                }
                if (0 < takerOrder.amount)
                {
                    takerOrder.remainAmount -= (static_cast<double>(ullMatchQuantity)/m_ullFactor)*static_cast<double>(llMatchedPrice);
                    if (0 < takerOrder.remainAmount)
                    {
                        takerOrder.status = OPNX::Order::PARTIAL_FILL;
                    }
                    else
                    {
                        takerOrder.status = OPNX::Order::FILLED;
                    }
                }
                else
                {
                    takerOrder.remainQuantity -= ullMatchQuantity;
                    if (0 < takerOrder.remainQuantity)
                    {
                        takerOrder.status = OPNX::Order::PARTIAL_FILL;
                    }
                    else
                    {
                        takerOrder.status = OPNX::Order::FILLED;
                        // handle bracket orders
                        handleBracketOrder(&takerOrder);
                    }
                }
                vecMatchedOrder.push_back(takerOrder);
            }

            OPNX::Order makerMatchOrder;
            memcpy(&makerMatchOrder, pMakerOrder, sizeof(OPNX::Order));
            makerMatchOrder.lastMatchQuantity = ullMatchQuantity;
            makerMatchOrder.lastMatchPrice = pMakerOrder->price;
            makerMatchOrder.remainQuantity -= ullMatchQuantity;
            makerMatchOrder.matchedId = ullMatchedId;
            makerMatchOrder.lastMatchedOrderId = takerOrder.orderId;
            makerMatchOrder.lastMatchedOrderId2 = ullThirdOrderId;
            makerMatchOrder.matchedType = OPNX::Order::MAKER;
            makerMatchOrder.timestamp = timestamp;
            if (g_ullRepoMarketId == makerMatchOrder.marketId)
            {
                makerMatchOrder.leg2Price = llLeg2Price;
            }
            if (0 < makerMatchOrder.remainQuantity)
            {
                makerMatchOrder.status = OPNX::Order::PARTIAL_FILL;
            }
            else
            {
                makerMatchOrder.status = OPNX::Order::FILLED;
                // handle bracket orders
                handleBracketOrder(&makerMatchOrder);
            }
            vecMatchedOrder.push_back(makerMatchOrder);
            updateOrderBook(makerMatchOrder, *pMakerOrder);
        }
        else
        {
            cfLog.error() << "Engine::matchOrder nullptr == pMakerOrder || ullMatchQuantity == " << ullMatchQuantity << std::endl;
        }
    } catch (...) {
        cfLog.fatal() << "Engine::matchOrder exception!!!" << std::endl;
    }
}

unsigned long long Engine::amountToMatchableQuantity(unsigned long long ullAmount, OPNX::Order::OrderSide side)
{
    unsigned long long ullMatchableQuantity = 0;
    OPNX::OrderBookItem bestObItem;
    if (OPNX::Order::BUY == side)
    {
        bestObItem = getBestAsk();
    }
    else
    {
        bestObItem = getBestBid();
    }
    if (0 < bestObItem.quantity)
    {
        ullMatchableQuantity = round_down(OPNX::Utils::double2int((static_cast<double>(ullAmount) / bestObItem.price) * m_ullFactor), m_qtyIncrement);
        while (ullAmount < static_cast<double>(ullMatchableQuantity)/m_ullFactor*bestObItem.price)
        {
            ullMatchableQuantity -= m_qtyIncrement;
        }
    }
    return ullMatchableQuantity;
}

void Engine::getSelfDisplayAskOrderBook(AskOrderBook& askOrderBook, int iSize)
{
    OPNX::CAutoMutex autoMutex(m_spinMutexOrderBook);
    auto it = m_askOrderBook.begin();
    for(int i = 0; i < iSize && m_askOrderBook.end() != it; it++)
    {
        askOrderBook[it->first] = it->second.obItem.displayQuantity;
        i++;
    }
}
void Engine::getSelfDisplayBidOrderBook(BidOrderBook& bidOrderBook, int iSize)
{
    OPNX::CAutoMutex autoMutex(m_spinMutexOrderBook);
    auto it = m_bidOrderBook.begin();
    for(int i = 0; i < iSize && m_bidOrderBook.end() != it; it++)
    {
        bidOrderBook[it->first] = it->second.obItem.displayQuantity;
        i++;
    }
}
void Engine::getDisplayAskOrderBook(AskOrderBook& askOrderBook, int iSize, int iImpliedSize, const OPNX::OrderBookItem* pBestBidObItem)
{
    getSelfDisplayAskOrderBook(askOrderBook, iSize);

    OPNX::OrderBookItem bestBidItem;
    if (nullptr == pBestBidObItem)
    {
        bestBidItem = getSelfBestBid();
    }
    else
    {
        bestBidItem.price = pBestBidObItem->price;
        bestBidItem.quantity = pBestBidObItem->quantity;
    }
    for (auto implier: m_vecImpliers)
    {
        implier.impliedAskOrderBook(askOrderBook, iImpliedSize, bestBidItem);
    }
}
void Engine::getDisplayBidOrderBook(BidOrderBook& bidOrderBook, int iSize, int iImpliedSize, const OPNX::OrderBookItem* pBestAskObItem)
{
    getSelfDisplayBidOrderBook(bidOrderBook, iSize);

    OPNX::OrderBookItem bestAskItem;
    if (nullptr == pBestAskObItem)
    {
        bestAskItem = getSelfBestAsk();
    }
    else
    {
        bestAskItem.price = pBestAskObItem->price;
        bestAskItem.quantity = pBestAskObItem->quantity;
    }
    for (auto implier: m_vecImpliers)
    {
        implier.impliedBidOrderBook(bidOrderBook, iImpliedSize, bestAskItem);
    }
}

unsigned long long Engine::getOrderMatchableQuantity(const OPNX::Order* pOrder)
{
    unsigned long long quantity = pOrder->remainQuantity;
    if (pOrder->displayQuantity < pOrder->quantity && 0 < pOrder->displayQuantity)
    {
        quantity = pOrder->displayQuantity - (pOrder->quantity - pOrder->remainQuantity) % pOrder->displayQuantity;
        if (0 < pOrder->remainQuantity && pOrder->remainQuantity <= pOrder->displayQuantity)
        {
            quantity = pOrder->remainQuantity;
        }
    }
    return quantity;
}

inline void Engine::logErrorOrder(const std::string& strError, const OPNX::Order& order)
{
    std::string strJsonOrder = "";
    OPNX::Order::orderToJsonString(order, strJsonOrder);
    cfLog.error() << strError << strJsonOrder << std::endl;
}