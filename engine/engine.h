//
// Created by Bob   on 2021/12/24.
//

#ifndef MATCHING_ENGINE_ENGINE_H
#define MATCHING_ENGINE_ENGINE_H

#include <map>
#include <unordered_map>
#include <vector>
#include <mutex>

#include "IEngine.h"
#include "json.hpp"
#include "order.h"
#include "implier.h"
#include "spin_mutex.hpp"



class Engine: public OPNX::IEngine{
private:
    Engine(const nlohmann::json& jsonMarketInfo, OPNX::ICallbackManager* pCallbackManager)
    : m_jsonMarketInfo(jsonMarketInfo)
    , m_pCallbackManager(pCallbackManager)
    , m_strMarketCode("")
    , m_strType("")
    , m_strReferencePair("")
    , m_ullMarketId(0)
    , m_miniTick(0)
    , m_ullFactor(100000000)
    , m_ullQtyFactor(100000000)
    , m_llMakerFees(0)
    , m_qtyIncrement(0)
    , m_bIsRepo(false)
    , m_iOrderGroupCount(OPNX::ORDER_COUNT)
    {
        m_ullFactor = 100000000;
        m_ullQtyFactor = 100000000;
        m_llMakerFees = 0;  // default is 0
        setMarketInfo(jsonMarketInfo);
    };
    Engine(const Engine &) = delete;
    Engine(Engine &&) = delete;
    Engine &operator=(const Engine &) = delete;
    Engine &operator=(Engine &&) = delete;
    ~Engine();

private:
    OPNX::SearchOrderMap m_unmapSearchOrder;   // save all orders
    OPNX::OrderBookAscendMap m_askOrderBook;                 // key is price, save ask order, in ascending order
    OPNX::OrderBookDescendMap m_bidOrderBook;              // key is price, save bid order, in descending order

    nlohmann::json m_jsonMarketInfo;

    OPNX::ICallbackManager* m_pCallbackManager;

    std::string m_strMarketCode;
    std::string m_strType;
    std::string m_strReferencePair;
    unsigned long long m_ullMarketId;
    long long m_miniTick;
    unsigned long long m_ullFactor;
    unsigned long long m_ullQtyFactor;
    long long m_llMakerFees;
    long long m_qtyIncrement;
    bool m_bIsRepo;
    int m_iOrderGroupCount;

    // implier
    std::vector<OPNX::Implier>  m_vecImpliers;

    mutable OPNX::spin_mutex m_spinMutexOrderBook;

public:
    static OPNX::IEngine* createEngine(const nlohmann::json& jsonMarketInfo, OPNX::ICallbackManager* pCallbackManager);
    virtual void releaseIEngine(){ delete this;};
    virtual void handleOrder(OPNX::Order& order);
    virtual void setImplier(OPNX::Implier& implier)
    {
        if (m_vecImpliers.end() == find(m_vecImpliers.begin(), m_vecImpliers.end(), implier))
        {
            m_vecImpliers.push_back(std::move(implier));
        }
    }
    virtual void eraseImplier(OPNX::IEngine *pEngine)
    {
        auto item = m_vecImpliers.begin();
        while (item != m_vecImpliers.end())
        {
            if ((*item).containEngine(pEngine))
            {
                item = m_vecImpliers.erase(item);
            }
            else
            {
                item ++;
            }
        }
    }

    virtual std::string getMarketCode() { return m_strMarketCode; }
    virtual std::string getType() { return m_strType; }
    virtual std::string getReferencePair() { return m_strReferencePair; }
    virtual long long getMarketId() { return m_ullMarketId; }
    virtual long long getMiniTick() { return m_miniTick; }
    virtual unsigned long long getFactor() { return m_ullFactor; }
    virtual unsigned long long getQtyFactor() { return m_ullQtyFactor; }
    virtual long long getMakerFees() { return m_llMakerFees; }
    virtual void setMakerFees(long long llMakerFees) {
        m_llMakerFees = llMakerFees;
    }
    void updateImpliedMakerFees() {
        for (auto it = m_vecImpliers.begin(); it != m_vecImpliers.end(); it++)
        {
            it->updateMakerFees();
        }
    }

    virtual std::string getExpiryTime() {
        std::string strExpiryTime = "";
        if ("FUTURE" == m_strType || "SPREAD" == m_strType) {
            std::vector<std::string> vecStr;
            OPNX::Utils::stringSplit(vecStr, m_strMarketCode, "-");
            std::string strTime = vecStr[vecStr.size()-2];
            std::regex reg("\\d");
            std::sregex_token_iterator tokens(strTime.begin(), strTime.end(), reg, -1);
            std::sregex_token_iterator end;
            for (; tokens != end; ++tokens) {
                if (!tokens->matched)
                {
                    strExpiryTime += *(tokens->second);
                }
            }
        }
        return strExpiryTime;
    }

