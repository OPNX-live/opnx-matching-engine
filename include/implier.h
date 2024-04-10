//
// Created by Bob   on 2022/2/10.
//

#ifndef MATCHING_ENGINE_IMPLIER_H
#define MATCHING_ENGINE_IMPLIER_H

#include "implied_engine.h"
#include "math.hpp"
#include "order.h"
#include "order_book_item.h"


namespace OPNX {
    class Implier {
    public:
        enum ImpliedType {
            PERP_REPO_OUT_SPOT = 0,
            SPREAD_PERP_OUT_FUTURES,
            FUTURES_SPREAD_OUT_PERP,
            FUTURES_PERP_OUT_SPREAD,
            SPOT_REPO_OUT_PERP,
            SPOT_PERP_OUT_REPO,
        };
        using ImpliedPriceFunc = std::function<long long(long long, long long)>;
    private:
        OPNX::ImpliedEngine *m_pLeg1Engine;
        OPNX::ImpliedEngine *m_pLeg2Engine;
        ImpliedType m_impliedType;
        long long m_llLeg1MakerFees;
        long long m_llLeg2MakerFees;
        unsigned long long m_ullFactor;
        long long m_miniTick;
    public:
        Implier(OPNX::ImpliedEngine *pLeg1Engine, OPNX::ImpliedEngine *pLeg2Engine, ImpliedType impliedType, long long miniTick,
                unsigned long long ullFactor = 1)
                : m_pLeg1Engine(pLeg1Engine), m_pLeg2Engine(pLeg2Engine), m_impliedType(impliedType), m_miniTick(miniTick),
                  m_ullFactor(ullFactor) { updateMakerFees(); }

        ~Implier() = default;

        void operator=(const Implier &implier) {
            m_pLeg1Engine = implier.m_pLeg1Engine;
            m_pLeg2Engine = implier.m_pLeg2Engine;
            m_impliedType = implier.m_impliedType;
            m_miniTick = implier.m_miniTick;
            m_llLeg1MakerFees = implier.m_llLeg1MakerFees;
            m_llLeg2MakerFees = implier.m_llLeg2MakerFees;
            m_ullFactor = implier.m_ullFactor;
        }

        bool operator==(const Implier &implier) {
            return (m_pLeg1Engine == implier.m_pLeg1Engine
                    && m_pLeg2Engine == implier.m_pLeg2Engine
                    && m_impliedType == implier.m_impliedType);
        }

        bool operator!=(const Implier &implier) {
            return !(operator==(implier));
        }

        bool containEngine(OPNX::ImpliedEngine *pEngine)
        {
            if (pEngine == m_pLeg1Engine || pEngine == m_pLeg2Engine)
            {
                return true;
            } else
            {
                return false;
            }
        }

        void updateMakerFees()
        {
            // If 0==m_ llMakerFees means MakerFees configured by config.json of ME
//            if (0 < m_llMakerFees) {
//                m_llMakerFees = llMakerFees;
//            }

            m_llLeg1MakerFees = m_pLeg1Engine->getMakerFees();
            m_llLeg2MakerFees = m_pLeg2Engine->getMakerFees();
        }
        void setMiniTick(long long llMiniTick)
        {
            m_miniTick = llMiniTick;
        }

        OPNX::OrderBookItem getBestAsk(const OPNX::OrderBookItem& bestBidItem) {
            OPNX::OrderBookItem obItem;
            switch (m_impliedType) {
                case PERP_REPO_OUT_SPOT:
                {
                    obItem = perpRepoOutSpotAsk();
                    break;
                }
                case SPOT_REPO_OUT_PERP:
                {
                    obItem = spotRepoOutPerpAsk();
                    break;
                }
                case SPOT_PERP_OUT_REPO:
                {
                    obItem = spotPerpOutRepoAsk();
                    break;
                }
                case SPREAD_PERP_OUT_FUTURES:
                {
                    obItem = spreadPerpOutFuturesAsk();
                    break;
                }
                case FUTURES_SPREAD_OUT_PERP:
                {
                    obItem = futuresSpreadOutPerpAsk();
                    break;
                }
                case FUTURES_PERP_OUT_SPREAD:
                {
                    obItem = futuresPerpOutSpreadAsk();
                    break;
                }
                default:
                    break;
            }
            if (0 < obItem.quantity &&  0 < bestBidItem.quantity &&  obItem.price <= bestBidItem.price)
            {
                obItem.price = bestBidItem.price + m_miniTick;
            }
            return obItem;
        }

        OPNX::OrderBookItem getBestBid(const OPNX::OrderBookItem& bestAskItem) {
            OPNX::OrderBookItem obItem;
            switch (m_impliedType) {
                case PERP_REPO_OUT_SPOT:
                {
                    obItem =  perpRepoOutSpotBid();
                    break;
                }
                case SPOT_REPO_OUT_PERP:
                {
                    obItem =  spotRepoOutPerpBid();
                    break;
                }
                case SPOT_PERP_OUT_REPO:
                {
                    obItem = spotPerpOutRepoBid();
                    break;
                }
                case SPREAD_PERP_OUT_FUTURES:
                {
                    obItem =  spreadPerpOutFuturesBid();
                    break;
                }
                case FUTURES_SPREAD_OUT_PERP:
                {
                    obItem =  futuresSpreadOutPerpBid();
                    break;
                }
                case FUTURES_PERP_OUT_SPREAD:
                {
                    obItem =  futuresPerpOutSpreadBid();
                    break;
                }
                default:
                    break;
            }
            if (0 < obItem.quantity && 0 < bestAskItem.quantity && obItem.price >= bestAskItem.price)
            {
                obItem.price = bestAskItem.price - m_miniTick;
            }
            return obItem;
        }
        unsigned long long getAskMatchableAmount(long long limitPrice, const OPNX::OrderBookItem& bestBidItem)
        {
            unsigned long long ullAmount = 0;
            switch (m_impliedType) {
                case PERP_REPO_OUT_SPOT: {
                    ullAmount = perpRepoOutSpotAskAmount(limitPrice, bestBidItem);
                    break;
                }
                case SPOT_REPO_OUT_PERP: {
                    ullAmount = spotRepoOutPerpAskAmount(limitPrice, bestBidItem);
                    break;
                }
                case SPOT_PERP_OUT_REPO: {
                    ullAmount = spotPerpOutRepoAskAmount(limitPrice, bestBidItem);
                    break;
                }
                case SPREAD_PERP_OUT_FUTURES: {
                    ullAmount = spreadPerpOutFuturesAskAmount(limitPrice, bestBidItem);
                    break;
                }
                case FUTURES_SPREAD_OUT_PERP: {
                    ullAmount = futuresSpreadOutPerpAskAmount(limitPrice, bestBidItem);
                    break;
                }
                case FUTURES_PERP_OUT_SPREAD: {
                    ullAmount = futuresPerpOutSpreadAskAmount(limitPrice, bestBidItem);
                    break;
                }
                default:
                    ullAmount = 0;
                    break;
            }
            return ullAmount;
        }
        unsigned long long getBidMatchableAmount(long long limitPrice, const OPNX::OrderBookItem& bestAskItem)
        {
            unsigned long long ullAmount = 0;
            switch (m_impliedType) {
                case PERP_REPO_OUT_SPOT: {
                    ullAmount = perpRepoOutSpotBidAmount(limitPrice, bestAskItem);
                    break;
                }
                case SPOT_REPO_OUT_PERP: {
                    ullAmount = spotRepoOutPerpBidAmount(limitPrice, bestAskItem);
                    break;
                }
                case SPOT_PERP_OUT_REPO: {
                    ullAmount = spotPerpOutRepoBidAmount(limitPrice, bestAskItem);
                    break;
                }
                case SPREAD_PERP_OUT_FUTURES: {
                    ullAmount = spreadPerpOutFuturesBidAmount(limitPrice, bestAskItem);
                    break;
                }
                case FUTURES_SPREAD_OUT_PERP: {
                    ullAmount = futuresSpreadOutPerpBidAmount(limitPrice, bestAskItem);
                    break;
                }
                case FUTURES_PERP_OUT_SPREAD: {
                    ullAmount = futuresPerpOutSpreadBidAmount(limitPrice, bestAskItem);
                    break;
                }
                default:
                    ullAmount = 0;
                    break;
            }
            return ullAmount;
        }

