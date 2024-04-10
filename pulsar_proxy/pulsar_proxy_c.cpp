//
// Created by Bob   on 2021/12/8.
//

#include <thread>
#include <unistd.h>
#include "pulsar_proxy_c.h"
#include "log.h"
#include "utils.h"
#include "rapidjson/document.h"

const std::string PULSAR_TOPIC_ORDER_IN = "persistent://OPNX-V1/PRETRADE-ME/ORDER-IN-";
const std::string PULSAR_TOPIC_ORDER_OUT = "persistent://OPNX-V1/ME-POSTTRADE/ORDER-OUT-";
const std::string PULSAR_TOPIC_MD_SNAPSHOT = "non-persistent://OPNX-V1/ME-WS/MD-SNAPSHOT-";
const std::string PULSAR_TOPIC_MD_DIFF = "non-persistent://OPNX-V1/ME-WS/MD-DIFF-";
const std::string PULSAR_TOPIC_MD_BEST = "non-persistent://OPNX-V1/ME-WS/MD-BEST-";
const std::string PULSAR_TOPIC_CMD_IN = "persistent://OPNX-V1/PRETRADE-ME/CMD-IN";
const std::string PULSAR_TOPIC_CMD_OUT = "persistent://OPNX-V1/ME-POSTTRADE/CMD-OUT";
const std::string PULSAR_TOPIC_MARK_PRICE = "non-persistent://OPNX-V1/PRICE-SERVER/MARK-PRICE";
const std::string PULSAR_TOPIC_HEARTBEAT = "non-persistent://OPNX-V1/ME-POSTTRADE/HEARTBEAT";
const std::string PULSAR_TOPIC_LOG = "non-persistent://OPNX-V1/DATA-COLLECTOR/ME-LOG";


IMessage* createIMessage(OPNX::OrderQueue<OPNX::Order>* pOrderQueue,
                         OPNX::OrderQueue<OPNX::Order>* pTriggerOrderQueue,
                         OPNX::OrderQueue<OPNX::TriggerPrice>* pMarkPriceQueue,
                         OPNX::OrderQueue<nlohmann::json>* pCmdQueue,
                         const std::string& strServiceUrl){
    return PulsarProxy::createIMessage(pOrderQueue, pTriggerOrderQueue, pMarkPriceQueue, pCmdQueue, strServiceUrl);
};
IMessage* PulsarProxy::createIMessage(OPNX::OrderQueue<OPNX::Order>* pOrderQueue,
                                      OPNX::OrderQueue<OPNX::Order>* pTriggerOrderQueue,
                                      OPNX::OrderQueue<OPNX::TriggerPrice>* pMarkPriceQueue,
                                      OPNX::OrderQueue<nlohmann::json>* pCmdQueue,
                                      const std::string& strServiceUrl){
    return static_cast<IMessage*>(new PulsarProxy(pOrderQueue, pTriggerOrderQueue, pMarkPriceQueue, pCmdQueue, strServiceUrl));
};