    virtual OPNX::OrderBookItem getSelfBestAsk();
    virtual OPNX::OrderBookItem getSelfBestBid();
    virtual OPNX::OrderBookItem getBestAsk(const OPNX::OrderBookItem* pBestBidObItem=nullptr);
    virtual OPNX::OrderBookItem getBestBid(const OPNX::OrderBookItem* pBestAskObItem=nullptr);
    virtual void getSelfDisplayAskOrderBook(AskOrderBook& askOrderBook, int iSize= 400);
    virtual void getSelfDisplayBidOrderBook(BidOrderBook& bidOrderBook, int iSize= 400);
    virtual void getDisplayAskOrderBook(AskOrderBook& askOrderBook, int iSize= 400, int iImpliedSize= 20, const OPNX::OrderBookItem* pBestBidObItem=nullptr);
    virtual void getDisplayBidOrderBook(BidOrderBook& bidOrderBook, int iSize= 400, int iImpliedSize= 20, const OPNX::OrderBookItem* pBestAskObItem=nullptr);
    virtual  OPNX::OrderBookAscendMap* getAskOrderBook() { return &m_askOrderBook; }
    virtual  OPNX::OrderBookDescendMap* getBidOrderBook() { return &m_bidOrderBook; }
    virtual unsigned long long getAskOrderBookSize(){ return m_askOrderBook.size(); }
    virtual unsigned long long getBidOrderBookSize(){ return m_bidOrderBook.size(); }
    virtual unsigned long long getOrdersCount(){ return m_unmapSearchOrder.size(); }
    virtual OPNX::Order* getBestAskOrder();
    virtual OPNX::Order* getBestBidOrder();
    virtual unsigned long long getOrderMatchableQuantity(const OPNX::Order* pOrder);
    virtual void matchOrder(std::vector<OPNX::Order>& vecMatchedOrder, OPNX::Order& takerOrder, OPNX::Order* pMakerOrder,
                            long long llMatchedPrice, unsigned long long ullMatchQuantity, unsigned long long ullMatchedId,
                            OPNX::Order* pThirdOrder = nullptr, bool bHandleTakerOrder=true, bool bHasRepo = false);

    virtual void setMarketInfo(const nlohmann::json& jsonMarketInfo);
    virtual void getMarketInfo(nlohmann::json& jsonMarketInfo) { jsonMarketInfo = m_jsonMarketInfo; };
    virtual void clearOrder();
    virtual void setOrderGroupCount(int iOrderGroupCount) { m_iOrderGroupCount = iOrderGroupCount; }

    OPNX::SortOrderBook* getBestAskSortOrderBook();
    OPNX::SortOrderBook* getBestBidSortOrderBook();

private:
    void handleNewOrder(OPNX::Order& order);
    void handleCancelOrder(OPNX::Order& order);
    void handleAmendOrder(OPNX::Order& order);
    inline OPNX::Order* saveToSearchOrder(OPNX::Order& order);
    void saveNewOrder(OPNX::Order* order);
    void handleBracketOrder(OPNX::Order* pBracketOrder);

    unsigned long long getAskMatchableAmount(const OPNX::Order& order);
    unsigned long long getBidMatchableAmount(const OPNX::Order& order);
    unsigned long long getImpliedAskMatchableAmount(long long limitPrice, const OPNX::OrderBookItem& bestBidItem);
    unsigned long long getImpliedBidMatchableAmount(long long limitPrice, const OPNX::OrderBookItem& bestAskItem);

    inline void eraseFromSearchOrderMap(const OPNX::Order& order);

    inline bool checkBestChange(const OPNX::Order& order);
    void updateOrderBook(const OPNX::Order& newOrder, const OPNX::Order& oldOrder);
    template<class SortOrderBookMap>
    void updateOrderBook(SortOrderBookMap& sortOrderBookMap, const OPNX::Order& newOrder, const OPNX::Order& oldOrder);

    inline unsigned long long amountToMatchableQuantity(unsigned long long ullAmount, OPNX::Order::OrderSide side);

    inline void logErrorOrder(const std::string& strError, const OPNX::Order& order);
};


#endif //MATCHING_ENGINE_ENGINE_H
