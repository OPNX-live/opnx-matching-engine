//
// Created by Bob   on 2021/12/8.
//

#ifndef __PULSAR_PROXY_H__
#define __PULSAR_PROXY_H__

#include <string>
#include <map>
#include <thread>
#include <mutex>

#include "IMessage.h"
#include "threadsafe_queue.h"
#include "thread_queue.h"
#include "pulsar/c/client.h"
#include "order.h"
#include "json.hpp"
#include "spin_mutex.hpp"


class PulsarProxy: public IMessage{
private:
    PulsarProxy(OPNX::OrderQueue<OPNX::Order>* pOrderQueue,
                OPNX::OrderQueue<OPNX::Order>* pTriggerOrderQueue,
                OPNX::OrderQueue<OPNX::TriggerPrice>* pMarkPriceQueue,
                OPNX::OrderQueue<nlohmann::json>* pCmdQueue,
                const std::string& strServiceUrl)
    : m_pOrderQueue(pOrderQueue)
    , m_pTriggerOrderQueue(pTriggerOrderQueue)
    , m_pMarkPriceQueue(pMarkPriceQueue)
    , m_pCmdQueue(pCmdQueue)
    , m_strServiceUrl(strServiceUrl)
    , m_pPulsarClient(nullptr)
    , m_pulsarClientConfiguration(pulsar_client_configuration_create())
    , m_pulsarProducerConfiguration(pulsar_producer_configuration_create())
    , m_pOrderProducer(nullptr)
    , m_pOrdersProducer(nullptr)
    , m_pBookSnapshotProducer(nullptr)
    , m_pBookDiffProducer(nullptr)
    , m_pBookBestProducer(nullptr)
    , m_pCmdProducer(nullptr)
    , m_pHeartbeatProducer(nullptr)
    , m_pLogProducer(nullptr)
    , m_bRunning(true)
    , m_bIsRecovery(true)
    , m_factor(100000000)
    , m_strMarketId("")
    , m_strMarketCode("")
    , m_strReferencePair("")
    , m_iOrderConsumerCount(20)
    , m_llMarkPrice(0){};
    PulsarProxy(const PulsarProxy &) = delete;
    PulsarProxy(PulsarProxy &&) = delete;
    PulsarProxy &operator=(const PulsarProxy &) = delete;
    PulsarProxy &operator=(PulsarProxy &&) = delete;
    ~PulsarProxy();
public:
    static IMessage* createIMessage(OPNX::OrderQueue<OPNX::Order>* pOrderQueue, OPNX::OrderQueue<OPNX::Order>* pTriggerOrderQueue, OPNX::OrderQueue<OPNX::TriggerPrice>* pMarkPriceQueue, OPNX::OrderQueue<nlohmann::json>* pCmdQueue, const std::string& strServiceUrl);
    virtual void releaseIMessage();

    // Create all consumers and producers in the specified market
    // jsonConfig contains the following fields: marketId, marketCode, referencePair, factor
    virtual void createMarketAll(const nlohmann::json& jsonConfig);

    // Create all test consumers and producers in the specified market
    // jsonConfig contains the following fields: marketId, marketCode, referencePair, factor
    virtual void createMarketAllTest(const nlohmann::json& jsonConfig);

    // Create consumer and producer of commander
    virtual void createCommander(const std::string& strReferencePair);

    // Create consumer and producer of test commander
    virtual void createTestCommander(const std::string& strReferencePair);

    virtual void sendOrder(const std::string& strJsonData);
    virtual void sendOrders(const std::string& strJsonData);
    virtual void sendOrderBookSnapshot(const nlohmann::json& jsonData);
    virtual void sendOrderBookDiff(const nlohmann::json& jsonData);
    virtual void sendOrderBookBest(const nlohmann::json& jsonData);
    virtual void sendCmd(const nlohmann::json& jsonData);
    virtual void sendHeartbeat(const nlohmann::json& jsonData);
    virtual void sendPulsarLog(const std::string& strLog);

    virtual void setRecovery(bool bRecovery) { m_bIsRecovery = bRecovery; }
    virtual bool getRecovery() { return m_bIsRecovery; }
    virtual void exitConsumer() { m_bRunning = false; }
    virtual void addOrderConsumer(int iCount);
    virtual void setMarketInfo(const nlohmann::json& jsonMarketInfo);