void PulsarProxy::releaseIMessage() {
    delete this;
}
PulsarProxy::~PulsarProxy()
{
    m_bRunning = false;
    for (auto it = m_consumerThreadPool.begin(); m_consumerThreadPool.end() != it; it++)
    {
        it->join();
    }
    for (auto it = m_listConsumerListener.begin(); it != m_listConsumerListener.end(); it++)
    {
        pulsar_consumer_t* consumer = *it;
//        pulsar_consumer_unsubscribe(consumer);
        pulsar_consumer_close(consumer);
        pulsar_consumer_free(consumer);
    }
    m_listConsumerListener.clear();
    for (auto it = m_listConsumerConfiguration.begin(); it != m_listConsumerConfiguration.end(); it++)
    {
        pulsar_consumer_configuration_t* pulsarConsumerConfiguration = *it;
        pulsar_consumer_configuration_free(pulsarConsumerConfiguration);
    }
    m_listConsumerConfiguration.clear();
    for (auto it = m_mapProducer.begin(); m_mapProducer.end() != it; it++)
    {
        pulsar_producer_t *producer = it->second;
        pulsar_producer_close(producer);
        pulsar_producer_free(producer);
    }
    m_mapProducer.clear();
    for (auto it = m_mapProducerClient.begin(); m_mapProducerClient.end() != it; it++)
    {
        pulsar_client_t *pPulsarClient = it->second;
        pulsar_client_close(pPulsarClient);
        pulsar_client_free(pPulsarClient);
    }
    m_mapProducerClient.clear();
    pulsar_client_close(m_pPulsarClient);
    pulsar_client_free(m_pPulsarClient);
    pulsar_producer_configuration_free(m_pulsarProducerConfiguration);
    pulsar_client_configuration_free(m_pulsarClientConfiguration);
}
void PulsarProxy::consumerOrderThread(PulsarProxy* pulsarProxy, const std::string& strTopic, const std::string& strConsumerName)
{
    if (pulsarProxy)
    {
        pulsarProxy->consumerOrder(strTopic, strConsumerName);
    }
}
void PulsarProxy::consumerMarkPriceThread(PulsarProxy* pulsarProxy, const std::string& strTopic, const std::string& strConsumerName)
{
    if (pulsarProxy)
    {
        pulsarProxy->consumerMarkPrice(strTopic, strConsumerName);
    }
}
void PulsarProxy::consumerCmdThread(PulsarProxy* pulsarProxy, const std::string& strTopic, const std::string& strConsumerName)
{
    if (pulsarProxy)
    {
        pulsarProxy->consumerCmd(strTopic, strConsumerName);
    }
}
void PulsarProxy::consumerToLogThread(PulsarProxy* pulsarProxy, const std::string& strTopic, const std::string& strConsumerName)
{
    if (pulsarProxy)
    {
        pulsarProxy->consumerToLog(strTopic, strConsumerName);
    }
}
void PulsarProxy::consumerSubscribe(pulsar_client_t* pPulsarClient, pulsar_consumer_configuration_t* pConf, pulsar_consumer_t** consumer, const std::string& strTopic, const std::string& strConsumerName)
{
    if (nullptr != pPulsarClient)
    {
        pulsar_result result = pulsar_result_UnknownError;
        while (pulsar_result_Ok != result) {
            cfLog.info() << "Consumer subscribe Topic: " << strTopic << " subscriptionName: " << strConsumerName << " thread id: " << std::this_thread::get_id() << std::endl;

            result = pulsar_client_subscribe(pPulsarClient, strTopic.c_str(),
                                             strConsumerName.c_str(), pConf, consumer);
            if (pulsar_result_Ok != result) {
                cfLog.error() << "Failed to subscribe: " << strTopic << " result: " << result << std::endl;
                sleep(1);
            }
        }
        cfLog.info() << "Consumer subscribe Topic is ok " << std::endl;
    }
}
void PulsarProxy::consumerOrder(const std::string& strTopic, const std::string& strConsumerName)
{
    pulsar_client_configuration_t *pulsarClientConfiguration = pulsar_client_configuration_create();
    pulsar_client_t *pPulsarClient = pulsar_client_create(m_strServiceUrl.c_str(), pulsarClientConfiguration);
    pulsar_consumer_t *consumer = nullptr;
    pulsar_consumer_configuration_t *pulsarConsumerConfiguration = pulsar_consumer_configuration_create();
    pulsar_consumer_configuration_set_consumer_type(pulsarConsumerConfiguration, pulsar_ConsumerShared);
    // pulsar_consumer_configuration_set_receiver_queue_size(pulsarConsumerConfiguration, 1);
    pulsar_consumer_set_consumer_name(pulsarConsumerConfiguration, strConsumerName.c_str());
    std::vector<std::string> vecString;
    OPNX::Utils::stringSplit(vecString, strConsumerName, "_");
    std::string subscriptionName = vecString[0];
    consumerSubscribe(pPulsarClient, pulsarConsumerConfiguration, &consumer, strTopic, subscriptionName);
    m_conditionOrder.notify_one();

    while (m_bRunning)
    {
        std::string strJsonOrder = "";
        try {
            pulsar_message_t *pulsarMessage  = nullptr;
            pulsar_result res = pulsar_consumer_receive_with_timeout(consumer, &pulsarMessage, 1000);
            try {
                if (pulsar_result_Ok == res)
                {
                    strJsonOrder = (char*)pulsar_message_get_data(pulsarMessage);
                    uint32_t length = pulsar_message_get_length(pulsarMessage);
                    strJsonOrder = strJsonOrder.substr(0, length);
                    if (cfLog.enabledInfo())
                    {
                        unsigned long long receivedTimestamp = OPNX::Utils::getMilliTimestamp();
                        uint64_t publish_timestamp = pulsar_message_get_publish_timestamp(pulsarMessage);
                        long timeInterval = receivedTimestamp - publish_timestamp;
                        cfLog.info() << "ME in <-- " << m_strMarketCode << " publish_timestamp: " << publish_timestamp << " interval: " << timeInterval
                                     << " thread_id: " << std::this_thread::get_id()
                                     << " pulsar receive order: " << strJsonOrder << std::endl;
                    }

                    if (m_bIsRecovery)
                    {
                        if (std::string::npos != strJsonOrder.find("RECOVERY_END"))
                        {
                            m_bIsRecovery = false;
                            if (nullptr != pulsarMessage)
                            {
                                pulsar_consumer_acknowledge_async(consumer, pulsarMessage, [](pulsar_result res, void *ctx){
                                    if (nullptr != ctx)
                                    {
                                        pulsar_message_t *pulsarMessage  = (pulsar_message_t *)ctx;
                                        pulsar_message_free(pulsarMessage);
                                        pulsarMessage = nullptr;
                                    }
                                }, (void*)pulsarMessage);
                            }
                            continue;
                        }
                        if (std::string::npos == strJsonOrder.find("RECOVERY"))
                        {
                            std::string orderStatus = ",\"st\":\"REJECT_MATCHING_ENGINE_RECOVERING\"";
                            strJsonOrder.insert(strJsonOrder.length() - 1, orderStatus);
                            std::stringstream ssOrder;
                            ssOrder << "{\"pt\":\"Order\",\"ol\":[" << strJsonOrder << "]}";
                            sendOrder(ssOrder.str());

                            if (nullptr != pulsarMessage)
                            {
                                pulsar_consumer_acknowledge_async(consumer, pulsarMessage, [](pulsar_result res, void *ctx){
                                    if (nullptr != ctx)
                                    {
                                        pulsar_message_t *pulsarMessage  = (pulsar_message_t *)ctx;
                                        pulsar_message_free(pulsarMessage);
                                        pulsarMessage = nullptr;
                                    }
                                }, (void*)pulsarMessage);
                            }
                            continue;
                        }
                    }
                    rapidjson::Document document;
                    document.Parse(strJsonOrder.c_str());
                    OPNX::Order order;
                    OPNX::Order::rapidjsonToOrder(document, order);

                    if (checkOrder(order))
                    {
                        if ((!order.isTriggered && (OPNX::Order::STOP_LIMIT == order.type || OPNX::Order::STOP_MARKET == order.type
                             || OPNX::Order::TAKE_PROFIT_LIMIT == order.type || OPNX::Order::TAKE_PROFIT_MARKET == order.type))
                             || (OPNX::Order::CANCEL == order.action && 0 == order.orderId))
                        {
                            if (m_pTriggerOrderQueue)
                            {
                                m_pTriggerOrderQueue->push(order);
                            }
                        }
                        else
                        {
                            if (m_pOrderQueue)
                            {
                                m_pOrderQueue->push(order);
                            }
                        }
                    }

                } else if (pulsar_result_Timeout == res) {
                    continue;
                } else {
                    cfLog.error() << "Message not ResultOk, topic: " << strTopic << " res: " << pulsar_result_str(res) << "JsonOrder: " << strJsonOrder << std::endl;
                }
            } catch (...) {
                cfLog.fatal() << "Thread consumerOrder exception0, topic: " << strTopic << " ConsumerName: " << strConsumerName << "JsonOrder: " << strJsonOrder << std::endl;
            }
            if (nullptr != pulsarMessage)
            {
                pulsar_consumer_acknowledge_async(consumer, pulsarMessage, [](pulsar_result res, void *ctx){
                    if (nullptr != ctx)
                    {
                        pulsar_message_t *pulsarMessage  = (pulsar_message_t *)ctx;
                        pulsar_message_free(pulsarMessage);
                        pulsarMessage = nullptr;
                    }
                }, (void*)pulsarMessage);
            }
        } catch (...) {
           cfLog.fatal() << "Thread consumerOrder exception, topic: " << strTopic << " ConsumerName: " << strConsumerName << "JsonOrder: " << strJsonOrder << std::endl;
        }
    }
    pulsar_consumer_unsubscribe(consumer);
    pulsar_consumer_close(consumer);
    pulsar_consumer_free(consumer);
    pulsar_consumer_configuration_free(pulsarConsumerConfiguration);
    pulsar_client_close(pPulsarClient);
    pulsar_client_free(pPulsarClient);
    pulsar_client_configuration_free(pulsarClientConfiguration);

    cfLog.printInfo() << "consumerOrder exit " << strConsumerName << std::endl;

}
void PulsarProxy::consumerMarkPrice(const std::string& strTopic, const std::string& strConsumerName)
{
    pulsar_client_configuration_t *pulsarClientConfiguration = pulsar_client_configuration_create();
    pulsar_client_t *pPulsarClient = pulsar_client_create(m_strServiceUrl.c_str(), pulsarClientConfiguration);
    pulsar_consumer_t *consumer = nullptr;
    pulsar_consumer_configuration_t *pulsarConsumerConfiguration = pulsar_consumer_configuration_create();
    consumerSubscribe(pPulsarClient, pulsarConsumerConfiguration, &consumer, strTopic, strConsumerName);

    unsigned long long marketId = strtoll(m_strMarketId.c_str(), nullptr, 10);
    long long llMarkPrice = 0;
    while (m_bRunning)
    {
        try {
            pulsar_message_t *pulsarMessage = nullptr;
            pulsar_result res = pulsar_consumer_receive_with_timeout(consumer, &pulsarMessage, 1000);
            if (pulsar_result_Ok == res && !m_bIsRecovery)
            {
                std::string strJsonOrder = (char*)pulsar_message_get_data(pulsarMessage);
                uint32_t length = pulsar_message_get_length(pulsarMessage);
                strJsonOrder = strJsonOrder.substr(0, length);
                rapidjson::Document document;
                document.Parse(strJsonOrder.c_str());
                std::string strMarketCode = "";
                auto it = document.FindMember("marketCode");
                if (document.MemberEnd() != it)
                {
                    strMarketCode = it->value.GetString();
                }
                it = document.FindMember("markPrice");
                if (document.MemberEnd() != it && strMarketCode == m_strMarketCode)
                {
                    double dMarkPrice = it->value.GetDouble();
                    long long price = OPNX::Utils::double2int(dMarkPrice * m_factor);
                    if (m_pMarkPriceQueue && price != llMarkPrice && !m_bIsRecovery)
                    {
                        cfLog.debug() << "ME in <-- pulsar receive markPrice: " << strJsonOrder << std::endl;
                        OPNX::TriggerPrice markPrice;
                        markPrice.price = price;
                        markPrice.marketId = marketId;
                        llMarkPrice = price;
                        m_pMarkPriceQueue->push(markPrice);
                    }
                }

            } else if (pulsar_result_Timeout != res) {
                cfLog.error() << "Message not ResultOk, topic: " << strTopic << " res: " << res << pulsar_result_str(res) << std::endl;
            }
            if (nullptr != pulsarMessage)
            {
                pulsar_consumer_acknowledge_async(consumer, pulsarMessage, [](pulsar_result res, void *ctx){
                    if (nullptr != ctx)
                    {
                        pulsar_message_t *pulsarMessage  = (pulsar_message_t *)ctx;
                        pulsar_message_free(pulsarMessage);
                        pulsarMessage = nullptr;
                    }
                }, (void*)pulsarMessage);
            }
        } catch (...) {
            cfLog.fatal() << "Thread consumerMarkPrice exception, topic: " << strTopic << " ConsumerName: " << strConsumerName << std::endl;
        }
    }
    pulsar_consumer_unsubscribe(consumer);
    pulsar_consumer_close(consumer);
    pulsar_consumer_free(consumer);
    pulsar_consumer_configuration_free(pulsarConsumerConfiguration);
    pulsar_client_close(pPulsarClient);
    pulsar_client_free(pPulsarClient);
    pulsar_client_configuration_free(pulsarClientConfiguration);

    cfLog.printInfo() << "consumerMarkPrice exit " << strConsumerName << std::endl;

}
void PulsarProxy::consumerCmd(const std::string& strTopic, const std::string& strConsumerName)
{
    pulsar_client_configuration_t *pulsarClientConfiguration = pulsar_client_configuration_create();
    pulsar_client_t *pPulsarClient = pulsar_client_create(m_strServiceUrl.c_str(), pulsarClientConfiguration);
    pulsar_consumer_t *consumer = nullptr;
    pulsar_consumer_configuration_t *pulsarConsumerConfiguration = pulsar_consumer_configuration_create();
    consumerSubscribe(pPulsarClient, pulsarConsumerConfiguration, &consumer, strTopic, strConsumerName);
    m_conditionCmd.notify_one();

    while (m_bRunning)
    {
        try {
            pulsar_message_t *pulsarMessage = nullptr;
            pulsar_result res = pulsar_consumer_receive_with_timeout(consumer, &pulsarMessage, 1000);
            if (pulsar_result_Ok == res)
            {
                std::string strJsonCmd = (char*)pulsar_message_get_data(pulsarMessage);
                uint32_t length = pulsar_message_get_length(pulsarMessage);
                strJsonCmd = strJsonCmd.substr(0, length);
                cfLog.info() << "ME in <-- pulsar receive cmd: " << strJsonCmd << std::endl;
                nlohmann::json jsonCmd = nlohmann::json::parse(strJsonCmd);
                std::string strPair = "";
                OPNX::Utils::getJsonValue(strPair, jsonCmd, "pair");
                std::string strPair2 = strPair;
                auto pos = strPair2.find("/");
                if (std::string::npos != pos)
                {
                    strPair2.replace(pos, 1,  "-");
                }
                if (m_pCmdQueue && ("" == strPair || strPair == m_strReferencePair || strPair2 == m_strReferencePair))
                {
                    m_pCmdQueue->push(jsonCmd);
                }
            } else if (pulsar_result_Timeout != res) {
                cfLog.error() << "Message not ResultOk, topic: " << strTopic << " res: " << res << pulsar_result_str(res) << std::endl;
            }
            if (nullptr != pulsarMessage)
            {
                pulsar_consumer_acknowledge_async(consumer, pulsarMessage, [](pulsar_result res, void *ctx){
                    if (nullptr != ctx)
                    {
                        pulsar_message_t *pulsarMessage  = (pulsar_message_t *)ctx;
                        pulsar_message_free(pulsarMessage);
                        pulsarMessage = nullptr;
                    }
                }, (void*)pulsarMessage);
            }
        } catch (...) {
            cfLog.fatal() << "Thread consumerCmd exception, topic: " << strTopic << " ConsumerName: " << strConsumerName << std::endl;
        }
    }
    pulsar_consumer_unsubscribe(consumer);
    pulsar_consumer_close(consumer);
    pulsar_consumer_free(consumer);
    pulsar_consumer_configuration_free(pulsarConsumerConfiguration);
    pulsar_client_close(pPulsarClient);
    pulsar_client_free(pPulsarClient);
    pulsar_client_configuration_free(pulsarClientConfiguration);

    cfLog.printInfo() << "consumerCmd exit " << strConsumerName << std::endl;

}
void PulsarProxy::consumerToLog(const std::string& strTopic, const std::string& strConsumerName)
{
    pulsar_client_configuration_t *pulsarClientConfiguration = pulsar_client_configuration_create();
    pulsar_client_t *pPulsarClient = pulsar_client_create(m_strServiceUrl.c_str(), pulsarClientConfiguration);
    pulsar_consumer_t *consumer = nullptr;
    pulsar_consumer_configuration_t *pulsarConsumerConfiguration = pulsar_consumer_configuration_create();
    consumerSubscribe(pPulsarClient, pulsarConsumerConfiguration, &consumer, strTopic, strConsumerName);

    while (m_bRunning)
    {
        try {
            pulsar_message_t *pulsarMessage = nullptr;
            pulsar_result res = pulsar_consumer_receive_with_timeout(consumer, &pulsarMessage, 1000);
            if (pulsar_result_Ok == res)
            {
                std::string strJsonCmd = (char*)pulsar_message_get_data(pulsarMessage);
                uint32_t length = pulsar_message_get_length(pulsarMessage);
                strJsonCmd = strJsonCmd.substr(0, length);
                cfLog.info() << "ME in <-- pulsar receive: " << strJsonCmd << std::endl;

            } else if (pulsar_result_Timeout != res) {
                cfLog.error() << "Message not ResultOk, topic: " << strTopic << " res: " << res << pulsar_result_str(res) << std::endl;
            }
            if (nullptr != pulsarMessage)
            {
                pulsar_consumer_acknowledge(consumer, pulsarMessage);
                pulsar_message_free(pulsarMessage);
                pulsarMessage = nullptr;
            }
        } catch (...) {
            cfLog.fatal() << "Thread consumerToLog exception, topic: " << strTopic << std::endl;
        }
    }
    pulsar_consumer_unsubscribe(consumer);
    pulsar_consumer_free(consumer);
    pulsar_consumer_configuration_free(pulsarConsumerConfiguration);
    pulsar_client_close(pPulsarClient);
    pulsar_client_free(pPulsarClient);
    pulsar_client_configuration_free(pulsarClientConfiguration);

    cfLog.printInfo() << "consumerToLog exit " << strConsumerName << std::endl;

}

