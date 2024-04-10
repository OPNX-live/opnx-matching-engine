//
// Created by Bob   on 2022/6/9.
//

#ifndef MATCHING_ENGINE_COMMON_H
#define MATCHING_ENGINE_COMMON_H

#include "order.h"
#include "order_book_item.h"

namespace OPNX {
    class ICallbackManager{
    public:
        virtual ~ICallbackManager(){}
        virtual void pulsarOrder(const OPNX::Order&)=0;
        virtual void pulsarOrderList(const std::vector<OPNX::Order>&)=0;
        virtual void triggerOrderToEngine(const OPNX::Order&)=0;
        virtual void engineOrderToTrigger(const OPNX::Order&)=0;
        virtual void bestOrderBookChange()=0;
        virtual void orderStore(OPNX::Order*)=0;
    };
}

using CallbackOrder = std::function<void(const OPNX::Order&)>;
using CallbackOrderList = std::function<void(const std::vector<OPNX::Order>&)>;
using CallbackBestOrderBook = std::function<void()>;



#endif //MATCHING_ENGINE_COMMON_H