    pulsar_producer_t* createProducer(ProxyType proxyType, const std::string& strTopic, const std::string& strProducerName);

    void sendMsg(pulsar_producer_t* producer, const std::string& strJsonData, const std::string& strPropertyName="", const std::string& strPropertyValue="");
private:
    void createConsumer(ProxyType proxyType, const std::string& strTopic, const std::string& strConsumerName);
    void createConsumerListener(ProxyType proxyType, const std::string& strTopic, const std::string& strConsumerName);
    static void consumerOrderThread(PulsarProxy* pulsarProxy, const std::string& strTopic, const std::string& strConsumerName);
    static void consumerMarkPriceThread(PulsarProxy* pulsarProxy, const std::string& strTopic, const std::string& strConsumerName);
    static void consumerCmdThread(PulsarProxy* pulsarProxy, const std::string& strTopic, const std::string& strConsumerName);
    static void consumerToLogThread(PulsarProxy* pulsarProxy, const std::string& strTopic, const std::string& strConsumerName);
    void consumerSubscribe(pulsar_client_t* pPulsarClient, pulsar_consumer_configuration_t* pConf, pulsar_consumer_t** consumer, const std::string& strTopic, const std::string& strConsumerName);
    void consumerOrder(const std::string& strTopic, const std::string& strConsumerName);
    void consumerMarkPrice(const std::string& strTopic, const std::string& strConsumerName);
    void consumerCmd(const std::string& strTopic, const std::string& strConsumerName);
    // only print msg of consume
    void consumerToLog(const std::string& strTopic, const std::string& strConsumerName);
    // consumer listener
    void consumerListenerOrder(const std::string& strTopic, const std::string& strConsumerName);
    void consumerListenerMarkPrice(const std::string& strTopic, const std::string& strConsumerName);
    void consumerListenerCmd(const std::string& strTopic, const std::string& strConsumerName);
    // only print msg of consume
    void consumerListenerToLog(const std::string& strTopic, const std::string& strConsumerName);

    inline bool checkOrder(OPNX::Order& order);
private:
    OPNX::OrderQueue<OPNX::Order> * m_pOrderQueue;
    OPNX::OrderQueue<OPNX::Order> * m_pTriggerOrderQueue;
    OPNX::OrderQueue<OPNX::TriggerPrice> * m_pMarkPriceQueue;
    OPNX::OrderQueue<nlohmann::json> * m_pCmdQueue;
    std::map<ProxyType, pulsar_client_t*> m_mapProducerClient;
    std::map<ProxyType, pulsar_producer_t*> m_mapProducer;
    std::vector<std::thread> m_consumerThreadPool;
    std::vector<pulsar_consumer_t*> m_listConsumerListener;
    std::vector<pulsar_consumer_configuration_t*> m_listConsumerConfiguration;
    pulsar_client_t *m_pPulsarClient;
    pulsar_client_configuration_t *m_pulsarClientConfiguration;
    pulsar_producer_configuration_t *m_pulsarProducerConfiguration;
    pulsar_producer_t* m_pOrderProducer;
    pulsar_producer_t* m_pOrdersProducer;
    pulsar_producer_t* m_pBookSnapshotProducer;
    pulsar_producer_t* m_pBookDiffProducer;
    pulsar_producer_t* m_pBookBestProducer;
    pulsar_producer_t* m_pCmdProducer;
    pulsar_producer_t* m_pHeartbeatProducer;
    pulsar_producer_t* m_pLogProducer;
    volatile bool m_bRunning;
    volatile bool m_bIsRecovery;
    unsigned long long m_factor;
    std::string m_strServiceUrl;
    std::string m_strMarketId;
    std::string m_strMarketCode;
    std::string m_strReferencePair;
    int m_iOrderConsumerCount;
    mutable OPNX::spin_mutex m_spinMutexProducer;
    mutable OPNX::spin_mutex m_spinMutexSendOrder;
    mutable OPNX::spin_mutex m_spinMutexSendOrders;
    mutable std::condition_variable m_conditionCmd;
    mutable std::condition_variable m_conditionOrder;
    long long m_llMarkPrice;

};

#endif //__PULSAR_PROXY_H__