void PulsarProxy::createConsumer(PulsarProxy::ProxyType proxyType, const std::string& strTopic, const std::string& strConsumerName)
{
    switch (proxyType) {
        case ORDER:
        {
            std::thread orderConsumer(PulsarProxy::consumerOrderThread, this, strTopic, strConsumerName);
            m_consumerThreadPool.push_back(std::move(orderConsumer));
            std::mutex mtx;
            std::unique_lock<std::mutex> lck(mtx);
            m_conditionOrder.wait(lck);
            break;
        }
        case MARK_PRICE:
        {
            std::thread markPriceConsumer(PulsarProxy::consumerMarkPriceThread, this, strTopic, strConsumerName);
            m_consumerThreadPool.push_back(std::move(markPriceConsumer));
            break;
        }
        case COMMAND:
        {
            std::thread cmdConsumer(PulsarProxy::consumerCmdThread, this, strTopic, strConsumerName);
            m_consumerThreadPool.push_back(std::move(cmdConsumer));
            std::mutex mtx;
            std::unique_lock<std::mutex> lck(mtx);
            m_conditionCmd.wait(lck);
            break;
        }
        case CONSUMER_TO_LOG:
        {
            std::thread consumerToLog(PulsarProxy::consumerToLogThread, this, strTopic, strConsumerName);
            m_consumerThreadPool.push_back(std::move(consumerToLog));
            break;
        }
        default:
            break;
    }
}

