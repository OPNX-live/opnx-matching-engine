//
// Created by Bob   on 2022/4/15.
//

#ifndef MATCHING_ENGINE_IMPLIED_ENGINE_H
#define MATCHING_ENGINE_IMPLIED_ENGINE_H


#include "order.h"
#include "order_book_item.h"


using AskOrderBook = std::map<long long, unsigned long long, std::less<long long>>;       // key is price, value is quantity
using BidOrderBook = std::map<long long, unsigned long long, std::greater<long long>>;    // key is price, value is quantity

namespace OPNX {
    class ImpliedEngine {
    public:
        virtual ~ImpliedEngine(){}

        virtual OPNX::OrderBookItem getSelfBestAsk()=0;
        virtual OPNX::OrderBookItem getSelfBestBid()=0;
        virtual OPNX::OrderBookItem getBestAsk(const OPNX::OrderBookItem* pBestBidObItem=nullptr)=0;
        virtual OPNX::OrderBookItem getBestBid(const OPNX::OrderBookItem* pBestAskObItem=nullptr)=0;
        virtual void getSelfDisplayAskOrderBook(AskOrderBook& askOrderBook, int iSize= 400)=0;
        virtual void getSelfDisplayBidOrderBook(BidOrderBook& bidOrderBook, int iSize= 400)=0;
        virtual void getDisplayAskOrderBook(AskOrderBook& askOrderBook, int iSize= 400, int iImpliedSize= 20, const OPNX::OrderBookItem* pBestBidObItem=nullptr)=0;
        virtual void getDisplayBidOrderBook(BidOrderBook& bidOrderBook, int iSize= 400, int iImpliedSize= 20, const OPNX::OrderBookItem* pBestAskObItem=nullptr)=0;
        virtual OPNX::OrderBookAscendMap* getAskOrderBook()=0;
        virtual OPNX::OrderBookDescendMap* getBidOrderBook()=0;
        virtual unsigned long long getAskOrderBookSize()=0;
        virtual unsigned long long getBidOrderBookSize()=0;
        virtual unsigned long long getOrdersCount()=0;
        virtual OPNX::Order* getBestAskOrder()=0;
        virtual OPNX::Order* getBestBidOrder()=0;
        virtual unsigned long long getFactor()=0;
        virtual unsigned long long getQtyFactor()=0;
        virtual long long getMakerFees()=0;
        virtual void updateImpliedMakerFees()=0;
        virtual unsigned long long getOrderMatchableQuantity(const OPNX::Order* pOrder)=0;
        virtual void matchOrder(std::vector<OPNX::Order>& vecMatchedOrder, OPNX::Order& takerOrder, OPNX::Order* pMakerOrder,
                                long long llMatchedPrice, unsigned long long ullMatchQuantity, unsigned long long ullMatchedId,
                                OPNX::Order* pThirdOrder = nullptr, bool bHandleTakerOrder=true, bool bHasRepo = false)=0;
    };

}

#endif //MATCHING_ENGINE_IMPLIED_ENGINE_H
