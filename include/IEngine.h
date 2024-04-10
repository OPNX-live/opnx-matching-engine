//
// Created by Bob   on 2022/2/21.
//

#ifndef MATCHING_ENGINE_IENGINE_H
#define MATCHING_ENGINE_IENGINE_H

#include <vector>

#include "common.h"
#include "implied_engine.h"
#include "implier.h"
#include "order.h"
#include "order_book_item.h"
#include "thread_queue.h"


namespace OPNX {
    class IEngine: public ImpliedEngine{
    public:
        virtual ~IEngine(){}
        virtual void releaseIEngine()=0;
        virtual void handleOrder(OPNX::Order& order)=0;;
        virtual void setImplier(OPNX::Implier& implier)=0;
        virtual void eraseImplier(OPNX::IEngine *pEngine)=0;
        virtual std::string getMarketCode()=0;
        virtual std::string getType()=0;
        virtual std::string getReferencePair()=0;
        virtual long long getMarketId()=0;
        virtual long long getMiniTick()=0;
        virtual void setMakerFees(long long llMakerFees)=0;
        virtual std::string getExpiryTime()=0;
        virtual void setMarketInfo(const nlohmann::json& jsonMarketInfo)=0;
        virtual void getMarketInfo(nlohmann::json& jsonMarketInfo)=0;
        virtual void clearOrder()=0;
        virtual void setOrderGroupCount(int iOrderGroupCount)=0;
    };

}

OPNX::IEngine* createIEngine(const nlohmann::json& jsonMarketInfo, OPNX::ICallbackManager* pCallbackManager);
#endif //MATCHING_ENGINE_IENGINE_H