void PulsarProxy::consumerListenerOrder(const std::string& strTopic, const std::string& strConsumerName)
{
    pulsar_consumer_t *consumer = nullptr;
    pulsar_consumer_configuration_t *pulsarConsumerConfiguration = pulsar_consumer_configuration_create();
    pulsar_consumer_configuration_set_consumer_type(pulsarConsumerConfiguration, pulsar_ConsumerShared);
    // pulsar_consumer_configuration_set_receiver_queue_size(pulsarConsumerConfiguration, 1);
    pulsar_consumer_set_consumer_name(pulsarConsumerConfiguration, strConsumerName.c_str());

    pulsar_consumer_configuration_set_message_listener(pulsarConsumerConfiguration, [](pulsar_consumer_t *consumer, pulsar_message_t *msg, void *ctx) {

        std::string strJsonOrder = "";
//        std::string strTopic = pulsar_consumer_get_topic(consumer);
        PulsarProxy* pPulsarProxy = (PulsarProxy*) ctx;
        try {
            pulsar_message_t *pulsarMessage  = msg;
            try {
                if (nullptr != pPulsarProxy)
                {
                    strJsonOrder = (char*)pulsar_message_get_data(pulsarMessage);
                    uint32_t length = pulsar_message_get_length(pulsarMessage);
                    strJsonOrder = strJsonOrder.substr(0, length);
                    if (cfLog.enabledInfo())
                    {
                        unsigned long long receivedTimestamp = OPNX::Utils::getMilliTimestamp();
                        uint64_t publish_timestamp = pulsar_message_get_publish_timestamp(pulsarMessage);
                        long timeInterval = receivedTimestamp - publish_timestamp;
                        cfLog.info() << "ME in <-- " << pPulsarProxy->m_strMarketCode << " publish_timestamp: " << publish_timestamp << " interval: " << timeInterval
                                     << " thread_id: " << std::this_thread::get_id()
                                     << " pulsar receive order: " << strJsonOrder << std::endl;
                    }

                    if (pPulsarProxy->m_bIsRecovery)
                    {
                        if (std::string::npos != strJsonOrder.find("RECOVERY_END"))
                        {
                            pPulsarProxy->m_bIsRecovery = false;
                            if (nullptr != pulsarMessage)
                            {
                                pulsar_consumer_acknowledge_async(consumer, pulsarMessage, [](pulsar_result res, void *ctx){
                                    if (nullptr != ctx)
                                    {
                                        pulsar_message_t *pulsarMessage  = (pulsar_message_t *)ctx;
                                        pulsar_message_free(pulsarMessage);
                                        pulsarMessage = nullptr;
                                    }
                                }, (void*)pulsarMessage);
                            }
                            return ;
                        }
                        if (std::string::npos == strJsonOrder.find("RECOVERY"))
                        {
                            std::string orderStatus = ",\"st\":\"REJECT_MATCHING_ENGINE_RECOVERING\"";
                            strJsonOrder.insert(strJsonOrder.length() - 1, orderStatus);
                            std::stringstream ssOrder;
                            ssOrder << "{\"pt\":\"Order\",\"ol\":[" << strJsonOrder << "]}";
                            pPulsarProxy->sendOrder(ssOrder.str());

                            if (nullptr != pulsarMessage)
                            {
                                pulsar_consumer_acknowledge_async(consumer, pulsarMessage, [](pulsar_result res, void *ctx){
                                    if (nullptr != ctx)
                                    {
                                        pulsar_message_t *pulsarMessage  = (pulsar_message_t *)ctx;
                                        pulsar_message_free(pulsarMessage);
                                        pulsarMessage = nullptr;
                                    }
                                }, (void*)pulsarMessage);
                            }
                            return;
                        }
                    }
                    rapidjson::Document document;
                    document.Parse(strJsonOrder.c_str());
                    OPNX::Order order;
                    OPNX::Order::rapidjsonToOrder(document, order);

                    if (pPulsarProxy->checkOrder(order))
                    {
                        if ((!order.isTriggered && (OPNX::Order::STOP_LIMIT == order.type || OPNX::Order::STOP_MARKET == order.type
                                                    || OPNX::Order::TAKE_PROFIT_LIMIT == order.type || OPNX::Order::TAKE_PROFIT_MARKET == order.type))
                            || (OPNX::Order::CANCEL == order.action && 0 == order.orderId))
                        {
                            if (pPulsarProxy->m_pTriggerOrderQueue)
                            {
                                pPulsarProxy->m_pTriggerOrderQueue->push(order);
                            }
                        }
                        else
                        {
                            if (pPulsarProxy->m_pOrderQueue)
                            {
                                pPulsarProxy->m_pOrderQueue->push(order);
                            }
                        }
                    }

                } else {
                    cfLog.error() << "pPulsarProxy is null !!!: " << std::endl;
                }
            } catch (...) {
                cfLog.fatal() << "Listener consumerOrder exception0, MarketCode: " << pPulsarProxy->m_strMarketCode << "JsonOrder: " << strJsonOrder << std::endl;
            }
            if (nullptr != pulsarMessage)
            {
                pulsar_consumer_acknowledge_async(consumer, pulsarMessage, [](pulsar_result res, void *ctx){
                    if (nullptr != ctx)
                    {
                        pulsar_message_t *pulsarMessage  = (pulsar_message_t *)ctx;
                        pulsar_message_free(pulsarMessage);
                        pulsarMessage = nullptr;
                    }
                }, (void*)pulsarMessage);
            }
        } catch (...) {
            cfLog.fatal() << "Listener consumerOrder exception, MarketCode: " << pPulsarProxy->m_strMarketCode << "JsonOrder: " << strJsonOrder << std::endl;
        }
    }, (void*)this);
    std::vector<std::string> vecString;
    OPNX::Utils::stringSplit(vecString, strConsumerName, "_");
    std::string subscriptionName = vecString[0];
    consumerSubscribe(m_pPulsarClient, pulsarConsumerConfiguration, &consumer, strTopic, subscriptionName);

    m_listConsumerListener.push_back(std::move(consumer));
    m_listConsumerConfiguration.push_back(std::move(pulsarConsumerConfiguration));
}
void PulsarProxy::consumerListenerMarkPrice(const std::string& strTopic, const std::string& strConsumerName)
{
    pulsar_consumer_t *consumer = nullptr;
    pulsar_consumer_configuration_t *pulsarConsumerConfiguration = pulsar_consumer_configuration_create();

    pulsar_consumer_configuration_set_message_listener(pulsarConsumerConfiguration, [](pulsar_consumer_t *consumer, pulsar_message_t *msg, void *ctx) {

//        std::string strTopic = pulsar_consumer_get_topic(consumer);
        PulsarProxy* pPulsarProxy = (PulsarProxy*) ctx;
        try {
            pulsar_message_t *pulsarMessage = msg;
            unsigned long long marketId = strtoll(pPulsarProxy->m_strMarketId.c_str(), nullptr, 10);

            if (nullptr != pPulsarProxy && !pPulsarProxy->m_bIsRecovery)
            {
                std::string strJsonOrder = (char*)pulsar_message_get_data(pulsarMessage);
                uint32_t length = pulsar_message_get_length(pulsarMessage);
                strJsonOrder = strJsonOrder.substr(0, length);
                rapidjson::Document document;
                document.Parse(strJsonOrder.c_str());
                std::string strMarketCode = "";
                auto it = document.FindMember("marketCode");
                if (document.MemberEnd() != it)
                {
                    strMarketCode = it->value.GetString();
                }
                it = document.FindMember("markPrice");
                if (document.MemberEnd() != it && strMarketCode == pPulsarProxy->m_strMarketCode)
                {
                    double dMarkPrice = it->value.GetDouble();
                    long long price = OPNX::Utils::double2int(dMarkPrice * pPulsarProxy->m_factor);
                    if (pPulsarProxy->m_pMarkPriceQueue && price != pPulsarProxy->m_llMarkPrice && !pPulsarProxy->m_bIsRecovery)
                    {
                        cfLog.debug() << "ME in <-- pulsar receive markPrice: " << strJsonOrder << std::endl;
                        OPNX::TriggerPrice markPrice;
                        markPrice.price = price;
                        markPrice.marketId = marketId;
                        pPulsarProxy->m_llMarkPrice = price;
                        pPulsarProxy->m_pMarkPriceQueue->push(markPrice);
                    }
                }

            }
            if (nullptr != pulsarMessage)
            {
                pulsar_consumer_acknowledge_async(consumer, pulsarMessage, [](pulsar_result res, void *ctx){
                    if (nullptr != ctx)
                    {
                        pulsar_message_t *pulsarMessage  = (pulsar_message_t *)ctx;
                        pulsar_message_free(pulsarMessage);
                        pulsarMessage = nullptr;
                    }
                }, (void*)pulsarMessage);
            }
        } catch (...) {
            cfLog.fatal() << "Listener consumerMarkPrice exception, MarketCode: " << pPulsarProxy->m_strMarketCode << std::endl;
        }
    }, (void*)this);
    consumerSubscribe(m_pPulsarClient, pulsarConsumerConfiguration, &consumer, strTopic, strConsumerName);

    m_listConsumerListener.push_back(std::move(consumer));
    m_listConsumerConfiguration.push_back(std::move(pulsarConsumerConfiguration));
}
void PulsarProxy::consumerListenerCmd(const std::string& strTopic, const std::string& strConsumerName)
{
    pulsar_consumer_t *consumer = nullptr;
    pulsar_consumer_configuration_t *pulsarConsumerConfiguration = pulsar_consumer_configuration_create();

    pulsar_consumer_configuration_set_message_listener(pulsarConsumerConfiguration, [](pulsar_consumer_t *consumer, pulsar_message_t *msg, void *ctx) {

//        std::string strTopic = pulsar_consumer_get_topic(consumer);
        PulsarProxy* pPulsarProxy = (PulsarProxy*) ctx;
        try {
            pulsar_message_t *pulsarMessage = msg;
            if (nullptr != pPulsarProxy)
            {
                std::string strJsonCmd = (char*)pulsar_message_get_data(pulsarMessage);
                uint32_t length = pulsar_message_get_length(pulsarMessage);
                strJsonCmd = strJsonCmd.substr(0, length);
                cfLog.info() << "ME in <-- pulsar receive cmd: " << strJsonCmd << std::endl;
                nlohmann::json jsonCmd = nlohmann::json::parse(strJsonCmd);
                std::string strPair = "";
                OPNX::Utils::getJsonValue(strPair, jsonCmd, "pair");
                std::string strPair2 = strPair;
                auto pos = strPair2.find("/");
                if (std::string::npos != pos)
                {
                    strPair2.replace(pos, 1,  "-");
                }
                if (pPulsarProxy->m_pCmdQueue && ("" == strPair || strPair == pPulsarProxy->m_strReferencePair || strPair2 == pPulsarProxy->m_strReferencePair))
                {
                    pPulsarProxy->m_pCmdQueue->push(jsonCmd);
                }
            }
            if (nullptr != pulsarMessage)
            {
                pulsar_consumer_acknowledge_async(consumer, pulsarMessage, [](pulsar_result res, void *ctx){
                    if (nullptr != ctx)
                    {
                        pulsar_message_t *pulsarMessage  = (pulsar_message_t *)ctx;
                        pulsar_message_free(pulsarMessage);
                        pulsarMessage = nullptr;
                    }
                }, (void*)pulsarMessage);
            }
        } catch (...) {
            cfLog.fatal() << "Listener consumerCmd exception, MarketCode: " << pPulsarProxy->m_strMarketCode << std::endl;
        }
    }, (void*)this);

    consumerSubscribe(m_pPulsarClient, pulsarConsumerConfiguration, &consumer, strTopic, strConsumerName);

    m_listConsumerListener.push_back(std::move(consumer));
    m_listConsumerConfiguration.push_back(std::move(pulsarConsumerConfiguration));
}
void PulsarProxy::consumerListenerToLog(const std::string& strTopic, const std::string& strConsumerName)
{
    pulsar_consumer_t *consumer = nullptr;
    pulsar_consumer_configuration_t *pulsarConsumerConfiguration = pulsar_consumer_configuration_create();

    pulsar_consumer_configuration_set_message_listener(pulsarConsumerConfiguration, [](pulsar_consumer_t *consumer, pulsar_message_t *msg, void *ctx) {

//        std::string strTopic = pulsar_consumer_get_topic(consumer);
        PulsarProxy* pPulsarProxy = (PulsarProxy*) ctx;
        try {
            pulsar_message_t *pulsarMessage = msg;
            if (nullptr != pPulsarProxy)
            {
                std::string strJsonCmd = (char*)pulsar_message_get_data(pulsarMessage);
                uint32_t length = pulsar_message_get_length(pulsarMessage);
                strJsonCmd = strJsonCmd.substr(0, length);
                cfLog.info() << "ME in <-- pulsar receive: " << strJsonCmd << std::endl;

            }
            if (nullptr != pulsarMessage)
            {
                pulsar_consumer_acknowledge(consumer, pulsarMessage);
                pulsar_message_free(pulsarMessage);
                pulsarMessage = nullptr;
            }
        } catch (...) {
            cfLog.fatal() << "Listener consumerToLog exception, MarketCode: " << pPulsarProxy->m_strMarketCode << std::endl;
        }
    }, (void*)this);

    consumerSubscribe(m_pPulsarClient, pulsarConsumerConfiguration, &consumer, strTopic, strConsumerName);

    m_listConsumerListener.push_back(std::move(consumer));
    m_listConsumerConfiguration.push_back(std::move(pulsarConsumerConfiguration));
}