        void impliedAskOrderBook(AskOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestBidItem)
        {
            switch (m_impliedType) {
                case PERP_REPO_OUT_SPOT: {
                    perpRepoOutSpotAskOrderBook(orderBook, iImpliedSize, bestBidItem);
                    break;
                }
                case SPOT_REPO_OUT_PERP: {
                    spotRepoOutPerpAskOrderBook(orderBook, iImpliedSize, bestBidItem);
                    break;
                }
                case SPOT_PERP_OUT_REPO: {
                    spotPerpOutRepoAskOrderBook(orderBook, iImpliedSize, bestBidItem);
                    break;
                }
                case SPREAD_PERP_OUT_FUTURES: {
                    spreadPerpOutFuturesAskOrderBook(orderBook, iImpliedSize, bestBidItem);
                    break;
                }
                case FUTURES_SPREAD_OUT_PERP: {
                    futuresSpreadOutPerpAskOrderBook(orderBook, iImpliedSize, bestBidItem);
                    break;
                }
                case FUTURES_PERP_OUT_SPREAD: {
                    futuresPerpOutSpreadAskOrderBook(orderBook, iImpliedSize, bestBidItem);
                    break;
                }
                default:
                    break;
            }
        }
        void impliedBidOrderBook(BidOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestAskItem)
        {
            switch (m_impliedType) {
                case PERP_REPO_OUT_SPOT: {
                    perpRepoOutSpotBidOrderBook(orderBook, iImpliedSize, bestAskItem);
                    break;
                }
                case SPOT_REPO_OUT_PERP: {
                    spotRepoOutPerpBidOrderBook(orderBook, iImpliedSize, bestAskItem);
                break;
                }
                case SPOT_PERP_OUT_REPO: {
                    spotPerpOutRepoBidOrderBook(orderBook, iImpliedSize, bestAskItem);
                    break;
                }
                case SPREAD_PERP_OUT_FUTURES: {
                    spreadPerpOutFuturesBidOrderBook(orderBook, iImpliedSize, bestAskItem);
                    break;
                }
                case FUTURES_SPREAD_OUT_PERP: {
                    futuresSpreadOutPerpBidOrderBook(orderBook, iImpliedSize, bestAskItem);
                    break;
                }
                case FUTURES_PERP_OUT_SPREAD: {
                    futuresPerpOutSpreadBidOrderBook(orderBook, iImpliedSize, bestAskItem);
                    break;
                }
                default:
                    break;
            }
        }

