//
// Created by Bob   on 2022/6/9.
//

#ifndef MATCHING_ENGINE_TRIGGERORDERMANAGER_H
#define MATCHING_ENGINE_TRIGGERORDERMANAGER_H


#include "common.h"
#include "ITriggerOrder.h"
#include "spin_mutex.hpp"
#include "json.hpp"
#include "utils.h"
#include "thread_queue.h"

class TriggerOrderManager: public OPNX::ITriggerOrder {
private:
    TriggerOrderManager(const nlohmann::json& jsonMarketInfo, OPNX::ICallbackManager* pCallbackManager)
            : m_jsonMarketInfo(jsonMarketInfo)
            , m_pCallbackManager(pCallbackManager)
            , m_strMarketCode("")
    {
        OPNX::Utils::getJsonValue<std::string>(m_strMarketCode, jsonMarketInfo, "marketCode");
    };
    TriggerOrderManager(const TriggerOrderManager &) = delete;
    TriggerOrderManager(TriggerOrderManager &&) = delete;
    TriggerOrderManager &operator=(const TriggerOrderManager &) = delete;
    TriggerOrderManager &operator=(TriggerOrderManager &&) = delete;
    ~TriggerOrderManager();

private:
    OPNX::SearchOrderMap m_unmapSearchOrder;   // save all orders
    OPNX::OrderDescendMap m_lessMarkPriceTriggerOrder; // key is price, Save the order with OrderStopCondition as LESS_EQUAL, in ascending order
    OPNX::OrderAscendMap m_greaterMarkPriceTriggerOrder; // key is price, Save the order with OrderStopCondition as GREATER_EQUAL, in descending order
    OPNX::OrderDescendMap m_lessLastPriceTriggerOrder; // key is price, Save the order with OrderStopCondition as LESS_EQUAL, in ascending order
    OPNX::OrderAscendMap m_greaterLastPriceTriggerOrder; // key is price, Save the order with OrderStopCondition as GREATER_EQUAL, in descending order

    nlohmann::json m_jsonMarketInfo;
    OPNX::ICallbackManager* m_pCallbackManager;

    std::string m_strMarketCode;

    mutable OPNX::spin_mutex m_spinMutexTriggerOrder;

public:
    static ITriggerOrder* createTriggerOrder(const nlohmann::json& jsonMarketInfo, OPNX::ICallbackManager* pCallbackManager);
    virtual void releaseITriggerOrder(){ delete this; };
    virtual void handleOrder(OPNX::Order& order);
    virtual void markPriceTriggerOrder(long long markPrice);
    virtual void lastPriceTriggerOrder(long long lastPrice);
    virtual std::string getMarketCode() { return m_strMarketCode; };
    virtual void clearOrder();
    virtual unsigned long long getOrdersCount() { return m_unmapSearchOrder.size(); };

private:
    void handleCancelOrder(OPNX::Order& order);
    void handleAmendOrder(OPNX::Order& order);
    inline OPNX::Order* saveToSearchOrder(OPNX::Order& order);
    void saveToTriggerOrder(OPNX::Order* order);
    void handleBracketOrder(const OPNX::Order& bracketOrder);

    inline void eraseFromSearchOrderMap(OPNX::Order& order);
    inline OPNX::SearchOrderMap::iterator eraseFromSearchOrderMap(OPNX::SearchOrderMap::iterator& it);
    void eraseFromTriggerOrderMap(OPNX::Order& order);
    template <typename PriceOrderMap>
    void eraseFromTriggerOrderMap(PriceOrderMap& priceOrderMap, const OPNX::Order& order);
};


#endif //MATCHING_ENGINE_TRIGGERORDERMANAGER_H