void PulsarProxy::createConsumerListener(PulsarProxy::ProxyType proxyType, const std::string& strTopic, const std::string& strConsumerName)
{
    switch (proxyType) {
        case ORDER:
        {
            consumerListenerOrder(strTopic, strConsumerName);
            break;
        }
        case MARK_PRICE:
        {
            consumerListenerMarkPrice(strTopic, strConsumerName);
            break;
        }
        case COMMAND:
        {
            consumerListenerCmd(strTopic, strConsumerName);
            break;
        }
        case CONSUMER_TO_LOG:
        {
            consumerListenerToLog(strTopic, strConsumerName);
            break;
        }
        default:
            break;
    }
}

pulsar_producer_t* PulsarProxy::createProducer(PulsarProxy::ProxyType proxyType, const std::string& strTopic, const std::string& strProducerName)
{
    OPNX::CAutoMutex autoMutex(m_spinMutexProducer);
    pulsar_producer_t *producer = nullptr;
    try {
//        std::lock_guard<std::mutex> lock(m_spinMutexProducer);

        pulsar_producer_configuration_set_batching_enabled(m_pulsarProducerConfiguration, false);
        pulsar_producer_configuration_set_block_if_queue_full(m_pulsarProducerConfiguration, true);
        pulsar_producer_configuration_set_producer_name(m_pulsarProducerConfiguration, strProducerName.c_str());
        pulsar_client_t *pPulsarClient = pulsar_client_create(m_strServiceUrl.c_str(), m_pulsarClientConfiguration);


        pulsar_result result = pulsar_result_UnknownError;
        while (pulsar_result_Ok != result) {
            cfLog.info() << "Creating Producer: " << strTopic << std::endl;
            result = pulsar_client_create_producer(pPulsarClient, strTopic.c_str(), m_pulsarProducerConfiguration, &producer);
            if (pulsar_result_Ok != result) {
                cfLog.error() << "Failed creating producer: " << result << std::endl;
                sleep(1);
            }
        }
        m_mapProducerClient.insert(std::pair<ProxyType, pulsar_client_t *>(proxyType, pPulsarClient));
        m_mapProducer.insert(std::pair<ProxyType, pulsar_producer_t *>(proxyType, producer));
    } catch (...) {
        cfLog.fatal() << "createProducer exception, topic: " << strTopic << std::endl;
    }
    return producer;
}
void PulsarProxy::createMarketAll(const nlohmann::json& jsonConfig)
{
    setMarketInfo(jsonConfig);

    if (nullptr == m_pPulsarClient)
    {
//        pulsar_client_configuration_set_io_threads(m_pulsarClientConfiguration, 10);
        pulsar_client_configuration_set_message_listener_threads(m_pulsarClientConfiguration, 30);
        m_pPulsarClient = pulsar_client_create(m_strServiceUrl.c_str(), m_pulsarClientConfiguration);
    }

    // order producer
    std::string strTopic = PULSAR_TOPIC_ORDER_OUT + m_strMarketId;
    m_pOrderProducer = createProducer(ORDER, strTopic, "ME-ORDER-OUT-" + m_strReferencePair + "-" + m_strMarketId);
    m_pOrdersProducer = createProducer(ORDERS, strTopic, "ME-ORDERS-OUT-" + m_strReferencePair + "-" + m_strMarketId);
    // MARKET_SNAPSHOT producer
    strTopic = PULSAR_TOPIC_MD_SNAPSHOT + m_strMarketId;
    m_pBookSnapshotProducer = createProducer(MARKET_SNAPSHOT, strTopic, "MD-SNAPSHOT-" + m_strReferencePair + "-" + m_strMarketId);
    // MARKET_DIFF producer
    strTopic = PULSAR_TOPIC_MD_DIFF + m_strMarketId;
    m_pBookDiffProducer = createProducer(MARKET_DIFF, strTopic, "MD-DIFF-" + m_strReferencePair + "-" + m_strMarketId);
    // MARKET_BEST producer
    strTopic = PULSAR_TOPIC_MD_BEST + m_strMarketId;
    m_pBookBestProducer = createProducer(MARKET_BEST, strTopic, "MD-BEST-" + m_strReferencePair + "-" + m_strMarketId);

    // mark price consumer
    strTopic = PULSAR_TOPIC_MARK_PRICE;
    std::string strConsumerName = "pulsar-proxy-mark-price-" + m_strMarketId;
    createConsumerListener(MARK_PRICE, strTopic, strConsumerName);

    // order consumer
    strTopic = PULSAR_TOPIC_ORDER_IN + m_strMarketId;
    strConsumerName = "pulsar-proxy-" + m_strMarketId;
    createConsumerListener(ORDER, strTopic, strConsumerName);
    if (1 < m_iOrderConsumerCount)
    {
        for (int i = 1; i < m_iOrderConsumerCount; i++)
        {
            strConsumerName = "pulsar-proxy-" + m_strMarketId + "_" + std::to_string(i);
            createConsumerListener(ORDER, strTopic, strConsumerName);
        }
    }
}
void PulsarProxy::createMarketAllTest(const nlohmann::json& jsonConfig)
{
    setMarketInfo(jsonConfig);

    if (nullptr == m_pPulsarClient)
    {
//        pulsar_client_configuration_set_io_threads(m_pulsarClientConfiguration, 10);
        pulsar_client_configuration_set_message_listener_threads(m_pulsarClientConfiguration, 30);
        m_pPulsarClient = pulsar_client_create(m_strServiceUrl.c_str(), m_pulsarClientConfiguration);
    }
    // order producer
    std::string strTopic = PULSAR_TOPIC_ORDER_IN + m_strMarketId;
    m_pOrderProducer = createProducer(ORDER, strTopic, "ME-TEST-ORDER-IN-" + m_strReferencePair + "-" + m_strMarketId);
    // MARK_PRICE producer
    strTopic = PULSAR_TOPIC_MARK_PRICE + m_strMarketId;
    createProducer(MARK_PRICE, strTopic, "ME-TEST-MARK-PRICE-" + m_strReferencePair + "-" + m_strMarketId);

    // order consumer
    strTopic = PULSAR_TOPIC_ORDER_OUT + m_strMarketId;
    std::string strConsumerName = "test-pulsar-proxy-" + m_strMarketId;
    createConsumerListener(CONSUMER_TO_LOG, strTopic, strConsumerName);
    // MARKET_SNAPSHOT consumer
    strTopic = PULSAR_TOPIC_MD_SNAPSHOT + m_strMarketId;
    strConsumerName = "test-pulsar-proxy-snapshot-" + m_strMarketId;
    createConsumerListener(CONSUMER_TO_LOG, strTopic, strConsumerName);
    // MARKET_DIFF consumer
    strTopic = PULSAR_TOPIC_MD_DIFF + m_strMarketId;
    strConsumerName = "test-pulsar-proxy-diff-" + m_strMarketId;
    createConsumerListener(CONSUMER_TO_LOG, strTopic, strConsumerName);
    // MARKET_BEST consumer
    strTopic = PULSAR_TOPIC_MD_BEST + m_strMarketId;
    strConsumerName = "test-pulsar-proxy-best-" + m_strMarketId;
    createConsumerListener(CONSUMER_TO_LOG, strTopic, strConsumerName);

}
void PulsarProxy::createCommander(const std::string& strReferencePair)
{
    if (nullptr == m_pPulsarClient)
    {
//        pulsar_client_configuration_set_message_listener_threads(m_pulsarClientConfiguration, 5);
        m_pPulsarClient = pulsar_client_create(m_strServiceUrl.c_str(), m_pulsarClientConfiguration);
    }

    m_strReferencePair = strReferencePair;

    // cmd producer
    std::string strTopic = PULSAR_TOPIC_CMD_OUT;
    m_pCmdProducer = createProducer(COMMAND, strTopic, "ME-CMD-OUT-" + m_strReferencePair);
    // cmd consumer
    strTopic = PULSAR_TOPIC_CMD_IN;
    std::string strConsumerName = "pulsar-proxy-cmd-" + m_strReferencePair;
    createConsumerListener(COMMAND, strTopic, strConsumerName);

    // m_pLogProducer
    strTopic = PULSAR_TOPIC_LOG;  // + strReferencePair;
    m_pLogProducer = createProducer(HEARTBEAT, strTopic, "ME-PulsarLog-" + m_strReferencePair);
    // cmd heartbeat
    strTopic = PULSAR_TOPIC_HEARTBEAT;
    m_pHeartbeatProducer = createProducer(HEARTBEAT, strTopic,"ME-HEARTBEAT-" + m_strReferencePair);

}
void PulsarProxy::createTestCommander(const std::string& strReferencePair)
{
    if (nullptr == m_pPulsarClient)
    {
//        pulsar_client_configuration_set_message_listener_threads(m_pulsarClientConfiguration, 5);
        m_pPulsarClient = pulsar_client_create(m_strServiceUrl.c_str(), m_pulsarClientConfiguration);
    }
    m_strReferencePair = strReferencePair;

    // cmd producer
    std::string strTopic = PULSAR_TOPIC_CMD_IN;
    m_pCmdProducer = createProducer(COMMAND, strTopic, "ME-CMD-IN-" + m_strReferencePair);
    // cmd consumer
    strTopic = PULSAR_TOPIC_CMD_OUT;
    std::string strConsumerName = "test-pulsar-proxy-cmd-" + m_strReferencePair;
    createConsumerListener(COMMAND, strTopic, strConsumerName);
}
void PulsarProxy::sendOrder(const std::string& strJsonData)
{
    m_spinMutexSendOrder.lock();
    if (nullptr != m_pOrderProducer)
    {
        cfLog.info() << "ME out --> thread_id: " << std::this_thread::get_id() <<", order: " << strJsonData << std::endl;
        sendMsg(m_pOrderProducer, strJsonData);
    } else {
        cfLog.error() << "PulsarProxy::sendOrder ProxyType not exting";
    }
    m_spinMutexSendOrder.unlock();
}
void PulsarProxy::sendOrders(const std::string& strJsonData)
{
    m_spinMutexSendOrders.lock();
    if (nullptr != m_pOrdersProducer)
    {
        cfLog.info() << "ME out --> thread_id: " << std::this_thread::get_id() << ", orders: " << strJsonData << std::endl;
        sendMsg(m_pOrdersProducer, strJsonData);
    } else {
        cfLog.error() << "PulsarProxy::sendOrder ProxyType not exting";
    }
    m_spinMutexSendOrders.unlock();
}
void PulsarProxy::sendOrderBookSnapshot(const nlohmann::json& jsonData)
{
    if (!m_bIsRecovery && nullptr != m_pBookSnapshotProducer)
    {
        std::string strJsonData = jsonData.dump();
        cfLog.trace() << "MD Snapshot out --> : " << strJsonData << std::endl;
        sendMsg(m_pBookSnapshotProducer, strJsonData);
    } else {
        cfLog.error() << "PulsarProxy::sendOrderBookSnapshot ProxyType not exting";
    }
}
void PulsarProxy::sendOrderBookDiff(const nlohmann::json& jsonData)
{
    if (!m_bIsRecovery && nullptr != m_pBookDiffProducer)
    {
        std::string strJsonData = jsonData.dump();
        cfLog.trace() << "MD Diff out --> " << strJsonData << std::endl;
        sendMsg(m_pBookDiffProducer, strJsonData);
    } else {
        cfLog.error() << "PulsarProxy::sendOrderBookDiff ProxyType not exting";
    }
}
void PulsarProxy::sendOrderBookBest(const nlohmann::json& jsonData)
{
    if (!m_bIsRecovery && nullptr != m_pBookBestProducer)
    {
        std::string strJsonData = jsonData.dump();
        cfLog.info() << "MD Best out --> " << strJsonData << std::endl;
        sendMsg(m_pBookBestProducer, strJsonData);
    } else {
        cfLog.error() << "PulsarProxy::sendOrderBookBest ProxyType not exting";
    }
}
void PulsarProxy::sendCmd(const nlohmann::json& jsonData)
{
    if (nullptr != m_pCmdProducer)
    {
        std::string strJsonData = jsonData.dump();
        std::string strAction = "";
        OPNX::Utils::getJsonValue<std::string>(strAction, jsonData, "action");
        cfLog.info() << "ME out --> CMD: " << strJsonData << std::endl;
        sendMsg(m_pCmdProducer, strJsonData, "action", strAction);
    } else {
        cfLog.error() << "PulsarProxy::sendCmd m_pCmdProducer is nullptr";
    }
}