        unsigned long long matchOrder(std::vector<OPNX::Order>& vecMatchedOrder, OPNX::Order& takerOrder, long long llMatchedPrice, unsigned long long ullMatchQuantity, OPNX::ICallbackManager* pCallbackManager, int iOrderGroupCount, bool bHasRepo = false)
        {
            OPNX::CInOutLog cInOutLog("implied matchOrder");

            while (0 < ullMatchQuantity)
            {
                OPNX::Order* pMaker1Order = nullptr;
                OPNX::Order* pMaker2Order = nullptr;
                if (OPNX::Order::BUY == takerOrder.side)
                {
                    switch (m_impliedType) {
                        case PERP_REPO_OUT_SPOT:
                        {
                            // perp ask + repo ask -> spot ask
                            pMaker1Order = m_pLeg1Engine->getBestAskOrder();
                            pMaker2Order = m_pLeg2Engine->getBestAskOrder();
                            break;
                        }
                        case SPOT_REPO_OUT_PERP:
                        {
                            // spot ask + repo bid -> perp ask
                            pMaker1Order = m_pLeg1Engine->getBestAskOrder();
                            pMaker2Order = m_pLeg2Engine->getBestBidOrder();
                            break;
                        }
                        case SPOT_PERP_OUT_REPO:
                        {
                            // spot ask + perp bid -> repo ask
                            pMaker1Order = m_pLeg1Engine->getBestAskOrder();
                            pMaker2Order = m_pLeg2Engine->getBestBidOrder();
                            break;
                        }
                        case SPREAD_PERP_OUT_FUTURES:
                        {
                            // spread ask + perp ask -> futures ask
                            pMaker1Order = m_pLeg1Engine->getBestAskOrder();
                            pMaker2Order = m_pLeg2Engine->getBestAskOrder();
                            break;
                        }
                        case FUTURES_SPREAD_OUT_PERP:
                        {
                            // futures ask + spread bid -> perp ask
                            pMaker1Order = m_pLeg1Engine->getBestAskOrder();
                            pMaker2Order = m_pLeg2Engine->getBestBidOrder();
                            break;
                        }
                        case FUTURES_PERP_OUT_SPREAD:
                        {
                            // futures ask + perp bid -> spread ask
                            pMaker1Order = m_pLeg1Engine->getBestAskOrder();
                            pMaker2Order = m_pLeg2Engine->getBestBidOrder();
                            break;
                        }
                        default:
                            break;
                    }
                }
                else  // OPNX::Order::SELL == order.side
                {
                    switch (m_impliedType) {
                        case PERP_REPO_OUT_SPOT:
                        {
                            // perp bid + repo bid -> spot bid
                            pMaker1Order = m_pLeg1Engine->getBestBidOrder();
                            pMaker2Order = m_pLeg2Engine->getBestBidOrder();
                            break;
                        }
                        case SPOT_REPO_OUT_PERP:
                        {
                            // spot bid + repo ask -> perp bid
                            pMaker1Order = m_pLeg1Engine->getBestBidOrder();
                            pMaker2Order = m_pLeg2Engine->getBestAskOrder();
                            break;
                        }
                        case SPOT_PERP_OUT_REPO:
                        {
                            // spot bid + perp ask -> repo bid
                            pMaker1Order = m_pLeg1Engine->getBestBidOrder();
                            pMaker2Order = m_pLeg2Engine->getBestAskOrder();
                            break;
                        }
                        case SPREAD_PERP_OUT_FUTURES:
                        {
                            // spread bid + perp bid -> futures bid
                            pMaker1Order = m_pLeg1Engine->getBestBidOrder();
                            pMaker2Order = m_pLeg2Engine->getBestBidOrder();
                            break;
                        }
                        case FUTURES_SPREAD_OUT_PERP:
                        {
                            // futures bid + spread ask -> perp bid
                            pMaker1Order = m_pLeg1Engine->getBestBidOrder();
                            pMaker2Order = m_pLeg2Engine->getBestAskOrder();
                            break;
                        }
                        case FUTURES_PERP_OUT_SPREAD:
                        {
                            // futures bid + perp ask -> spread bid
                            pMaker1Order = m_pLeg1Engine->getBestBidOrder();
                            pMaker2Order = m_pLeg2Engine->getBestAskOrder();
                            break;
                        }
                        default:
                            break;
                    }
                }
                if (nullptr != pMaker1Order && nullptr != pMaker2Order)
                {
                    unsigned long long ullMatchedId = OPNX::Utils::getMatchId();
                    unsigned long long quantity1 = m_pLeg1Engine->getOrderMatchableQuantity(pMaker1Order);
                    unsigned long long quantity2 = m_pLeg2Engine->getOrderMatchableQuantity(pMaker2Order);

//                    unsigned long long orderId1 = pMaker1Order->orderId;
//                    unsigned long long orderId2 = pMaker2Order->orderId;

                    unsigned long long ullMinMatchQuantity = std::min(ullMatchQuantity, std::min(quantity1, quantity2));
                    m_pLeg1Engine->matchOrder(vecMatchedOrder, takerOrder, pMaker1Order, llMatchedPrice, ullMinMatchQuantity, ullMatchedId, pMaker2Order, true, bHasRepo);
                    m_pLeg2Engine->matchOrder(vecMatchedOrder, takerOrder, pMaker2Order, llMatchedPrice, ullMinMatchQuantity, ullMatchedId, pMaker1Order, false, bHasRepo);
                    ullMatchQuantity -= ullMinMatchQuantity;
                }
                else
                {
                    break;
                }
                // Split group orders
                if (OPNX::Order::AUCTION != takerOrder.timeCondition && iOrderGroupCount <= vecMatchedOrder.size())
                {
                    pCallbackManager->pulsarOrderList(vecMatchedOrder);
                    vecMatchedOrder.clear();
                }
            }
            return ullMatchQuantity;  // Return to match remaining quantity
        }

        long long perpRepoOutSpotAskPrice(long long leg1Price, long long leg2Price) {
            // perp ask + repo ask -> spot ask
            return round_up(round_up(leg1Price * (double(m_ullFactor) + leg2Price) / m_ullFactor, 1) + handle_pips(leg1Price, m_llLeg1MakerFees), m_miniTick);

        }

        long long perpRepoOutSpotBidPrice(long long leg1Price, long long leg2Price) {
            // perp bid + repo bid -> spot bid
            return round_down(leg1Price * (double(m_ullFactor) + leg2Price) / m_ullFactor - handle_pips(leg1Price, m_llLeg1MakerFees), m_miniTick);

        }
        long long spotRepoOutPerpAskPrice(long long leg1Price, long long leg2Price) {
            // spot ask + repo bid -> perp ask
            return round_up((leg1Price + handle_pips(leg1Price, m_llLeg1MakerFees))/((double(m_ullFactor) + leg2Price) / m_ullFactor), m_miniTick);

        }
        long long spotRepoOutPerpBidPrice(long long leg1Price, long long leg2Price) {
            // spot bid + repo ask -> perp bid
            return round_down((leg1Price - handle_pips(leg1Price, m_llLeg1MakerFees))/((double(m_ullFactor) + leg2Price) / m_ullFactor), m_miniTick);

        }
        long long spotPerpOutRepoAskPrice(long long leg1Price, long long leg2Price) {
            // spot ask + perp bid -> repo ask
            return round_up((double(leg1Price + handle_pips(leg1Price, m_llLeg1MakerFees) + handle_pips(leg2Price, m_llLeg2MakerFees))/leg2Price - 1)*m_ullFactor, m_miniTick);

        }
        long long spotPerpOutRepoBidPrice(long long leg1Price, long long leg2Price) {
            // spot bid + perp ask -> repo bid
            return round_down((double(leg1Price - handle_pips(leg1Price, m_llLeg1MakerFees) - handle_pips(leg2Price, m_llLeg2MakerFees))/leg2Price - 1)*m_ullFactor, m_miniTick);

        }

        long long spreadPerpOutFuturesAskPrice(long long leg1Price, long long leg2Price) {
            // spread ask + perp ask -> futures ask
            return round_up(leg1Price + leg2Price + handle_pips(leg1Price, m_llLeg1MakerFees) + handle_pips(leg2Price, m_llLeg2MakerFees), m_miniTick);
        }

        long long spreadPerpOutFuturesBidPrice(long long leg1Price, long long leg2Price) {
            // spread bid + perp bid -> futures bid
            return round_down(leg1Price + leg2Price - handle_pips(leg1Price, m_llLeg1MakerFees) - handle_pips(leg2Price, m_llLeg2MakerFees), m_miniTick);
        }

