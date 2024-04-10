//
// Created by Bob   on 2022/2/21.
//

#ifndef MATCHING_ENGINE_ITRIGGER_ORDER_H
#define MATCHING_ENGINE_ITRIGGER_ORDER_H

#include <vector>

#include "order.h"
#include "order_book_item.h"
#include "thread_queue.h"

namespace OPNX {
    class ITriggerOrder {
    public:
        virtual ~ITriggerOrder() {}

        virtual void releaseITriggerOrder() = 0;

        virtual void handleOrder(OPNX::Order &order) = 0;

        virtual void markPriceTriggerOrder(long long markPrice) = 0;

        virtual void lastPriceTriggerOrder(long long lastPrice) = 0;

        virtual std::string getMarketCode()=0;

        virtual void clearOrder() = 0;

        virtual unsigned long long getOrdersCount()=0;
    };
}

OPNX::ITriggerOrder* createTriggerOrder(const nlohmann::json& jsonMarketInfo, OPNX::ICallbackManager* pCallbackManager);

#endif //MATCHING_ENGINE_ITRIGGER_ORDER_H