void PulsarProxy::sendHeartbeat(const nlohmann::json& jsonData)
{
    if (nullptr != m_pHeartbeatProducer)
    {
        std::string strJsonData = jsonData.dump();
        std::string strAction = "";
        OPNX::Utils::getJsonValue<std::string>(strAction, jsonData, "action");
        cfLog.trace() << "ME out --> heartbeat: " << strJsonData << std::endl;
        sendMsg(m_pHeartbeatProducer, strJsonData, "action", strAction);
    } else {
        cfLog.error() << "PulsarProxy::sendHeartbeat m_pHeartbeatProducer is nullptr";
    }
}

void PulsarProxy::sendPulsarLog(const std::string& strLog)
{
    if (nullptr != m_pLogProducer)
    {
        cfLog.trace() << "ME out --> CallBackPulsarLog: " << strLog << std::endl;
        sendMsg(m_pLogProducer, strLog, "market", m_strReferencePair);
    } else {
        cfLog.error() << "PulsarProxy::sendPulsarLog m_pLogProducer is nullptr";
    }
}

void PulsarProxy::sendMsg(pulsar_producer_t *producer, const std::string& strJsonData, const std::string& strPropertyName, const std::string& strPropertyValue)
{
    pulsar_message_t *pulsarMessage = pulsar_message_create();
    if (nullptr != pulsarMessage)
    {
        pulsar_message_set_property(pulsarMessage, strPropertyName.c_str(), strPropertyValue.c_str());
        pulsar_message_set_content(pulsarMessage, strJsonData.c_str(), strJsonData.size());

        pulsar_producer_send_async(producer, pulsarMessage, [](pulsar_result res, pulsar_message_id_t *msgId, void *ctx){
//            pulsar_message_t *pulsarMessage = (pulsar_message_t*)ctx;
            if (pulsar_result_Ok != res) {
//                std::string strData = (char*)pulsar_message_get_data(pulsarMessage);
//                uint32_t length = pulsar_message_get_length(pulsarMessage);
//                strData = strData.substr(0, length);
//                cfLog.error() << "Pulsar send error, res: " << pulsar_result_str(res) << " data:" << strData << std::endl;
                cfLog.error() << "Pulsar send error, res: " << pulsar_result_str(res) << std::endl;
            }
            pulsar_message_id_free(msgId);
            msgId = nullptr;
//            pulsar_message_free(pulsarMessage);
//            pulsarMessage = nullptr;
        }, (void*)nullptr);
//        pulsar_result res = pulsar_producer_send(producer, pulsarMessage);
//        if (pulsar_result_Ok != res) {
//            cfLog.info() << "res: " << pulsar_result_str(res) << " Retry sending:" << strJsonData << std::endl;
//            res = pulsar_producer_send(producer, pulsarMessage);
//            cfLog.info() << "Finished Retry sending, res: " << pulsar_result_str(res) << std::endl;
//        }
        pulsar_message_free(pulsarMessage);
        pulsarMessage = nullptr;

    }
}
void PulsarProxy::addOrderConsumer(int iCount){
    if (0 < iCount){
        // order consumer
        std::string strTopic = PULSAR_TOPIC_ORDER_IN + m_strMarketId;
        for (int i = 0; i < iCount; i++)
        {
            std::string strConsumerName = "pulsar-proxy-" + m_strMarketId + "_" + std::to_string(m_iOrderConsumerCount+i);
            createConsumer(ORDER, strTopic, strConsumerName);
        }
        m_iOrderConsumerCount += iCount;
    }
}