        long long futuresSpreadOutPerpAskPrice(long long leg1Price, long long leg2Price) {
            // futures ask + spread bid -> perp ask
            return round_up(leg1Price - leg2Price + handle_pips(leg1Price, m_llLeg1MakerFees) + handle_pips(leg2Price, m_llLeg2MakerFees), m_miniTick);
        }

        long long futuresSpreadOutPerpBidPrice(long long leg1Price, long long leg2Price) {
            // futures bid + spread ask -> perp bid
            return round_down(leg1Price - leg2Price - handle_pips(leg1Price, m_llLeg1MakerFees) - handle_pips(leg2Price, m_llLeg2MakerFees), m_miniTick);
        }

        long long futuresPerpOutSpreadAskPrice(long long leg1Price, long long leg2Price) {
            // futures ask + perp bid -> spread ask
            return round_up(leg1Price - leg2Price + handle_pips(leg1Price, m_llLeg1MakerFees) + handle_pips(leg2Price, m_llLeg2MakerFees), m_miniTick);
        }

        long long futuresPerpOutSpreadBidPrice(long long leg1Price, long long leg2Price) {
            // futures bid + perp ask -> spread bid
            return round_down(leg1Price - leg2Price - handle_pips(leg1Price, m_llLeg1MakerFees) - handle_pips(leg2Price, m_llLeg2MakerFees), m_miniTick);
        }

    private:
        OPNX::OrderBookItem impliedTemplate(OPNX::OrderBookItem& leg1ObItem, OPNX::OrderBookItem& leg2ObItem, ImpliedPriceFunc&& impliedPriceFunc)
        {
            OPNX::OrderBookItem obItem;
            if (0 != leg1ObItem.quantity && 0 != leg2ObItem.quantity)
            {
                obItem.quantity = std::min(leg1ObItem.quantity, leg2ObItem.quantity);
                obItem.displayQuantity = std::min(leg1ObItem.displayQuantity, leg2ObItem.displayQuantity);
                long long leg1Price = leg1ObItem.price;
                long long leg2Price = leg2ObItem.price;
                obItem.price = impliedPriceFunc(leg1Price, leg2Price);
            }
            return obItem;
        }

        OPNX::OrderBookItem perpRepoOutSpotAsk() {
            // perp ask + repo ask -> spot ask
            auto leg1ObItem = m_pLeg1Engine->getSelfBestAsk();
            auto leg2ObItem = m_pLeg2Engine->getSelfBestAsk();
            return impliedTemplate(leg1ObItem, leg2ObItem, [&](long long leg1Price, long long leg2Price)->long long{return perpRepoOutSpotAskPrice(leg1Price, leg2Price);});
        }

        OPNX::OrderBookItem perpRepoOutSpotBid() {
            // perp bid + repo bid -> spot bid
            auto leg1ObItem = m_pLeg1Engine->getSelfBestBid();
            auto leg2ObItem = m_pLeg2Engine->getSelfBestBid();
            return impliedTemplate(leg1ObItem, leg2ObItem, [&](long long leg1Price, long long leg2Price)->long long{ return perpRepoOutSpotBidPrice(leg1Price, leg2Price);});

        }
        OPNX::OrderBookItem spotRepoOutPerpAsk() {
            // spot ask + repo bid -> perp ask
            auto leg1ObItem = m_pLeg1Engine->getSelfBestAsk();
            auto leg2ObItem = m_pLeg2Engine->getSelfBestBid();
            return impliedTemplate(leg1ObItem, leg2ObItem, [&](long long leg1Price, long long leg2Price)->long long{return spotRepoOutPerpAskPrice(leg1Price, leg2Price);});
        }

        OPNX::OrderBookItem spotRepoOutPerpBid() {
            // spot bid + repo ask -> perp bid
            auto leg1ObItem = m_pLeg1Engine->getSelfBestBid();
            auto leg2ObItem = m_pLeg2Engine->getSelfBestAsk();
            return impliedTemplate(leg1ObItem, leg2ObItem, [&](long long leg1Price, long long leg2Price)->long long{ return spotRepoOutPerpBidPrice(leg1Price, leg2Price);});

        }
        OPNX::OrderBookItem spotPerpOutRepoAsk() {
            // spot ask + perp bid -> repo ask
            auto leg1ObItem = m_pLeg1Engine->getSelfBestAsk();
            auto leg2ObItem = m_pLeg2Engine->getSelfBestBid();
            return impliedTemplate(leg1ObItem, leg2ObItem, [&](long long leg1Price, long long leg2Price)->long long{return spotPerpOutRepoAskPrice(leg1Price, leg2Price);});
        }

        OPNX::OrderBookItem spotPerpOutRepoBid() {
            // spot bid + perp ask -> repo bid
            auto leg1ObItem = m_pLeg1Engine->getSelfBestBid();
            auto leg2ObItem = m_pLeg2Engine->getSelfBestAsk();
            return impliedTemplate(leg1ObItem, leg2ObItem, [&](long long leg1Price, long long leg2Price)->long long{ return spotPerpOutRepoBidPrice(leg1Price, leg2Price);});

        }

        OPNX::OrderBookItem spreadPerpOutFuturesAsk() {
            // spread ask + perp ask -> futures ask
            auto leg1ObItem = m_pLeg1Engine->getSelfBestAsk();
            auto leg2ObItem = m_pLeg2Engine->getSelfBestAsk();
            return impliedTemplate(leg1ObItem, leg2ObItem, [&](long long leg1Price, long long leg2Price)->long long{return spreadPerpOutFuturesAskPrice(leg1Price, leg2Price);});
        }

        OPNX::OrderBookItem spreadPerpOutFuturesBid() {
            // spread bid + perp bid -> futures bid
            auto leg1ObItem = m_pLeg1Engine->getSelfBestBid();
            auto leg2ObItem = m_pLeg2Engine->getSelfBestBid();
            return impliedTemplate(leg1ObItem, leg2ObItem, [&](long long leg1Price, long long leg2Price)->long long{return spreadPerpOutFuturesBidPrice(leg1Price, leg2Price);});
        }

        OPNX::OrderBookItem futuresSpreadOutPerpAsk() {
            // futures ask + spread bid -> perp ask
            auto leg1ObItem = m_pLeg1Engine->getSelfBestAsk();
            auto leg2ObItem = m_pLeg2Engine->getSelfBestBid();
            return impliedTemplate(leg1ObItem, leg2ObItem, [&](long long leg1Price, long long leg2Price)->long long{return futuresSpreadOutPerpAskPrice(leg1Price, leg2Price);});

        }

