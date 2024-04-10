//
// Created by Bob   on 2022/4/13.
//

#ifndef MATCHING_ENGINE_IMESSAGE_H
#define MATCHING_ENGINE_IMESSAGE_H


#include "threadsafe_queue.h"
#include "thread_queue.h"
#include "order.h"
#include "json.hpp"

class IMessage{
public:
    enum ProxyType
    {
        ORDER,
        ORDERS,
        MARK_PRICE,
        MARKET_SNAPSHOT,
        MARKET_DIFF,
        MARKET_BEST,
        COMMAND,
        CONSUMER_TO_LOG,
        HEARTBEAT,
        SPREAD_SNAPSHOT
    };
    virtual ~IMessage(){}
    virtual void releaseIMessage()=0;

    // Create all consumers and producers in the specified market
    // jsonConfig contains the following fields: marketId, marketCode, referencePair, factor
    virtual void createMarketAll(const nlohmann::json& jsonConfig)=0;

    // Create all test consumers and producers in the specified market
    // jsonConfig contains the following fields: marketId, marketCode, referencePair, factor
    virtual void createMarketAllTest(const nlohmann::json& jsonConfig)=0;

    // Create consumer and producer of commander
    virtual void createCommander(const std::string& strReferencePair)=0;

    // Create consumer and producer of test commander
    virtual void createTestCommander(const std::string& strReferencePair)=0;

    virtual void sendOrder(const std::string& strJsonData)=0;
    virtual void sendOrders(const std::string& strJsonData)=0;
    virtual void sendOrderBookSnapshot(const nlohmann::json& jsonData)=0;
    virtual void sendOrderBookDiff(const nlohmann::json& jsonData)=0;
    virtual void sendOrderBookBest(const nlohmann::json& jsonData)=0;
    virtual void sendCmd(const nlohmann::json& jsonData)=0;
    virtual void sendHeartbeat(const nlohmann::json& jsonData)=0;
    virtual void sendPulsarLog(const std::string& strLog)=0;

    virtual void setRecovery(bool bRecovery)=0;
    virtual bool getRecovery()=0;
    virtual void exitConsumer()=0;
    virtual void addOrderConsumer(int iCount) = 0;
    virtual void setMarketInfo(const nlohmann::json& jsonMarketInfo)=0;
};

IMessage* createIMessage(OPNX::OrderQueue<OPNX::Order>* pOrderQueue,
                         OPNX::OrderQueue<OPNX::Order>* pTriggerOrderQueue,
                         OPNX::OrderQueue<OPNX::TriggerPrice>* pMarkPriceQueue,
                         OPNX::OrderQueue<nlohmann::json>* pCmdQueue,
                         const std::string& strServiceUrl);

#endif //MATCHING_ENGINE_IMESSAGE_H
