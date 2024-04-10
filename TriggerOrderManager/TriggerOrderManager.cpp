//
// Created by Bob   on 2022/6/9.
//

#include "TriggerOrderManager.h"
#include "log.h"


OPNX::ITriggerOrder* createTriggerOrder(const nlohmann::json& jsonMarketInfo, OPNX::ICallbackManager* pCallbackManager)
{
    return TriggerOrderManager::createTriggerOrder(jsonMarketInfo, pCallbackManager);
}
OPNX::ITriggerOrder* TriggerOrderManager::createTriggerOrder(const nlohmann::json& jsonMarketInfo, OPNX::ICallbackManager* pCallbackManager)
{
    return static_cast<OPNX::ITriggerOrder*>(new TriggerOrderManager(jsonMarketInfo, pCallbackManager));
}

TriggerOrderManager::~TriggerOrderManager()
{
    clearOrder();
}


void TriggerOrderManager::handleOrder(OPNX::Order& order)
{
    OPNX::CInOutLog cInOutLog("TriggerOrderManager::handleOrder");

    OPNX::CAutoMutex  autoMutex(m_spinMutexTriggerOrder);
    try {
        if (OPNX::Order::NEW == order.action)
        {
            order.status = OPNX::Order::OPEN;
            saveToSearchOrder(order);
            m_pCallbackManager->pulsarOrder(order);
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
        else if (OPNX::Order::TRIGGER_BRACKET == order.action)
        {
            handleBracketOrder(order);
        }
        else
        {
            cfLog.error() << "order action is error !!!" << std::endl;
        }

    } catch (...) {
        cfLog.fatal() << "TriggerOrderManager::handleOrder exception!!!" << std::endl;
    }

}

void TriggerOrderManager::markPriceTriggerOrder(long long markPrice)
{
    if (cfLog.enabledTrace())
    {
        OPNX::CInOutLog cInOutLog(m_strMarketCode + " markPriceTriggerOrder");
    }

    if (OPNX::Order::MAX_PRICE == markPrice)
    {
        return;
    }
    try {
        OPNX::CAutoMutex autoMutex(m_spinMutexTriggerOrder);
        auto itLess = m_lessMarkPriceTriggerOrder.begin();
        while (itLess != m_lessMarkPriceTriggerOrder.end())
        {
            if (markPrice <= itLess->first)
            {
                std::for_each(itLess->second.begin(), itLess->second.end(), [&](auto it) {
                    auto &order = *(it.second);
                    OPNX::Order copyOrder(order);
                    copyOrder.isTriggered = true;
                    copyOrder.action = OPNX::Order::NEW;
                    if (OPNX::Order::STOP_LIMIT == copyOrder.type || OPNX::Order::TAKE_PROFIT_LIMIT == copyOrder.type)
                    {
                        copyOrder.type = OPNX::Order::LIMIT;
                    }
                    else // OPNX::Order::STOP_MARKET == copyOrder.type || OPNX::Order::TAKE_PROFIT_MARKET == copyOrder.type
                    {
                        copyOrder.type = OPNX::Order::MARKET;
                    }
                    if (nullptr != m_pCallbackManager)
                    {
                        m_pCallbackManager->triggerOrderToEngine(copyOrder);
                    }
                    auto item = m_unmapSearchOrder.find(order.orderId);
                    if (m_unmapSearchOrder.end() != item)
                    {
                        m_unmapSearchOrder.erase(item);
                    }
                });
                itLess = m_lessMarkPriceTriggerOrder.erase(itLess);
            }
            else
            {
                break;
            }
        }
        auto itGreater = m_greaterMarkPriceTriggerOrder.begin();
        while (itGreater != m_greaterMarkPriceTriggerOrder.end())
        {
            if (markPrice >= itGreater->first)
            {
                std::for_each(itGreater->second.begin(), itGreater->second.end(), [&](auto it) {
                    auto &order = *(it.second);
                    OPNX::Order copyOrder(order);
                    copyOrder.isTriggered = true;
                    copyOrder.action = OPNX::Order::NEW;
                    if (OPNX::Order::STOP_LIMIT == copyOrder.type || OPNX::Order::TAKE_PROFIT_LIMIT == copyOrder.type)
                    {
                        copyOrder.type = OPNX::Order::LIMIT;
                    }
                    else // OPNX::Order::STOP_MARKET == copyOrder.type || OPNX::Order::TAKE_PROFIT_MARKET == copyOrder.type
                    {
                        copyOrder.type = OPNX::Order::MARKET;
                    }
                    if (nullptr != m_pCallbackManager)
                    {
                        m_pCallbackManager->triggerOrderToEngine(copyOrder);
                    }
                    auto item = m_unmapSearchOrder.find(order.orderId);
                    if (m_unmapSearchOrder.end() != item)
                    {
                        m_unmapSearchOrder.erase(item);
                    }
                });
                itGreater = m_greaterMarkPriceTriggerOrder.erase(itGreater);
            }
            else
            {
                break;
            }
        }
    } catch (...) {
        cfLog.fatal() << "markPriceTriggerOrder exception!!!" << std::endl;
    }
}
void TriggerOrderManager::lastPriceTriggerOrder(long long lastPrice)
{
    if (cfLog.enabledTrace())
    {
        OPNX::CInOutLog cInOutLog(m_strMarketCode + " lastPriceTriggerOrder");
    }

    if (OPNX::Order::MAX_PRICE == lastPrice)
    {
        return;
    }
    try {
        OPNX::CAutoMutex autoMutex(m_spinMutexTriggerOrder);
        auto itLess = m_lessLastPriceTriggerOrder.begin();
        while (itLess != m_lessLastPriceTriggerOrder.end())
        {
            if (lastPrice <= itLess->first)
            {
                std::for_each(itLess->second.begin(), itLess->second.end(), [&](auto it) {
                    auto &order = *(it.second);
                    OPNX::Order copyOrder(order);
                    copyOrder.isTriggered = true;
                    copyOrder.action = OPNX::Order::NEW;
                    if (OPNX::Order::STOP_LIMIT == copyOrder.type || OPNX::Order::TAKE_PROFIT_LIMIT == copyOrder.type)
                    {
                        copyOrder.type = OPNX::Order::LIMIT;
                    }
                    else // OPNX::Order::STOP_MARKET == copyOrder.type || OPNX::Order::TAKE_PROFIT_MARKET == copyOrder.type
                    {
                        copyOrder.type = OPNX::Order::MARKET;
                    }
                    if (nullptr != m_pCallbackManager)
                    {
                        m_pCallbackManager->triggerOrderToEngine(copyOrder);
                    }
                    auto item = m_unmapSearchOrder.find(order.orderId);
                    if (m_unmapSearchOrder.end() != item)
                    {
                        m_unmapSearchOrder.erase(item);
                    }
                });
                itLess = m_lessLastPriceTriggerOrder.erase(itLess);
            }
            else
            {
                break;
            }
        }
        auto itGreater = m_greaterLastPriceTriggerOrder.begin();
        while (itGreater != m_greaterLastPriceTriggerOrder.end())
        {
            if (lastPrice >= itGreater->first)
            {
                std::for_each(itGreater->second.begin(), itGreater->second.end(), [&](auto it) {
                    auto &order = *(it.second);
                    OPNX::Order copyOrder(order);
                    copyOrder.isTriggered = true;
                    copyOrder.action = OPNX::Order::NEW;
                    if (OPNX::Order::STOP_LIMIT == copyOrder.type || OPNX::Order::TAKE_PROFIT_LIMIT == copyOrder.type)
                    {
                        copyOrder.type = OPNX::Order::LIMIT;
                    }
                    else // OPNX::Order::STOP_MARKET == copyOrder.type || OPNX::Order::TAKE_PROFIT_MARKET == copyOrder.type
                    {
                        copyOrder.type = OPNX::Order::MARKET;
                    }
                    if (nullptr != m_pCallbackManager)
                    {
                        m_pCallbackManager->triggerOrderToEngine(copyOrder);
                    }
                    auto item = m_unmapSearchOrder.find(order.orderId);
                    if (m_unmapSearchOrder.end() != item)
                    {
                        m_unmapSearchOrder.erase(item);
                    }
                });
                itGreater = m_lessLastPriceTriggerOrder.erase(itGreater);
            }
            else
            {
                break;
            }
        }
    } catch (...) {
        cfLog.fatal() << "lastPriceTriggerOrder exception!!!" << std::endl;
    }
}
void TriggerOrderManager::clearOrder()
{
    m_lessMarkPriceTriggerOrder.clear();
    m_greaterMarkPriceTriggerOrder.clear();
    m_lessLastPriceTriggerOrder.clear();
    m_greaterLastPriceTriggerOrder.clear();
    m_unmapSearchOrder.clear();
}

void TriggerOrderManager::handleCancelOrder(OPNX::Order& order)
{
    OPNX::CInOutLog cInOutLog(m_strMarketCode + " TriggerOrderManager::handleCancelOrder");
    try {

        if (0 != order.orderId)
        {
            auto it = m_unmapSearchOrder.find(order.orderId);
            if (m_unmapSearchOrder.end() == it)
            {
                // If the order cannot be found, the order needs to be delivered to the engine
                if (nullptr != m_pCallbackManager)
                {
                    m_pCallbackManager->triggerOrderToEngine(order);
                }
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

                eraseFromSearchOrderMap(it);

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
                    if (order.accountId == it->second.accountId)
                    {
                        if (0 == order.clientOrderId || (0 != order.clientOrderId && order.clientOrderId == it->second.clientOrderId))
                        {

                            it->second.action = OPNX::Order::CANCEL;
                            it = eraseFromSearchOrderMap(it);
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
        cfLog.fatal() << "TriggerOrderManager::handleCancelOrder exception!!!" << std::endl;
    }
}

void TriggerOrderManager::handleAmendOrder(OPNX::Order& order)
{
    OPNX::CInOutLog cInOutLog(m_strMarketCode + " TriggerOrderManager::handleAmendOrder");
    try {
        auto it = m_unmapSearchOrder.find(order.orderId);
        if (m_unmapSearchOrder.end() == it)
        {
            // If the order cannot be found, the order needs to be delivered to the engine
            if (nullptr != m_pCallbackManager)
            {
                m_pCallbackManager->triggerOrderToEngine(order);
            }
        }
        else
        {
            auto& oldOrder = it->second;


            OPNX::Order bakOrder(oldOrder);
            oldOrder.price = order.price;
            oldOrder.triggerPrice = order.triggerPrice;
            oldOrder.quantity = order.quantity;
            oldOrder.displayQuantity = order.displayQuantity;
            oldOrder.remainQuantity = oldOrder.quantity;
            oldOrder.amount = order.amount;
            oldOrder.remainAmount = order.remainAmount;
            oldOrder.status = OPNX::Order::OPEN;
            strncpy(&oldOrder.tag[0], &order.tag[0], sizeof(order.tag));
            memcpy(&order, &oldOrder, sizeof(OPNX::Order));
            if (oldOrder.triggerPrice != bakOrder.triggerPrice)
            {
                eraseFromTriggerOrderMap(bakOrder);
                saveToTriggerOrder(&oldOrder);
            }
            order.action = OPNX::Order::AMEND;
            order.timestamp = OPNX::Utils::getMilliTimestamp();
            m_pCallbackManager->pulsarOrder(order);

        }
    } catch (...) {
        cfLog.fatal() << "TriggerOrderManager::handleAmendOrder exception!!!" << std::endl;
    }
}

inline OPNX::Order* TriggerOrderManager::saveToSearchOrder(OPNX::Order& order)
{
    OPNX::CInOutLog cInOutLog(m_strMarketCode + " TriggerOrderManager::saveToSearchOrder");

    OPNX::Order* pOrder = nullptr;
    try {
        auto item = m_unmapSearchOrder.emplace(std::pair<unsigned long long, OPNX::Order>(order.orderId, order));
        pOrder = &(item.first->second);
        if (item.second)
        {
            if (0 == order.bracketOrderId)
            {
                saveToTriggerOrder(pOrder);
            }
            // 0 != order.bracketOrderId need to wait for the parent order traded before storing the child order in stop order
        }
        else
        {
            std::string strJsonOrder = "";
            OPNX::Order::orderToJsonString(order, strJsonOrder);
            cfLog.error() << "TriggerOrderManager::saveToSearchOrder order existed: " << strJsonOrder << std::endl;
        }
    } catch (...) {
        cfLog.fatal() << "TriggerOrderManager::saveToSearchOrder exception!!!" << std::endl;
    }
    return pOrder;
}

void TriggerOrderManager::saveToTriggerOrder(OPNX::Order* pOrder)
{
    if (nullptr != pOrder)
    {
        m_pCallbackManager->orderStore(pOrder);

        pOrder->sortId = OPNX::Utils::getSortId();
        if (OPNX::Order::GREATER_EQUAL == pOrder->stopCondition)
        {
            if (OPNX::Order::MARK_PRICE == pOrder->triggerType)
            {
                m_greaterMarkPriceTriggerOrder[pOrder->triggerPrice][pOrder->sortId] = pOrder;
            }
            else
            {
                m_greaterLastPriceTriggerOrder[pOrder->triggerPrice][pOrder->sortId] = pOrder;
            }
        }
        else if (OPNX::Order::LESS_EQUAL == pOrder->stopCondition)
        {
            if (OPNX::Order::MARK_PRICE == pOrder->triggerType)
            {
                m_lessMarkPriceTriggerOrder[pOrder->triggerPrice][pOrder->sortId] = pOrder;
            }
            else
            {
                m_lessLastPriceTriggerOrder[pOrder->triggerPrice][pOrder->sortId] = pOrder;
            }
        }

    }
}

inline void TriggerOrderManager::eraseFromSearchOrderMap(OPNX::Order& order)
{
    eraseFromTriggerOrderMap(order);
    auto it = m_unmapSearchOrder.find(order.orderId);
    if (m_unmapSearchOrder.end() != it)
    {
        m_unmapSearchOrder.erase(it);
    }
}

inline OPNX::SearchOrderMap::iterator TriggerOrderManager::eraseFromSearchOrderMap(OPNX::SearchOrderMap::iterator& it)
{
    auto& order = it->second;
    eraseFromTriggerOrderMap(order);
    return m_unmapSearchOrder.erase(it);
}

void TriggerOrderManager::eraseFromTriggerOrderMap(OPNX::Order& order)
{
    try {
        if (OPNX::Order::LESS_EQUAL == order.stopCondition)
        {
            if (OPNX::Order::MARK_PRICE == order.triggerType)
            {
                eraseFromTriggerOrderMap<OPNX::OrderDescendMap>(m_lessMarkPriceTriggerOrder, order);
            }
            else
            {
                eraseFromTriggerOrderMap<OPNX::OrderDescendMap>(m_lessLastPriceTriggerOrder, order);
            }
        }
        else if (OPNX::Order::GREATER_EQUAL == order.stopCondition)
        {
            if (OPNX::Order::MARK_PRICE == order.triggerType)
            {
                eraseFromTriggerOrderMap<OPNX::OrderAscendMap>(m_greaterMarkPriceTriggerOrder, order);
            }
            else
            {
                eraseFromTriggerOrderMap<OPNX::OrderAscendMap>(m_greaterLastPriceTriggerOrder, order);
            }
        }
    } catch (...) {
        cfLog.fatal() << "Engine::eraseFromPriceOrderMap exception!!!" << std::endl;
    }
}

template <typename PriceOrderMap>
void TriggerOrderManager::eraseFromTriggerOrderMap(PriceOrderMap& priceOrderMap, const OPNX::Order& order)
{
    auto orderMap = priceOrderMap.find(order.triggerPrice);
    if (priceOrderMap.end() != orderMap)
    {
        auto it = orderMap->second.find(order.sortId);
        if (orderMap->second.end() != it)
        {
            orderMap->second.erase(it);
        }
        if (orderMap->second.empty())
        {
            priceOrderMap.erase(orderMap);
        }
    }
}


void TriggerOrderManager::handleBracketOrder(const OPNX::Order& bracketOrder)
{
    if (0 != bracketOrder.takeProfitOrderId )
    {
        auto it = m_unmapSearchOrder.find(bracketOrder.takeProfitOrderId);
        if (m_unmapSearchOrder.end() != it)
        {
            if (0 == bracketOrder.bracketOrderId)
            {
                auto* pOrder = &(it->second);
                saveToTriggerOrder(pOrder);
            }
            else
            {
                it->second.status = OPNX::Order::CANCELED_BY_BRACKET_ORDER;
                m_pCallbackManager->pulsarOrder(it->second);
                eraseFromSearchOrderMap(it);
            }
        }
    }
    if (0 != bracketOrder.stopLossOrderId)
    {
        auto it = m_unmapSearchOrder.find(bracketOrder.stopLossOrderId);
        if (m_unmapSearchOrder.end() != it)
        {
            if (0 == bracketOrder.bracketOrderId)
            {
                auto* pOrder = &(it->second);
                saveToTriggerOrder(pOrder);
            }
            else
            {
                it->second.status = OPNX::Order::CANCELED_BY_BRACKET_ORDER;
                m_pCallbackManager->pulsarOrder(it->second);
                eraseFromSearchOrderMap(it);
            }
        }
    }
}