        OPNX::OrderBookItem futuresSpreadOutPerpBid() {
            // futures bid + spread ask -> perp bid
            auto leg1ObItem = m_pLeg1Engine->getSelfBestBid();
            auto leg2ObItem = m_pLeg2Engine->getSelfBestAsk();
            return impliedTemplate(leg1ObItem, leg2ObItem, [&](long long leg1Price, long long leg2Price)->long long{return futuresSpreadOutPerpBidPrice(leg1Price, leg2Price);});

        }

        OPNX::OrderBookItem futuresPerpOutSpreadAsk() {
            // futures ask + perp bid -> spread ask
            auto leg1ObItem = m_pLeg1Engine->getSelfBestAsk();
            auto leg2ObItem = m_pLeg2Engine->getSelfBestBid();
            return impliedTemplate(leg1ObItem, leg2ObItem, [&](long long leg1Price, long long leg2Price)->long long{return futuresPerpOutSpreadAskPrice(leg1Price, leg2Price);});

        }

        OPNX::OrderBookItem futuresPerpOutSpreadBid() {
            // futures bid + perp ask -> spread bid
            auto leg1ObItem = m_pLeg1Engine->getSelfBestBid();
            auto leg2ObItem = m_pLeg2Engine->getSelfBestAsk();
            return impliedTemplate(leg1ObItem, leg2ObItem, [&](long long leg1Price, long long leg2Price)->long long{return futuresPerpOutSpreadBidPrice(leg1Price, leg2Price);});

        }

        template<typename Leg1OrderBook, typename Leg2OrderBook>
        unsigned long long impliedMatchableAmount(long long limitPrice, OPNX::Order::OrderSide orderSide, const OPNX::OrderBookItem& bestItem, Leg1OrderBook *leg1OrderBook, Leg2OrderBook *leg2OrderBook, ImpliedPriceFunc&& impliedPriceFunc)
        {
//            OPNX::CInOutLog cInOutLog("impliedMatchableAmount");

            unsigned long long ullAmount = 0;
            auto leg1Item = leg1OrderBook->begin();
            auto leg2Item = leg2OrderBook->begin();
            unsigned long long quantity1 = 0;
            unsigned long long quantity2 = 0;
            if (leg1Item != leg1OrderBook->end() && leg2Item != leg2OrderBook->end())
            {
                quantity1 = leg1Item->second.obItem.quantity;
                quantity2 = leg2Item->second.obItem.quantity;
            }

            while (leg1Item != leg1OrderBook->end() && leg2Item != leg2OrderBook->end())
            {
                unsigned long long quantity = std::min(quantity1, quantity2);
                long long leg1Price = leg1Item->first;
                long long leg2Price = leg2Item->first;
                auto price = impliedPriceFunc(leg1Price, leg2Price);
                if (OPNX::Order::OrderSide::SELL == orderSide)
                {
                    if (0 < bestItem.quantity)
                    {
                        if (price <= bestItem.price)
                        {
                            price = bestItem.price + m_miniTick;
                        }
                    }
                    if (price <= limitPrice)
                    {
                        ullAmount += quantity;
                        quantity1 -= quantity;
                        quantity2 -= quantity;
                        if (0 == quantity1)
                        {
                            leg1Item++;
                            if (leg1Item != leg1OrderBook->end())
                            {
                                quantity1 = leg1Item->second.obItem.quantity;
                            }
                        }
                        if (0 == quantity2)
                        {
                            leg2Item++;
                            if (leg2Item != leg2OrderBook->end())
                            {
                                quantity2 = leg2Item->second.obItem.quantity;
                            }
                        }
                    }
                    else {
                        break;
                    }
                }
                else
                {
                    if (0 < bestItem.quantity)
                    {
                        if (price >= bestItem.price)
                        {
                            price = bestItem.price - m_miniTick;
                        }
                    }
                    if (price >= limitPrice)
                    {
                        ullAmount += quantity;
                        quantity1 -= quantity;
                        quantity2 -= quantity;
                        if (0 == quantity1)
                        {
                            leg1Item++;
                            if (leg1Item != leg1OrderBook->end())
                            {
                                quantity1 = leg1Item->second.obItem.quantity;
                            }
                        }
                        if (0 == quantity2)
                        {
                            leg2Item++;
                            if (leg2Item != leg2OrderBook->end())
                            {
                                quantity2 = leg2Item->second.obItem.quantity;
                            }
                        }
                    }
                    else {
                        break;
                    }
                }
            }
            return ullAmount;
        }

        unsigned long long perpRepoOutSpotAskAmount(long long limitPrice, const OPNX::OrderBookItem& bestBidItem) {
            // perp ask + repo ask -> spot ask
            auto leg1OrderBook = m_pLeg1Engine->getAskOrderBook();
            auto leg2OrderBook = m_pLeg2Engine->getAskOrderBook();
            return impliedMatchableAmount<OPNX::OrderBookAscendMap, OPNX::OrderBookAscendMap>(limitPrice, OPNX::Order::OrderSide::SELL, bestBidItem, leg1OrderBook, leg2OrderBook,
                                                                                                      [&](long long leg1Price, long long leg2Price)->long long{return perpRepoOutSpotAskPrice(leg1Price, leg2Price);});

        }

        unsigned long long perpRepoOutSpotBidAmount(long long limitPrice, const OPNX::OrderBookItem& bestAskItem) {
            // perp bid + repo bid -> spot bid
            auto leg1OrderBook = m_pLeg1Engine->getBidOrderBook();
            auto leg2OrderBook = m_pLeg2Engine->getBidOrderBook();
            return impliedMatchableAmount<OrderBookDescendMap, OrderBookDescendMap>(limitPrice, OPNX::Order::OrderSide::BUY, bestAskItem, leg1OrderBook, leg2OrderBook,
                                                                                    [&](long long leg1Price, long long leg2Price)->long long{return perpRepoOutSpotBidPrice(leg1Price, leg2Price);});

        }

        unsigned long long spotRepoOutPerpAskAmount(long long limitPrice, const OPNX::OrderBookItem& bestBidItem) {
            // spot ask + repo bid -> perp ask
            auto leg1OrderBook = m_pLeg1Engine->getAskOrderBook();
            auto leg2OrderBook = m_pLeg2Engine->getBidOrderBook();
            return impliedMatchableAmount<OPNX::OrderBookAscendMap, OPNX::OrderBookDescendMap>(limitPrice, OPNX::Order::OrderSide::SELL, bestBidItem, leg1OrderBook, leg2OrderBook,
                                                                                                       [&](long long leg1Price, long long leg2Price)->long long{return spotRepoOutPerpAskPrice(leg1Price, leg2Price);});

        }