void PulsarProxy::setMarketInfo(const nlohmann::json& jsonMarketInfo)
{
    unsigned long long llMarketId = 0;
    OPNX::Utils::getJsonValue<unsigned long long>(llMarketId, jsonMarketInfo, "marketId");
    m_strMarketId = std::to_string(llMarketId);
    OPNX::Utils::getJsonValue<std::string>(m_strMarketCode, jsonMarketInfo, "marketCode");
    OPNX::Utils::getJsonValue<std::string>(m_strReferencePair, jsonMarketInfo, "referencePair");
    OPNX::Utils::getJsonValue<unsigned long long>(m_factor, jsonMarketInfo, "factor");
}

inline bool PulsarProxy::checkOrder(OPNX::Order& order)
{
    bool checkResult = false;
    while (!checkResult)
    {
        if (OPNX::Order::AUCTION == order.timeCondition || OPNX::Order::CANCEL == order.action)  // don't check AUCTION order and CANCEL order
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
//        nlohmann::json jsonOrder;
//        OPNX::Order::orderToJson(order, jsonOrder);
//        nlohmann::json jsonGroupOrder;
//        jsonGroupOrder[OPNX::Order::key_payloadType] = "Order";
//        jsonGroupOrder[OPNX::Order::key_orderList] = nlohmann::json::array();
//        jsonGroupOrder[OPNX::Order::key_orderList].push_back(jsonOrder);
//        sendOrder(jsonGroupOrder);
        std::stringstream ssOrder;
        std::string strJsonOrder = "";
        OPNX::Order::orderToJsonString(order, strJsonOrder);
        ssOrder << "{\"pt\":\"Order\",\"ol\":[" << strJsonOrder << "]}";
        sendOrder(ssOrder.str());
    }
    return checkResult;
}


