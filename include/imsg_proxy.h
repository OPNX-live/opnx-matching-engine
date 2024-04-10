//
// Created by Bob   on 2021/12/7.
//

#ifndef MATCHING_ENGINE_MSG_PROXY_H
#define MATCHING_ENGINE_MSG_PROXY_H

#include "thread_queue.h"
#include "json.hpp"

class IMsgProxy {
public:
    ~virtual IMsgProxy(){};
    virtual init(const nlohmann::json& jsonConfig, OrderQueue* pOrderQueue) = 0;
    virtual sendOrder(char * strOrder) = 0;
    virtual sendOrderBookSnapshot(char* strOrderBookSnapshot) = 0;
    virtual sendOrderBookUpdates(char* strOrderBookUpdate) = 0;
    virtual sendOrderBookBest(char* strOrderBookBest) = 0;
    virtual sendCommand(char* strCommand) = 0;

};
#endif //MATCHING_ENGINE_MSG_PROXY_H