        unsigned long long spotRepoOutPerpBidAmount(long long limitPrice, const OPNX::OrderBookItem& bestAskItem) {
            // spot bid + repo ask -> perp bid
            auto leg1OrderBook = m_pLeg1Engine->getBidOrderBook();
            auto leg2OrderBook = m_pLeg2Engine->getAskOrderBook();
            return impliedMatchableAmount<OrderBookDescendMap, OrderBookAscendMap>(limitPrice, OPNX::Order::OrderSide::BUY, bestAskItem, leg1OrderBook, leg2OrderBook,
                                                                                   [&](long long leg1Price, long long leg2Price)->long long{return spotRepoOutPerpBidPrice(leg1Price, leg2Price);});

        }

        unsigned long long spotPerpOutRepoAskAmount(long long limitPrice, const OPNX::OrderBookItem& bestBidItem) {
            // spot ask + perp bid -> repo ask
            auto leg1OrderBook = m_pLeg1Engine->getAskOrderBook();
            auto leg2OrderBook = m_pLeg2Engine->getBidOrderBook();
            return impliedMatchableAmount<OPNX::OrderBookAscendMap, OPNX::OrderBookDescendMap>(limitPrice, OPNX::Order::OrderSide::SELL, bestBidItem, leg1OrderBook, leg2OrderBook,
                                                                                                       [&](long long leg1Price, long long leg2Price)->long long{return spotPerpOutRepoAskPrice(leg1Price, leg2Price);});

        }

        unsigned long long spotPerpOutRepoBidAmount(long long limitPrice, const OPNX::OrderBookItem& bestAskItem) {
            // spot bid + perp ask -> repo bid
            auto leg1OrderBook = m_pLeg1Engine->getBidOrderBook();
            auto leg2OrderBook = m_pLeg2Engine->getAskOrderBook();
            return impliedMatchableAmount<OrderBookDescendMap, OrderBookAscendMap>(limitPrice, OPNX::Order::OrderSide::BUY, bestAskItem, leg1OrderBook, leg2OrderBook,
                                                                                   [&](long long leg1Price, long long leg2Price)->long long{return spotPerpOutRepoBidPrice(leg1Price, leg2Price);});

        }

        unsigned long long spreadPerpOutFuturesAskAmount(long long limitPrice, const OPNX::OrderBookItem& bestBidItem) {
            // spread ask + perp ask -> futures ask
            auto leg1OrderBook = m_pLeg1Engine->getAskOrderBook();
            auto leg2OrderBook = m_pLeg2Engine->getAskOrderBook();
            return impliedMatchableAmount<OPNX::OrderBookAscendMap, OPNX::OrderBookAscendMap>(limitPrice, OPNX::Order::OrderSide::SELL, bestBidItem, leg1OrderBook, leg2OrderBook,
                                                                      [&](long long leg1Price, long long leg2Price)->long long{return spreadPerpOutFuturesAskPrice(leg1Price, leg2Price);});

        }

        unsigned long long spreadPerpOutFuturesBidAmount(long long limitPrice, const OPNX::OrderBookItem& bestAskItem) {
            // spread bid + perp bid -> futures bid
            auto leg1OrderBook = m_pLeg1Engine->getBidOrderBook();
            auto leg2OrderBook = m_pLeg2Engine->getBidOrderBook();
            return impliedMatchableAmount<OPNX::OrderBookDescendMap, OPNX::OrderBookDescendMap>(limitPrice, OPNX::Order::OrderSide::BUY, bestAskItem, leg1OrderBook, leg2OrderBook,
                                                                      [&](long long leg1Price, long long leg2Price)->long long{return spreadPerpOutFuturesBidPrice(leg1Price, leg2Price);});

        }

        unsigned long long futuresSpreadOutPerpAskAmount(long long limitPrice, const OPNX::OrderBookItem& bestBidItem) {
            // futures ask + spread bid -> perp ask
            auto leg1OrderBook = m_pLeg1Engine->getAskOrderBook();
            auto leg2OrderBook = m_pLeg2Engine->getBidOrderBook();
            return impliedMatchableAmount<OPNX::OrderBookAscendMap, OPNX::OrderBookDescendMap>(limitPrice, OPNX::Order::OrderSide::SELL, bestBidItem, leg1OrderBook, leg2OrderBook,
                                                                      [&](long long leg1Price, long long leg2Price)->long long{return futuresSpreadOutPerpAskPrice(leg1Price, leg2Price);});

        }

        unsigned long long futuresSpreadOutPerpBidAmount(long long limitPrice, const OPNX::OrderBookItem& bestAskItem) {
            // futures bid + spread ask -> perp bid
            auto leg1OrderBook = m_pLeg1Engine->getBidOrderBook();
            auto leg2OrderBook = m_pLeg2Engine->getAskOrderBook();
            return impliedMatchableAmount<OPNX::OrderBookDescendMap, OPNX::OrderBookAscendMap>(limitPrice, OPNX::Order::OrderSide::BUY, bestAskItem, leg1OrderBook, leg2OrderBook,
                                                                      [&](long long leg1Price, long long leg2Price)->long long{return futuresSpreadOutPerpBidPrice(leg1Price, leg2Price);});

        }

        unsigned long long futuresPerpOutSpreadAskAmount(long long limitPrice, const OPNX::OrderBookItem& bestBidItem) {
            // futures ask + perp bid -> spread ask
            auto leg1OrderBook = m_pLeg1Engine->getAskOrderBook();
            auto leg2OrderBook = m_pLeg2Engine->getBidOrderBook();
            return impliedMatchableAmount<OPNX::OrderBookAscendMap, OPNX::OrderBookDescendMap>(limitPrice, OPNX::Order::OrderSide::SELL, bestBidItem, leg1OrderBook, leg2OrderBook,
                                                                      [&](long long leg1Price, long long leg2Price)->long long{return futuresPerpOutSpreadAskPrice(leg1Price, leg2Price);});

        }

        unsigned long long futuresPerpOutSpreadBidAmount(long long limitPrice, const OPNX::OrderBookItem& bestAskItem) {
            // futures bid + perp ask -> spread bid
            auto leg1OrderBook = m_pLeg1Engine->getBidOrderBook();
            auto leg2OrderBook = m_pLeg2Engine->getAskOrderBook();
            return impliedMatchableAmount<OPNX::OrderBookDescendMap, OPNX::OrderBookAscendMap>(limitPrice, OPNX::Order::OrderSide::BUY, bestAskItem, leg1OrderBook, leg2OrderBook,
                                                                      [&](long long leg1Price, long long leg2Price)->long long{return futuresPerpOutSpreadBidPrice(leg1Price, leg2Price);});

        }

        template<typename OrderBook, typename Leg1OrderBook, typename Leg2OrderBook>
        void impliedOrderBook(OrderBook& orderBook, int iImpliedSize, OPNX::Order::OrderSide orderSide, const OPNX::OrderBookItem& bestItem, Leg1OrderBook *leg1OrderBook, Leg2OrderBook *leg2OrderBook, ImpliedPriceFunc&& impliedPriceFunc)
        {
            OrderBook tempOrderBook;
            auto leg1Item = leg1OrderBook->begin();
            auto leg2Item = leg2OrderBook->begin();
            unsigned long long quantity1 = 0;
            unsigned long long quantity2 = 0;
            if (leg1Item != leg1OrderBook->end() && leg2Item != leg2OrderBook->end())
            {
                quantity1 = leg1Item->second;
                quantity2 = leg2Item->second;
            }

            while (leg1Item != leg1OrderBook->end() && leg2Item != leg2OrderBook->end())
            {
                unsigned long long quantity = std::min(quantity1, quantity2);
                if (0 == quantity)   // filter quantity is 0
                {
                    if (0 == quantity1)
                    {
                        leg1Item++;
                        if (leg1Item != leg1OrderBook->end())
                        {
                            quantity1 = leg1Item->second;
                        }
                    }
                    if (0 == quantity2)
                    {
                        leg2Item++;
                        if (leg2Item != leg2OrderBook->end())
                        {
                            quantity2 = leg2Item->second;
                        }
                    }
                    continue;
                }
                long long leg1Price = leg1Item->first;
                long long leg2Price = leg2Item->first;
                auto price = impliedPriceFunc(leg1Price, leg2Price);
                if (0 < bestItem.quantity)
                {
                    if (OPNX::Order::OrderSide::SELL == orderSide)
                    {
                        if (price <= bestItem.price)
                        {
                            price = bestItem.price + m_miniTick;
                        }
                    }
                    else
                    {
                        if (price >= bestItem.price)
                        {
                            price = bestItem.price - m_miniTick;
                        }
                    }
                }
                auto it = tempOrderBook.find(price);
                if (tempOrderBook.end() == it)
                {
                    tempOrderBook.emplace(std::pair<long long, unsigned long long>(price, quantity));
                }
                else
                {
                    it->second = it->second + quantity;
                }
                if (tempOrderBook.size() < iImpliedSize)
                {
                    quantity1 -= quantity;
                    quantity2 -= quantity;
                    if (0 == quantity1)
                    {
                        leg1Item++;
                        if (leg1Item != leg1OrderBook->end())
                        {
                            quantity1 = leg1Item->second;
                        }
                    }
                    if (0 == quantity2)
                    {
                        leg2Item++;
                        if (leg2Item != leg2OrderBook->end())
                        {
                            quantity2 = leg2Item->second;
                        }
                    }
                }
                else
                {
                    break;
                }
            }

            for (auto itOb: tempOrderBook)
            {
                auto it = orderBook.find(itOb.first);
                if (orderBook.end() == it)
                {
                    orderBook.emplace(std::pair<long long, unsigned long long>(itOb.first, itOb.second));
                }
                else
                {
                    it->second = it->second + itOb.second;
                }
            }
        }

        void perpRepoOutSpotAskOrderBook(AskOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestBidItem) {
            // perp ask + repo ask -> spot ask
            AskOrderBook leg1OrderBook;
            AskOrderBook leg2OrderBook;
            m_pLeg1Engine->getSelfDisplayAskOrderBook(leg1OrderBook);
            m_pLeg2Engine->getSelfDisplayAskOrderBook(leg2OrderBook);
            impliedOrderBook<AskOrderBook, AskOrderBook, AskOrderBook>(orderBook, iImpliedSize, OPNX::Order::OrderSide::SELL, bestBidItem, &leg1OrderBook, &leg2OrderBook,
                                                                       [&](long long leg1Price, long long leg2Price)->long long{return perpRepoOutSpotAskPrice(leg1Price, leg2Price);});

        }

        void perpRepoOutSpotBidOrderBook(BidOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestAskItem) {
            // perp bid + repo bid -> spot bid
            BidOrderBook leg1OrderBook;
            BidOrderBook leg2OrderBook;
            m_pLeg1Engine->getSelfDisplayBidOrderBook(leg1OrderBook);
            m_pLeg2Engine->getSelfDisplayBidOrderBook(leg2OrderBook);
            impliedOrderBook<BidOrderBook, BidOrderBook, BidOrderBook>(orderBook, iImpliedSize, OPNX::Order::OrderSide::BUY, bestAskItem, &leg1OrderBook, &leg2OrderBook,
                                                                       [&](long long leg1Price, long long leg2Price)->long long{return perpRepoOutSpotBidPrice(leg1Price, leg2Price);});

        }
        void spotRepoOutPerpAskOrderBook(AskOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestBidItem) {
            // spot ask + repo bid -> perp ask
            AskOrderBook leg1OrderBook;
            BidOrderBook leg2OrderBook;
            m_pLeg1Engine->getSelfDisplayAskOrderBook(leg1OrderBook);
            m_pLeg2Engine->getSelfDisplayBidOrderBook(leg2OrderBook);
            impliedOrderBook<AskOrderBook, AskOrderBook, BidOrderBook>(orderBook, iImpliedSize, OPNX::Order::OrderSide::SELL, bestBidItem, &leg1OrderBook, &leg2OrderBook,
                                                                       [&](long long leg1Price, long long leg2Price)->long long{return spotRepoOutPerpAskPrice(leg1Price, leg2Price);});

        }

        void spotRepoOutPerpBidOrderBook(BidOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestAskItem) {
            // spot bid + repo ask -> perp bid
            BidOrderBook leg1OrderBook;
            AskOrderBook leg2OrderBook;
            m_pLeg1Engine->getSelfDisplayBidOrderBook(leg1OrderBook);
            m_pLeg2Engine->getSelfDisplayAskOrderBook(leg2OrderBook);
            impliedOrderBook<BidOrderBook, BidOrderBook, AskOrderBook>(orderBook, iImpliedSize, OPNX::Order::OrderSide::BUY, bestAskItem, &leg1OrderBook, &leg2OrderBook,
                                                                       [&](long long leg1Price, long long leg2Price)->long long{return spotRepoOutPerpBidPrice(leg1Price, leg2Price);});

        }
        void spotPerpOutRepoAskOrderBook(AskOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestBidItem) {
            // spot ask + perp bid -> repo ask
            AskOrderBook leg1OrderBook;
            BidOrderBook leg2OrderBook;
            m_pLeg1Engine->getSelfDisplayAskOrderBook(leg1OrderBook);
            m_pLeg2Engine->getSelfDisplayBidOrderBook(leg2OrderBook);
            impliedOrderBook<AskOrderBook, AskOrderBook, BidOrderBook>(orderBook, iImpliedSize, OPNX::Order::OrderSide::SELL, bestBidItem, &leg1OrderBook, &leg2OrderBook,
                                                                       [&](long long leg1Price, long long leg2Price)->long long{return spotPerpOutRepoAskPrice(leg1Price, leg2Price);});

        }

        void spotPerpOutRepoBidOrderBook(BidOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestAskItem) {
            // spot bid + perp ask -> repo bid
            BidOrderBook leg1OrderBook;
            AskOrderBook leg2OrderBook;
            m_pLeg1Engine->getSelfDisplayBidOrderBook(leg1OrderBook);
            m_pLeg2Engine->getSelfDisplayAskOrderBook(leg2OrderBook);
            impliedOrderBook<BidOrderBook, BidOrderBook, AskOrderBook>(orderBook, iImpliedSize, OPNX::Order::OrderSide::BUY, bestAskItem, &leg1OrderBook, &leg2OrderBook,
                                                                       [&](long long leg1Price, long long leg2Price)->long long{return spotPerpOutRepoBidPrice(leg1Price, leg2Price);});

        }

        void spreadPerpOutFuturesAskOrderBook(AskOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestBidItem) {
            // spread ask + perp ask -> futures ask
            AskOrderBook leg1OrderBook;
            AskOrderBook leg2OrderBook;
            m_pLeg1Engine->getSelfDisplayAskOrderBook(leg1OrderBook);
            m_pLeg2Engine->getSelfDisplayAskOrderBook(leg2OrderBook);
            impliedOrderBook<AskOrderBook, AskOrderBook, AskOrderBook>(orderBook, iImpliedSize, OPNX::Order::OrderSide::SELL, bestBidItem, &leg1OrderBook, &leg2OrderBook,
                                                                       [&](long long leg1Price, long long leg2Price)->long long{return spreadPerpOutFuturesAskPrice(leg1Price, leg2Price);});

        }

        void spreadPerpOutFuturesBidOrderBook(BidOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestAskItem) {
            // spread bid + perp bid -> futures bid
            BidOrderBook leg1OrderBook;
            BidOrderBook leg2OrderBook;
            m_pLeg1Engine->getSelfDisplayBidOrderBook(leg1OrderBook);
            m_pLeg2Engine->getSelfDisplayBidOrderBook(leg2OrderBook);
            impliedOrderBook<BidOrderBook, BidOrderBook, BidOrderBook>(orderBook, iImpliedSize, OPNX::Order::OrderSide::BUY, bestAskItem, &leg1OrderBook, &leg2OrderBook,
                                                                       [&](long long leg1Price, long long leg2Price)->long long{return spreadPerpOutFuturesBidPrice(leg1Price, leg2Price);});

        }

        void futuresSpreadOutPerpAskOrderBook(AskOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestBidItem) {
            // futures ask + spread bid -> perp ask
            AskOrderBook leg1OrderBook;
            BidOrderBook leg2OrderBook;
            m_pLeg1Engine->getSelfDisplayAskOrderBook(leg1OrderBook);
            m_pLeg2Engine->getSelfDisplayBidOrderBook(leg2OrderBook);
            impliedOrderBook<AskOrderBook, AskOrderBook, BidOrderBook>(orderBook, iImpliedSize, OPNX::Order::OrderSide::SELL, bestBidItem, &leg1OrderBook, &leg2OrderBook,
                                                                       [&](long long leg1Price, long long leg2Price)->long long{return futuresSpreadOutPerpAskPrice(leg1Price, leg2Price);});

        }

        void futuresSpreadOutPerpBidOrderBook(BidOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestAskItem) {
            // futures bid + spread ask -> perp bid
            BidOrderBook leg1OrderBook;
            AskOrderBook leg2OrderBook;
            m_pLeg1Engine->getSelfDisplayBidOrderBook(leg1OrderBook);
            m_pLeg2Engine->getSelfDisplayAskOrderBook(leg2OrderBook);
            impliedOrderBook<BidOrderBook, BidOrderBook, AskOrderBook>(orderBook, iImpliedSize, OPNX::Order::OrderSide::BUY, bestAskItem, &leg1OrderBook, &leg2OrderBook,
                                                                       [&](long long leg1Price, long long leg2Price)->long long{return futuresSpreadOutPerpBidPrice(leg1Price, leg2Price);});

        }

        void futuresPerpOutSpreadAskOrderBook(AskOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestBidItem) {
            // futures ask + perp bid -> spread ask
            AskOrderBook leg1OrderBook;
            BidOrderBook leg2OrderBook;
            m_pLeg1Engine->getSelfDisplayAskOrderBook(leg1OrderBook);
            m_pLeg2Engine->getSelfDisplayBidOrderBook(leg2OrderBook);
            impliedOrderBook<AskOrderBook, AskOrderBook, BidOrderBook>(orderBook, iImpliedSize, OPNX::Order::OrderSide::SELL, bestBidItem, &leg1OrderBook, &leg2OrderBook,
                                                                       [&](long long leg1Price, long long leg2Price)->long long{return futuresPerpOutSpreadAskPrice(leg1Price, leg2Price);});

        }

        void futuresPerpOutSpreadBidOrderBook(BidOrderBook& orderBook, int iImpliedSize, const OPNX::OrderBookItem& bestAskItem) {
            // futures bid + perp ask -> spread bid
            BidOrderBook leg1OrderBook;
            AskOrderBook leg2OrderBook;
            m_pLeg1Engine->getSelfDisplayBidOrderBook(leg1OrderBook);
            m_pLeg2Engine->getSelfDisplayAskOrderBook(leg2OrderBook);
            impliedOrderBook<BidOrderBook, BidOrderBook, AskOrderBook>(orderBook, iImpliedSize, OPNX::Order::OrderSide::BUY, bestAskItem, &leg1OrderBook, &leg2OrderBook,
                                                                       [&](long long leg1Price, long long leg2Price)->long long{return futuresPerpOutSpreadBidPrice(leg1Price, leg2Price);});

        }
    };
}
#endif //MATCHING_ENGINE_IMPLIER_H
