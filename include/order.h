#ifndef MATCHING_INC_ORDER_HPP_
#define MATCHING_INC_ORDER_HPP_

#include <limits>
#include <vector>
#include <sstream>

#include "json.hpp"
#include "rapidjson/document.h"
#include "utils.h"
#include "log.h"



namespace OPNX {
    // Split group orders
    const int ORDER_COUNT = 10;

    // json key of order response
    static const char* key_payloadType = "pt";            // json key is pt
    static const char* key_orderList = "ol";              // json key is mid
    // json key of order
    static const char* key_accountId = "aid";             // json key is aid
    static const char* key_marketId = "mid";              // json key is mid
    static const char* key_price = "p";                   // json key is p
    static const char* key_quantity = "q";                // json key is q
    static const char* key_displayQuantity = "dq";        // json key is dq
    static const char* key_remainQuantity = "rq";         // json key is rq
    static const char* key_amount = "a";                  // json key is a
    static const char* key_remainAmount = "ra";           // json key is ra
    static const char* key_upperBound = "ub";             // json key is ub
    static const char* key_lowerBound = "lb";             // json key is lb
    static const char* key_lastMatchPrice = "mp";         // json key is mp
    static const char* key_leg2Price = "l2p";             // json key is l2p
    static const char* key_lastMatchQuantity = "mq";      // json key is mq
    static const char* key_orderId = "id";                // json key is id
    static const char* key_clientOrderId = "cid";         // json key is cid
    static const char* key_lastMatchedOrderId = "lmid";   // json key is lmid
    static const char* key_lastMatchedOrderId2 = "lmid2"; // json key is lmid2
    static const char* key_matchedId = "mtid";            // json key is mtid
    static const char* key_sortId = "sid";                // json key is sid
    static const char* key_bracketOrderId = "bid";        // json key is bid
    static const char* key_takeProfitOrderId = "tpid";    // json key is tpid
    static const char* key_stopLossOrderId = "slid";      // json key is slid
    static const char* key_triggerPrice = "tp";           // json key is tp
    static const char* key_isTriggered = "it";            // json key is it
    static const char* key_triggerType = "tt";            // json key is tt
    static const char* key_side = "s";                    // json key is s
    static const char* key_type = "ty";                   // json key is ty
    static const char* key_stopCondition = "stc";         // json key is stc
    static const char* key_timeCondition = "tc";          // json key is tc
    static const char* key_action = "ac";                 // json key is ac
    static const char* key_status = "st";                 // json key is st
    static const char* key_matchedType = "mt";            // json key is mt
    static const char* key_timestamp = "t";               // json key is t
    static const char* key_orderCreated = "oc";           // json key is oc
    static const char* key_source = "sc";                 // json key is sc
    static const char* key_tag = "tag";                   // json key is tag
    static const char* key_selfTradeProtection = "stp";   // json key is stp

    struct Order {
    public:
        const static long long MAX_PRICE = std::numeric_limits<long long>::max();
        const static long long MIN_PRICE = std::numeric_limits<long long>::min();
    public:
        enum OrderSide : unsigned char {
            BUY = 0x00,
            SELL = 0x01,
        };
        enum OrderType : unsigned char {
            LIMIT = 0x00,
            MARKET = 0x01,
            STOP_LIMIT = 0x02,
            STOP_MARKET = 0x03,
            TAKE_PROFIT_LIMIT = 0x04,
            TAKE_PROFIT_MARKET = 0x05,
        };
        enum OrderStopCondition : unsigned char {
            NONE = 0x00,
            GREATER_EQUAL = 0x01,
            LESS_EQUAL = 0x02,
        };
        enum OrderActionType : unsigned char {
            NEW = 0x00,
            AMEND = 0x01,
            CANCEL = 0x02,
            RECOVERY = 0x03,
            RECOVERY_END = 0x04,
            TRIGGER_BRACKET = 0x05,
        };
        enum OrderTimeCondition : unsigned char {
            GTC = 0x00,
            IOC = 0x01,
            FOK = 0x02,
            MAKER_ONLY = 0x03,
            MAKER_ONLY_REPRICE = 0x04,
            AUCTION = 0x05,
        };
        enum TriggerType : unsigned char {
            MARK_PRICE,
            LAST_PRICE,
            TRIGGER_NONE,
        };
        enum SelfTradeProtectionType : unsigned char {
            STP_NONE,
            STP_TAKER,
            STP_MAKER,
            STP_BOTH,
        };
        enum OrderStatusType : unsigned char {
            OPEN = 0x00,
            PARTIAL_FILL,
            FILLED,
            CANCELED_BY_USER,
            CANCELED_BY_MARKET_ORDER_NOT_FULL_MATCHED,
            CANCELED_BY_MARKET_ORDER_NOTHING_MATCH,
            CANCELED_ALL_BY_IOC,
            CANCELED_PARTIAL_BY_IOC,
            CANCELED_BY_FOK,
            CANCELED_BY_MAKER_ONLY,
            CANCELED_ALL_BY_AUCTION,
            CANCELED_PARTIAL_BY_AUCTION,
            CANCELED_BY_AMEND,
            CANCELED_BY_NO_AUCTION,
            CANCELED_BY_PRE_AUCTION,
            CANCELED_BY_BRACKET_ORDER,
            CANCELED_BY_SELF_TRADE_PROTECTION,
            REJECT_CANCEL_ORDER_ID_NOT_FOUND,
            REJECT_AMEND_ORDER_ID_NOT_FOUND,
            REJECT_DISPLAY_QUANTITY_ZERO,
            REJECT_DISPLAY_QUANTITY_LARGER_THAN_QUANTITY,
            REJECT_BUY_STOP_TRIGGER_LARGE_THAN_STOP_LIMIT,
            REJECT_SELL_STOP_TRIGGER_LESS_THAN_STOP_LIMIT,
            REJECT_UNKNOWN_ORDER_ACTION,
            REJECT_QUANTITY_AND_AMOUNT_ZERO,
            REJECT_LIMIT_ORDER_WITH_MARKET_PRICE,
            REJECT_AUCTION_SUPPORT_BUY_SELL_ONLY,
            REJECT_ORDER_AMENDING_OR_CANCELING,
            REJECT_MATCHING_ENGINE_RECOVERING,
            REJECT_STOP_CONDITION_IS_NONE,
            REJECT_STOP_TRIGGER_PRICE_IS_NONE,
            REJECT_QUANTITY_AND_AMOUNT_LARGER_ZERO,
            REJECT_AMEND_ORDER_IS_TRIGGERED,
            REJECT_AMEND_NEW_QUANTITY_IS_LESS_THAN_MATCHED_QUANTITY,
        };
        enum OrderMatchedType : unsigned char {
            MAKER = 0x00,
            TAKER = 0x01
        };
    public:
        unsigned long long accountId;
        unsigned long long marketId;
        long long price;
        unsigned long long quantity;
        unsigned long long displayQuantity;   // Iceberg order: displayQuantity < quantity
        unsigned long long remainQuantity;
        unsigned long long amount;
        unsigned long long remainAmount;
        long long upperBound;
        long long lowerBound;
        long long lastMatchPrice;
        long long leg2Price;
        unsigned long long lastMatchQuantity;
        unsigned long long orderId;
        unsigned long long clientOrderId;
        unsigned long long lastMatchedOrderId;
        unsigned long long lastMatchedOrderId2;
        unsigned long long matchedId;
        unsigned long long sortId;
        unsigned long long bracketOrderId;      // bracket order
        unsigned long long takeProfitOrderId;   // bracket take Profit order
        unsigned long long stopLossOrderId;     // bracket stop Loss order
        long long triggerPrice;
        unsigned long long timestamp;
        unsigned long long orderCreated;   // order created timestamp
        int source; // unused, leslie generation
        bool isTriggered;
        TriggerType triggerType;
        OrderSide side;
        OrderType type;
        OrderStopCondition stopCondition;
        OrderTimeCondition timeCondition;
        OrderActionType action;
        OrderStatusType status;
        OrderMatchedType matchedType;
        SelfTradeProtectionType selfTradeProtectionType;
        char tag[64];

    public:
        Order() :
                accountId(0),
                marketId(0),
                price(MAX_PRICE),
                quantity(0),
                displayQuantity(0),
                remainQuantity(0),
                amount(0),
                remainAmount(0),
                upperBound(MAX_PRICE),
                lowerBound(MIN_PRICE),
                lastMatchPrice(0),
                leg2Price(0),
                lastMatchQuantity(0),
                orderId(0),
                clientOrderId(0),
                lastMatchedOrderId(0),
                lastMatchedOrderId2(0),
                matchedId(0),
                sortId(0),
                bracketOrderId(0),
                takeProfitOrderId(0),
                stopLossOrderId(0),
                triggerPrice(MAX_PRICE),
                timestamp(0),
                orderCreated(0),
                source(0),
                isTriggered(false),
                triggerType(TriggerType::TRIGGER_NONE),
                side(OrderSide::BUY),
                type(OrderType::LIMIT),
                stopCondition(OrderStopCondition::NONE),
                timeCondition(OrderTimeCondition::GTC),
                action(OrderActionType::NEW),
                status(OrderStatusType::OPEN),
                matchedType(OrderMatchedType::MAKER),
                selfTradeProtectionType(SelfTradeProtectionType::STP_NONE),
                tag{0} {
        }

        Order(const Order &) = default;

        Order(Order &&) = default;

        Order &operator=(const Order &) = default;

        Order &operator=(Order &&) = default;

        ~Order() = default;

        static inline bool canAmend(const Order &amendOrder, const Order &initOrder) {
            if (amendOrder.side != initOrder.side) {
                return false;
            }
            if (OrderType::LIMIT == initOrder.type || OrderType::MARKET == initOrder.type) {
                if (amendOrder.price != initOrder.price) {
                    return false;
                }
            }
            if (OrderType::STOP_LIMIT == initOrder.type || OrderType::STOP_MARKET == initOrder.type) {
                if (initOrder.isTriggered)
                {
                    if (amendOrder.price != initOrder.price)
                    {
                        return false;
                    }
                }
                else
                {
                    if (amendOrder.triggerPrice != initOrder.triggerPrice)
                    {
                        return false;
                    }
                    return true;
                }
            }
            if (amendOrder.quantity > initOrder.quantity)
                return false;
            return true;
        }

        static void orderToJsonString0(const OPNX::Order& order, std::string& jsonOrderString)
        {
            OPNX::CInOutLog cInOutLog("orderToJsonString");

            std::string triggerType = "";
            switch (order.triggerType) {
                case TriggerType::MARK_PRICE      : triggerType = "MARK_PRICE"    ; break;
                case TriggerType::LAST_PRICE      : triggerType = "LAST_PRICE"    ; break;
                case TriggerType::TRIGGER_NONE    : triggerType = "NONE"          ; break;
                default: break;
            }

            std::string type = "";
            switch (order.type) {
                case OrderType::LIMIT      : type = "LIMIT"     ; break;
                case OrderType::MARKET     : type = "MARKET"    ; break;
                case OrderType::STOP_LIMIT : type = "STOP_LIMIT"; break;
                case OrderType::STOP_MARKET : type = "STOP_MARKET"; break;
                case OrderType::TAKE_PROFIT_LIMIT : type = "TAKE_PROFIT_LIMIT"; break;
                case OrderType::TAKE_PROFIT_MARKET : type = "TAKE_PROFIT_MARKET"; break;
                default: break;
            }

            std::string side = "";
            switch (order.side) {
                case OrderSide::BUY      : side = "BUY"     ; break;
                case OrderSide::SELL     : side = "SELL"    ; break;
                default: break;
            }

            std::string stopCondition = "";
            switch (order.stopCondition) {
                case OrderStopCondition::NONE            : stopCondition = "NONE"     ; break;
                case OrderStopCondition::GREATER_EQUAL   : stopCondition = "GREATER_EQUAL" ; break;
                case OrderStopCondition::LESS_EQUAL      : stopCondition = "LESS_EQUAL"; break;
                default: break;
            }

            std::string action = "";
            switch (order.action) {
                case NEW         : action = "NEW"         ; break;
                case AMEND       : action = "AMEND"       ; break;
                case CANCEL      : action = "CANCEL"      ; break;
                case RECOVERY    : action = "RECOVERY"    ; break;
                case RECOVERY_END: action = "RECOVERY_END"; break;
                case TRIGGER_BRACKET: action = "TRIGGER_BRACKET"; break;
                default: break;
            }

            std::string timeCondition = "";
            switch (order.timeCondition) {
                case GTC               : timeCondition = "GTC"; break;
                case IOC               : timeCondition = "IOC"; break;
                case FOK               : timeCondition = "FOK"; break;
                case MAKER_ONLY        : timeCondition = "MAKER_ONLY"; break;
                case MAKER_ONLY_REPRICE: timeCondition = "MAKER_ONLY_REPRICE"; break;
                case AUCTION           : timeCondition = "AUCTION"; break;
                default: break;
            }

            std::string selfTradeProtectionType = "";
            switch (order.selfTradeProtectionType) {
                case STP_NONE       : selfTradeProtectionType = "NONE"       ; break;
                case STP_TAKER      : selfTradeProtectionType = "EXPIRE_TAKER"      ; break;
                case STP_MAKER      : selfTradeProtectionType = "EXPIRE_MAKER"    ; break;
                case STP_BOTH       : selfTradeProtectionType = "EXPIRE_BOTH"; break;
                default: break;
            }

            std::string status = "";
            switch (order.status) {
                case OPEN                                           : status = "OPEN"; break;
                case PARTIAL_FILL                                   : status = "PARTIAL_FILL"; break;
                case FILLED                                         : status = "FILLED"; break;
                case CANCELED_BY_USER                               : status = "CANCELED_BY_USER"; break;
                case CANCELED_BY_MARKET_ORDER_NOT_FULL_MATCHED      : status = "CANCELED_BY_MARKET_ORDER_NOT_FULL_MATCHED"; break;
                case CANCELED_BY_MARKET_ORDER_NOTHING_MATCH         : status = "CANCELED_BY_MARKET_ORDER_NOTHING_MATCH"; break;
                case CANCELED_ALL_BY_IOC                            : status = "CANCELED_ALL_BY_IOC"; break;
                case CANCELED_PARTIAL_BY_IOC                        : status = "CANCELED_PARTIAL_BY_IOC"; break;
                case CANCELED_BY_FOK                                : status = "CANCELED_BY_FOK"; break;
                case CANCELED_BY_MAKER_ONLY                         : status = "CANCELED_BY_MAKER_ONLY"; break;
                case CANCELED_ALL_BY_AUCTION                        : status = "CANCELED_ALL_BY_AUCTION"; break;
                case CANCELED_PARTIAL_BY_AUCTION                    : status = "CANCELED_PARTIAL_BY_AUCTION"; break;
                case CANCELED_BY_AMEND                              : status = "CANCELED_BY_AMEND"; break;
                case CANCELED_BY_NO_AUCTION                         : status = "CANCELED_BY_NO_AUCTION"; break;
                case CANCELED_BY_PRE_AUCTION                        : status = "CANCELED_BY_PRE_AUCTION"; break;
                case CANCELED_BY_BRACKET_ORDER                      : status = "CANCELED_BY_BRACKET_ORDER"; break;
                case REJECT_CANCEL_ORDER_ID_NOT_FOUND               : status = "REJECT_CANCEL_ORDER_ID_NOT_FOUND"; break;
                case REJECT_AMEND_ORDER_ID_NOT_FOUND                : status = "REJECT_AMEND_ORDER_ID_NOT_FOUND"; break;
                case REJECT_DISPLAY_QUANTITY_ZERO                   : status = "REJECT_DISPLAY_QUANTITY_ZERO"; break;
                case REJECT_DISPLAY_QUANTITY_LARGER_THAN_QUANTITY   : status = "REJECT_DISPLAY_QUANTITY_LARGER_THAN_QUANTITY"; break;
                case REJECT_BUY_STOP_TRIGGER_LARGE_THAN_STOP_LIMIT  : status = "REJECT_BUY_STOP_TRIGGER_LARGE_THAN_STOP_LIMIT"; break;
                case REJECT_SELL_STOP_TRIGGER_LESS_THAN_STOP_LIMIT  : status = "REJECT_SELL_STOP_TRIGGER_LESS_THAN_STOP_LIMIT"; break;
                case REJECT_UNKNOWN_ORDER_ACTION                    : status = "REJECT_UNKNOWN_ORDER_ACTION"; break;
                case REJECT_QUANTITY_AND_AMOUNT_ZERO                : status = "REJECT_QUANTITY_AND_AMOUNT_ZERO"; break;
                case REJECT_LIMIT_ORDER_WITH_MARKET_PRICE           : status = "REJECT_LIMIT_ORDER_WITH_MARKET_PRICE"; break;
                case REJECT_AUCTION_SUPPORT_BUY_SELL_ONLY           : status = "REJECT_AUCTION_SUPPORT_BUY_SELL_ONLY"; break;
                case REJECT_ORDER_AMENDING_OR_CANCELING             : status = "REJECT_ORDER_AMENDING_OR_CANCELING"; break;
                case REJECT_MATCHING_ENGINE_RECOVERING              : status = "REJECT_MATCHING_ENGINE_RECOVERING"; break;
                case REJECT_STOP_CONDITION_IS_NONE                  : status = "REJECT_STOP_CONDITION_IS_NONE"; break;
                case REJECT_STOP_TRIGGER_PRICE_IS_NONE              : status = "REJECT_STOP_TRIGGER_PRICE_IS_NONE"; break;
                case REJECT_QUANTITY_AND_AMOUNT_LARGER_ZERO         : status = "REJECT_QUANTITY_AND_AMOUNT_LARGER_ZERO"; break;
                default: break;
            }

            std::string matchedType = "";
            switch (order.matchedType) {
                case MAKER : matchedType = "MAKER"; break;
                case TAKER : matchedType = "TAKER"; break;
                default: break;
            }
            unsigned long long timestamp = (0 != order.timestamp ? order.timestamp : Utils::getMilliTimestamp());
            std::string isTriggered = (order.isTriggered ? "true" : "false");

            std::stringstream ssOrder;
            ssOrder << "{\"aid\":" << order.accountId << ",\"mid\":" << order.marketId
                    << ",\"p\":" << order.price << ",\"q\":" << order.quantity
                    << ",\"dq\":" << order.displayQuantity << ",\"rq\":" << order.remainQuantity
                    << ",\"q\":" << order.amount << ",\"ra\":" << order.remainAmount
//                    << ",\"ub\":" << order.upperBound << ",\"lb\":" << order.lowerBound
                    << ",\"mp\":" << order.lastMatchPrice<< ",\"l2p\":" << order.leg2Price << ",\"mq\":" << order.lastMatchQuantity
                    << ",\"id\":" << order.orderId << ",\"cid\":" << order.clientOrderId
                    << ",\"lmid\":" << order.lastMatchedOrderId << ",\"lmid2\":" << order.lastMatchedOrderId2
                    << ",\"mtid\":" << order.matchedId << ",\"sid\":" << order.sortId
                    << ",\"bid\":" << order.bracketOrderId << ",\"tpid\":" << order.takeProfitOrderId
                    << ",\"slid\":" << order.stopLossOrderId
                    << ",\"tp\":" << order.triggerPrice << ",\"it\":" << isTriggered
                    << ",\"t\":" << timestamp << ",\"oc\":" << order.orderCreated
                    << ",\"sc\":" << order.source << ",\"tag\":\"" << order.tag
                    << "\",\"tt\":\"" << triggerType << "\",\"s\":\"" << side
                    << "\",\"ty\":\"" << type << "\",\"stc\":\"" << stopCondition
                    << "\",\"tc\":\"" << timeCondition << "\",\"ac\":\"" << action
                    << "\",\"st\":\"" << status << "\",\"mt\":\"" << matchedType << "\",\"stp\":\"" << selfTradeProtectionType
                    << "\"}";
//            ssOrder << "{\"accountId\":" << order.accountId << ",\"key_marketId\":" << order.marketId
//                    << ",\"price\":" << order.price << ",\"quantity\":" << order.quantity
//                    << ",\"displayQuantity\":" << order.displayQuantity << ",\"remainQuantity\":" << order.remainQuantity
//                    << ",\"amount\":" << order.amount << ",\"remainAmount\":" << order.remainAmount
//                    << ",\"upperBound\":" << order.upperBound << ",\"lowerBound\":" << order.lowerBound
//                    << ",\"lastMatchPrice\":" << order.lastMatchPrice<< ",\"leg2Price\":" << order.leg2Price << ",\"lastMatchQuantity\":" << order.lastMatchQuantity
//                    << ",\"orderId\":" << order.orderId << ",\"clientOrderId\":" << order.clientOrderId
//                    << ",\"lastMatchedOrderId\":" << order.lastMatchedOrderId << ",\"lastMatchedOrderId2\":" << order.lastMatchedOrderId2
//                    << ",\"matchedId\":" << order.matchedId << ",\"sortId\":" << order.sortId
//                    << ",\"bracketOrderId\":" << order.bracketOrderId << ",\"takeProfitOrderId\":" << order.takeProfitOrderId
//                    << ",\"stopLossOrderId\":" << order.stopLossOrderId
//                    << ",\"triggerPrice\":" << order.triggerPrice << ",\"isTriggered\":" << isTriggered
//                    << ",\"timestamp\":" << timestamp << ",\"orderCreated\":" << order.orderCreated
//                    << ",\"source\":" << order.source << ",\"tag\":\"" << order.tag
//                    << "\",\"triggerType\":\"" << triggerType << "\",\"side\":\"" << side
//                    << "\",\"type\":\"" << type << "\",\"stopCondition\":\"" << stopCondition
//                    << "\",\"timeCondition\":\"" << timeCondition << "\",\"action\":\"" << action
//                    << "\",\"status\":\"" << status << "\",\"matchedType\":\"" << matchedType
//                    << "\"}";

            jsonOrderString = ssOrder.str();
        }

        static void orderToJsonString(const OPNX::Order& order, std::string& jsonOrderString)
        {
//            OPNX::CInOutLog cInOutLog("orderToJsonString");

            std::string triggerType = "";
            switch (order.triggerType) {
                case TriggerType::MARK_PRICE      : triggerType = "MARK_PRICE"    ; break;
                case TriggerType::LAST_PRICE      : triggerType = "LAST_PRICE"    ; break;
                case TriggerType::TRIGGER_NONE    : triggerType = "NONE"          ; break;
                default: break;
            }

            std::string type = "";
            switch (order.type) {
                case OrderType::LIMIT      : type = "LIMIT"     ; break;
                case OrderType::MARKET     : type = "MARKET"    ; break;
                case OrderType::STOP_LIMIT : type = "STOP_LIMIT"; break;
                case OrderType::STOP_MARKET : type = "STOP_MARKET"; break;
                case OrderType::TAKE_PROFIT_LIMIT : type = "TAKE_PROFIT_LIMIT"; break;
                case OrderType::TAKE_PROFIT_MARKET : type = "TAKE_PROFIT_MARKET"; break;
                default: break;
            }

            std::string side = "";
            switch (order.side) {
                case OrderSide::BUY      : side = "BUY"     ; break;
                case OrderSide::SELL     : side = "SELL"    ; break;
                default: break;
            }

            std::string stopCondition = "";
            switch (order.stopCondition) {
                case OrderStopCondition::NONE            : stopCondition = "NONE"     ; break;
                case OrderStopCondition::GREATER_EQUAL   : stopCondition = "GREATER_EQUAL" ; break;
                case OrderStopCondition::LESS_EQUAL      : stopCondition = "LESS_EQUAL"; break;
                default: break;
            }

            std::string action = "";
            switch (order.action) {
                case NEW         : action = "NEW"         ; break;
                case AMEND       : action = "AMEND"       ; break;
                case CANCEL      : action = "CANCEL"      ; break;
                case RECOVERY    : action = "RECOVERY"    ; break;
                case RECOVERY_END: action = "RECOVERY_END"; break;
                case TRIGGER_BRACKET: action = "TRIGGER_BRACKET"; break;
                default: break;
            }

            std::string timeCondition = "";
            switch (order.timeCondition) {
                case GTC               : timeCondition = "GTC"; break;
                case IOC               : timeCondition = "IOC"; break;
                case FOK               : timeCondition = "FOK"; break;
                case MAKER_ONLY        : timeCondition = "MAKER_ONLY"; break;
                case MAKER_ONLY_REPRICE: timeCondition = "MAKER_ONLY_REPRICE"; break;
                case AUCTION           : timeCondition = "AUCTION"; break;
                default: break;
            }

            std::string selfTradeProtectionType = "";
            switch (order.selfTradeProtectionType) {
                case STP_NONE       : selfTradeProtectionType = "NONE"       ; break;
                case STP_TAKER      : selfTradeProtectionType = "EXPIRE_TAKER"      ; break;
                case STP_MAKER      : selfTradeProtectionType = "EXPIRE_MAKER"    ; break;
                case STP_BOTH       : selfTradeProtectionType = "EXPIRE_BOTH"; break;
                default: break;
            }

            std::string status = "";
            switch (order.status) {
                case OPEN                                           : status = "OPEN"; break;
                case PARTIAL_FILL                                   : status = "PARTIAL_FILL"; break;
                case FILLED                                         : status = "FILLED"; break;
                case CANCELED_BY_USER                               : status = "CANCELED_BY_USER"; break;
                case CANCELED_BY_MARKET_ORDER_NOT_FULL_MATCHED      : status = "CANCELED_BY_MARKET_ORDER_NOT_FULL_MATCHED"; break;
                case CANCELED_BY_MARKET_ORDER_NOTHING_MATCH         : status = "CANCELED_BY_MARKET_ORDER_NOTHING_MATCH"; break;
                case CANCELED_ALL_BY_IOC                            : status = "CANCELED_ALL_BY_IOC"; break;
                case CANCELED_PARTIAL_BY_IOC                        : status = "CANCELED_PARTIAL_BY_IOC"; break;
                case CANCELED_BY_FOK                                : status = "CANCELED_BY_FOK"; break;
                case CANCELED_BY_MAKER_ONLY                         : status = "CANCELED_BY_MAKER_ONLY"; break;
                case CANCELED_ALL_BY_AUCTION                        : status = "CANCELED_ALL_BY_AUCTION"; break;
                case CANCELED_PARTIAL_BY_AUCTION                    : status = "CANCELED_PARTIAL_BY_AUCTION"; break;
                case CANCELED_BY_AMEND                              : status = "CANCELED_BY_AMEND"; break;
                case CANCELED_BY_NO_AUCTION                         : status = "CANCELED_BY_NO_AUCTION"; break;
                case CANCELED_BY_PRE_AUCTION                        : status = "CANCELED_BY_PRE_AUCTION"; break;
                case CANCELED_BY_BRACKET_ORDER                      : status = "CANCELED_BY_BRACKET_ORDER"; break;
                case CANCELED_BY_SELF_TRADE_PROTECTION              : status = "CANCELED_BY_SELF_TRADE_PROTECTION"; break;
                case REJECT_CANCEL_ORDER_ID_NOT_FOUND               : status = "REJECT_CANCEL_ORDER_ID_NOT_FOUND"; break;
                case REJECT_AMEND_ORDER_ID_NOT_FOUND                : status = "REJECT_AMEND_ORDER_ID_NOT_FOUND"; break;
                case REJECT_DISPLAY_QUANTITY_ZERO                   : status = "REJECT_DISPLAY_QUANTITY_ZERO"; break;
                case REJECT_DISPLAY_QUANTITY_LARGER_THAN_QUANTITY   : status = "REJECT_DISPLAY_QUANTITY_LARGER_THAN_QUANTITY"; break;
                case REJECT_BUY_STOP_TRIGGER_LARGE_THAN_STOP_LIMIT  : status = "REJECT_BUY_STOP_TRIGGER_LARGE_THAN_STOP_LIMIT"; break;
                case REJECT_SELL_STOP_TRIGGER_LESS_THAN_STOP_LIMIT  : status = "REJECT_SELL_STOP_TRIGGER_LESS_THAN_STOP_LIMIT"; break;
                case REJECT_UNKNOWN_ORDER_ACTION                    : status = "REJECT_UNKNOWN_ORDER_ACTION"; break;
                case REJECT_QUANTITY_AND_AMOUNT_ZERO                : status = "REJECT_QUANTITY_AND_AMOUNT_ZERO"; break;
                case REJECT_LIMIT_ORDER_WITH_MARKET_PRICE           : status = "REJECT_LIMIT_ORDER_WITH_MARKET_PRICE"; break;
                case REJECT_AUCTION_SUPPORT_BUY_SELL_ONLY           : status = "REJECT_AUCTION_SUPPORT_BUY_SELL_ONLY"; break;
                case REJECT_ORDER_AMENDING_OR_CANCELING             : status = "REJECT_ORDER_AMENDING_OR_CANCELING"; break;
                case REJECT_MATCHING_ENGINE_RECOVERING              : status = "REJECT_MATCHING_ENGINE_RECOVERING"; break;
                case REJECT_STOP_CONDITION_IS_NONE                  : status = "REJECT_STOP_CONDITION_IS_NONE"; break;
                case REJECT_STOP_TRIGGER_PRICE_IS_NONE              : status = "REJECT_STOP_TRIGGER_PRICE_IS_NONE"; break;
                case REJECT_QUANTITY_AND_AMOUNT_LARGER_ZERO         : status = "REJECT_QUANTITY_AND_AMOUNT_LARGER_ZERO"; break;
                default: break;
            }

            std::string matchedType = "";
            switch (order.matchedType) {
                case MAKER : matchedType = "MAKER"; break;
                case TAKER : matchedType = "TAKER"; break;
                default: break;
            }
            unsigned long long timestamp = (0 != order.timestamp ? order.timestamp : Utils::getMilliTimestamp());
            std::string isTriggered = (order.isTriggered ? "true" : "false");

            char szJsonOrder[1024];
            memset(szJsonOrder, 0, 1024);

            snprintf(szJsonOrder, 1024,
                     "{\"aid\":%llu,\"mid\":%llu,"
                     "\"p\":%lld,\"q\":%llu,"
                     "\"dq\":%llu,\"rq\":%llu,"
                     "\"a\":%llu,\"ra\":%llu,"
//                     "\"ub\":%lld,\"lb\":%lld,"
                     "\"mp\":%lld,\"l2p\":%lld,\"mq\":%llu,"
                     "\"id\":%llu,\"cid\":%llu,"
                     "\"lmid\":%llu,\"lmid2\":%llu,"
                     "\"mtid\":%llu,\"sid\":%llu,"
                     "\"bid\":%llu,\"tpid\":%llu,\"slid\":%llu,"
                     "\"tp\":%lld,\"it\":%s,"
                     "\"t\":%llu,\"oc\":%llu,"
                     "\"sc\":%d,\"tag\":\"%s\","
                     "\"tt\":\"%s\",\"s\":\"%s\","
                     "\"ty\":\"%s\",\"stc\":\"%s\","
                     "\"tc\":\"%s\",\"ac\":\"%s\","
                     "\"st\":\"%s\",\"mt\":\"%s\",\"stp\":\"%s\"}",
                     order.accountId, order.marketId,
                     order.price, order.quantity,
                     order.displayQuantity, order.remainQuantity,
                     order.amount, order.remainAmount,
//                     order.upperBound, order.lowerBound,
                     order.lastMatchPrice, order.leg2Price, order.lastMatchQuantity,
                     order.orderId, order.clientOrderId,
                     order.lastMatchedOrderId, order.lastMatchedOrderId2,
                     order.matchedId, order.sortId,
                     order.bracketOrderId, order.takeProfitOrderId, order.stopLossOrderId,
                     order.triggerPrice, isTriggered.c_str(),
                     timestamp, order.orderCreated,
                     order.source, order.tag,
                     triggerType.c_str(), side.c_str(),
                     type.c_str(), stopCondition.c_str(),
                     timeCondition.c_str(), action.c_str(),
                     status.c_str(), matchedType.c_str(), selfTradeProtectionType.c_str());
//            snprintf(szJsonOrder, 1024,
//                     "{\"accountId\":%llu,\"marketId\":%llu,"
//                     "\"price\":%lld,\"quantity\":%llu,"
//                     "\"displayQuantity\":%llu,\"remainQuantity\":%llu,"
//                     "\"amount\":%llu,\"remainAmount\":%llu,"
//                     "\"upperBound\":%lld,\"lowerBound\":%lld,"
//                     "\"lastMatchPrice\":%lld,\"leg2Price\":%lld,\"lastMatchQuantity\":%llu,"
//                     "\"orderId\":%llu,\"clientOrderId\":%llu,"
//                     "\"lastMatchedOrderId\":%llu,\"lastMatchedOrderId2\":%llu,"
//                     "\"matchedId\":%llu,\"sortId\":%llu,"
//                     "\"bracketOrderId\":%llu,\"takeProfitOrderId\":%llu,\"stopLossOrderId\":%llu,"
//                     "\"triggerPrice\":%lld,\"isTriggered\":%s,"
//                     "\"timestamp\":%llu,\"orderCreated\":%llu,"
//                     "\"source\":%d,\"tag\":\"%s\","
//                     "\"triggerType\":\"%s\",\"side\":\"%s\","
//                     "\"type\":\"%s\",\"stopCondition\":\"%s\","
//                     "\"timeCondition\":\"%s\",\"action\":\"%s\","
//                     "\"status\":\"%s\",\"matchedType\":\"%s\"}",
//                     order.accountId, order.marketId,
//                     order.price, order.quantity,
//                     order.displayQuantity, order.remainQuantity,
//                     order.amount, order.remainAmount,
//                     order.upperBound, order.lowerBound,
//                     order.lastMatchPrice, order.leg2Price, order.lastMatchQuantity,
//                     order.orderId, order.clientOrderId,
//                     order.lastMatchedOrderId, order.lastMatchedOrderId2,
//                     order.matchedId, order.sortId,
//                     order.bracketOrderId, order.takeProfitOrderId, order.stopLossOrderId,
//                     order.triggerPrice, isTriggered.c_str(),
//                     timestamp, order.orderCreated,
//                     order.source, order.tag,
//                     triggerType.c_str(), side.c_str(),
//                     type.c_str(), stopCondition.c_str(),
//                     timeCondition.c_str(), action.c_str(),
//                     status.c_str(), matchedType.c_str());

            jsonOrderString = szJsonOrder;
        }

        static void orderToJson(const OPNX::Order& order, nlohmann::json& jsonOrder)
        {
            OPNX::CInOutLog cInOutLog("orderToJson");

            jsonOrder[key_accountId] = order.accountId;
            jsonOrder[key_marketId] = order.marketId;
            jsonOrder[key_price] = order.price;
            jsonOrder[key_quantity] = order.quantity;
            jsonOrder[key_displayQuantity] = order.displayQuantity;
            jsonOrder[key_remainQuantity] = order.remainQuantity;
            jsonOrder[key_remainAmount] = order.remainAmount;
            jsonOrder[key_amount] = order.amount;
//            jsonOrder[key_upperBound] = order.upperBound;
//            jsonOrder[key_lowerBound] = order.lowerBound;
            jsonOrder[key_lastMatchPrice] = order.lastMatchPrice;
            jsonOrder[key_leg2Price] = order.leg2Price;
            jsonOrder[key_lastMatchQuantity] = order.lastMatchQuantity;
            jsonOrder[key_orderId] = order.orderId;
            jsonOrder[key_clientOrderId] = order.clientOrderId;
            jsonOrder[key_lastMatchedOrderId] = order.lastMatchedOrderId;
            jsonOrder[key_lastMatchedOrderId2] = order.lastMatchedOrderId2;
            jsonOrder[key_matchedId] = order.matchedId;
            jsonOrder[key_sortId] = order.sortId;
            jsonOrder[key_bracketOrderId] = order.bracketOrderId;
            jsonOrder[key_takeProfitOrderId] = order.takeProfitOrderId;
            jsonOrder[key_stopLossOrderId] = order.stopLossOrderId;
            jsonOrder[key_triggerPrice] = order.triggerPrice;
            jsonOrder[key_isTriggered] = order.isTriggered;
            jsonOrder[key_timestamp] = 0 != order.timestamp ? order.timestamp : Utils::getMilliTimestamp();
            jsonOrder[key_orderCreated] = order.orderCreated;
            jsonOrder[key_source] = order.source;
            jsonOrder[key_tag] = std::string(order.tag);

            switch (order.triggerType) {
                case TriggerType::MARK_PRICE      : jsonOrder[key_triggerType] = "MARK_PRICE"     ; break;
                case TriggerType::LAST_PRICE     : jsonOrder[key_triggerType] = "LAST_PRICE"    ; break;
                case TriggerType::TRIGGER_NONE    : jsonOrder[key_triggerType] = "NONE"          ; break;
                default: break;
            }

            switch (order.type) {
                case OrderType::LIMIT      : jsonOrder[key_type] = "LIMIT"     ; break;
                case OrderType::MARKET     : jsonOrder[key_type] = "MARKET"    ; break;
                case OrderType::STOP_LIMIT : jsonOrder[key_type] = "STOP_LIMIT"; break;
                case OrderType::STOP_MARKET : jsonOrder[key_type] = "STOP_MARKET"; break;
                case OrderType::TAKE_PROFIT_LIMIT : jsonOrder[key_type] = "TAKE_PROFIT_LIMIT"; break;
                case OrderType::TAKE_PROFIT_MARKET : jsonOrder[key_type] = "TAKE_PROFIT_MARKET"; break;
                default: break;
            }

            switch (order.side) {
                case OrderSide::BUY      : jsonOrder[key_side] = "BUY"     ; break;
                case OrderSide::SELL     : jsonOrder[key_side] = "SELL"    ; break;
                default: break;
            }

            switch (order.stopCondition) {
                case OrderStopCondition::NONE            : jsonOrder[key_stopCondition] = "NONE"     ; break;
                case OrderStopCondition::GREATER_EQUAL   : jsonOrder[key_stopCondition] = "GREATER_EQUAL" ; break;
                case OrderStopCondition::LESS_EQUAL      : jsonOrder[key_stopCondition] = "LESS_EQUAL"; break;
                default: break;
            }

            switch (order.action) {
                case NEW         : jsonOrder[key_action] = "NEW"         ; break;
                case AMEND       : jsonOrder[key_action] = "AMEND"       ; break;
                case CANCEL      : jsonOrder[key_action] = "CANCEL"      ; break;
                case RECOVERY    : jsonOrder[key_action] = "RECOVERY"    ; break;
                case RECOVERY_END: jsonOrder[key_action] = "RECOVERY_END"; break;
                case TRIGGER_BRACKET: jsonOrder[key_action] = "TRIGGER_BRACKET"; break;
                default: break;
            }

            switch (order.timeCondition) {
                case GTC               : jsonOrder[key_timeCondition] = "GTC"; break;
                case IOC               : jsonOrder[key_timeCondition] = "IOC"; break;
                case FOK               : jsonOrder[key_timeCondition] = "FOK"; break;
                case MAKER_ONLY        : jsonOrder[key_timeCondition] = "MAKER_ONLY"; break;
                case MAKER_ONLY_REPRICE: jsonOrder[key_timeCondition] = "MAKER_ONLY_REPRICE"; break;
                case AUCTION           : jsonOrder[key_timeCondition] = "AUCTION"; break;
                default: break;
            }

            switch (order.selfTradeProtectionType) {
                case STP_NONE       : jsonOrder[key_selfTradeProtection] = "NONE"       ; break;
                case STP_TAKER      : jsonOrder[key_selfTradeProtection] = "EXPIRE_TAKER"      ; break;
                case STP_MAKER      : jsonOrder[key_selfTradeProtection] = "EXPIRE_MAKER"    ; break;
                case STP_BOTH       : jsonOrder[key_selfTradeProtection] = "EXPIRE_BOTH"; break;
                default: break;
            }

            switch (order.status) {
                case OPEN                                           : jsonOrder[key_status] = "OPEN"; break;
                case PARTIAL_FILL                                   : jsonOrder[key_status] = "PARTIAL_FILL"; break;
                case FILLED                                         : jsonOrder[key_status] = "FILLED"; break;
                case CANCELED_BY_USER                               : jsonOrder[key_status] = "CANCELED_BY_USER"; break;
                case CANCELED_BY_MARKET_ORDER_NOT_FULL_MATCHED      : jsonOrder[key_status] = "CANCELED_BY_MARKET_ORDER_NOT_FULL_MATCHED"; break;
                case CANCELED_BY_MARKET_ORDER_NOTHING_MATCH         : jsonOrder[key_status] = "CANCELED_BY_MARKET_ORDER_NOTHING_MATCH"; break;
                case CANCELED_ALL_BY_IOC                            : jsonOrder[key_status] = "CANCELED_ALL_BY_IOC"; break;
                case CANCELED_PARTIAL_BY_IOC                        : jsonOrder[key_status] = "CANCELED_PARTIAL_BY_IOC"; break;
                case CANCELED_BY_FOK                                : jsonOrder[key_status] = "CANCELED_BY_FOK"; break;
                case CANCELED_BY_MAKER_ONLY                         : jsonOrder[key_status] = "CANCELED_BY_MAKER_ONLY"; break;
                case CANCELED_ALL_BY_AUCTION                        : jsonOrder[key_status] = "CANCELED_ALL_BY_AUCTION"; break;
                case CANCELED_PARTIAL_BY_AUCTION                    : jsonOrder[key_status] = "CANCELED_PARTIAL_BY_AUCTION"; break;
                case CANCELED_BY_AMEND                              : jsonOrder[key_status] = "CANCELED_BY_AMEND"; break;
                case CANCELED_BY_NO_AUCTION                         : jsonOrder[key_status] = "CANCELED_BY_NO_AUCTION"; break;
                case CANCELED_BY_PRE_AUCTION                        : jsonOrder[key_status] = "CANCELED_BY_PRE_AUCTION"; break;
                case CANCELED_BY_BRACKET_ORDER                      : jsonOrder[key_status] = "CANCELED_BY_BRACKET_ORDER"; break;
                case CANCELED_BY_SELF_TRADE_PROTECTION              : jsonOrder[key_status] = "CANCELED_BY_SELF_TRADE_PROTECTION"; break;
                case REJECT_CANCEL_ORDER_ID_NOT_FOUND               : jsonOrder[key_status] = "REJECT_CANCEL_ORDER_ID_NOT_FOUND"; break;
                case REJECT_AMEND_ORDER_ID_NOT_FOUND                : jsonOrder[key_status] = "REJECT_AMEND_ORDER_ID_NOT_FOUND"; break;
                case REJECT_DISPLAY_QUANTITY_ZERO                   : jsonOrder[key_status] = "REJECT_DISPLAY_QUANTITY_ZERO"; break;
                case REJECT_DISPLAY_QUANTITY_LARGER_THAN_QUANTITY   : jsonOrder[key_status] = "REJECT_DISPLAY_QUANTITY_LARGER_THAN_QUANTITY"; break;
                case REJECT_BUY_STOP_TRIGGER_LARGE_THAN_STOP_LIMIT  : jsonOrder[key_status] = "REJECT_BUY_STOP_TRIGGER_LARGE_THAN_STOP_LIMIT"; break;
                case REJECT_SELL_STOP_TRIGGER_LESS_THAN_STOP_LIMIT  : jsonOrder[key_status] = "REJECT_SELL_STOP_TRIGGER_LESS_THAN_STOP_LIMIT"; break;
                case REJECT_UNKNOWN_ORDER_ACTION                    : jsonOrder[key_status] = "REJECT_UNKNOWN_ORDER_ACTION"; break;
                case REJECT_QUANTITY_AND_AMOUNT_ZERO                : jsonOrder[key_status] = "REJECT_QUANTITY_AND_AMOUNT_ZERO"; break;
                case REJECT_LIMIT_ORDER_WITH_MARKET_PRICE           : jsonOrder[key_status] = "REJECT_LIMIT_ORDER_WITH_MARKET_PRICE"; break;
                case REJECT_AUCTION_SUPPORT_BUY_SELL_ONLY           : jsonOrder[key_status] = "REJECT_AUCTION_SUPPORT_BUY_SELL_ONLY"; break;
                case REJECT_ORDER_AMENDING_OR_CANCELING             : jsonOrder[key_status] = "REJECT_ORDER_AMENDING_OR_CANCELING"; break;
                case REJECT_MATCHING_ENGINE_RECOVERING              : jsonOrder[key_status] = "REJECT_MATCHING_ENGINE_RECOVERING"; break;
                case REJECT_STOP_CONDITION_IS_NONE                  : jsonOrder[key_status] = "REJECT_STOP_CONDITION_IS_NONE"; break;
                case REJECT_STOP_TRIGGER_PRICE_IS_NONE              : jsonOrder[key_status] = "REJECT_STOP_TRIGGER_PRICE_IS_NONE"; break;
                case REJECT_QUANTITY_AND_AMOUNT_LARGER_ZERO         : jsonOrder[key_status] = "REJECT_QUANTITY_AND_AMOUNT_LARGER_ZERO"; break;
                default: break;
            }

            switch (order.matchedType) {
                case MAKER : jsonOrder[key_matchedType] = "MAKER"; break;
                case TAKER : jsonOrder[key_matchedType] = "TAKER"; break;
                default: break;
            }
        }

        static void jsonToOrder(const nlohmann::json& jsonOrder, OPNX::Order& order)
        {
            OPNX::CInOutLog cInOutLog("jsonToOrder");

            auto it = jsonOrder.find(key_accountId);
            if (jsonOrder.end() != it)
            {
                order.accountId = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_marketId);
            if (jsonOrder.end() != it)
            {
                order.marketId = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_price);
            if (jsonOrder.end() != it)
            {
                order.price = it->get<long long>();
            }
            it = jsonOrder.find(key_quantity);
            if (jsonOrder.end() != it)
            {
                order.quantity = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_displayQuantity);
            if (jsonOrder.end() != it)
            {
                order.displayQuantity = it->get<unsigned long long>();
                if (0 == order.displayQuantity)
                {
                    order.displayQuantity = order.quantity;
                }
            } else {
                order.displayQuantity = order.quantity;
            }
            it = jsonOrder.find(key_remainQuantity);
            if (jsonOrder.end() != it)
            {
                order.remainQuantity = it->get<unsigned long long>();
            } else {
                order.remainQuantity = order.quantity;
            }
            it = jsonOrder.find(key_amount);
            if (jsonOrder.end() != it)
            {
                order.amount = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_remainAmount);
            if (jsonOrder.end() != it)
            {
                order.remainAmount = it->get<unsigned long long>();
            }
            else
            {
                order.remainAmount = order.amount;
            }
            it = jsonOrder.find(key_upperBound);
            if (jsonOrder.end() != it)
            {
                order.upperBound = it->get<long long>();
            }
            it = jsonOrder.find(key_lowerBound);
            if (jsonOrder.end() != it)
            {
                order.lowerBound = it->get<long long>();
            }
            it = jsonOrder.find(key_lastMatchPrice);
            if (jsonOrder.end() != it)
            {
                order.lastMatchPrice = it->get<long long>();
            }
            it = jsonOrder.find(key_leg2Price);
            if (jsonOrder.end() != it)
            {
                order.leg2Price = it->get<long long>();
            }
            it = jsonOrder.find(key_lastMatchQuantity);
            if (jsonOrder.end() != it)
            {
                order.lastMatchQuantity = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_orderId);
            if (jsonOrder.end() != it)
            {
                order.orderId = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_clientOrderId);
            if (jsonOrder.end() != it)
            {
                order.clientOrderId = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_lastMatchedOrderId);
            if (jsonOrder.end() != it)
            {
                order.lastMatchedOrderId = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_lastMatchedOrderId2);
            if (jsonOrder.end() != it)
            {
                order.lastMatchedOrderId2 = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_matchedId);
            if (jsonOrder.end() != it)
            {
                order.matchedId = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_sortId);
            if (jsonOrder.end() != it)
            {
                order.sortId = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_bracketOrderId);
            if (jsonOrder.end() != it)
            {
                order.bracketOrderId = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_takeProfitOrderId);
            if (jsonOrder.end() != it)
            {
                order.takeProfitOrderId = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_stopLossOrderId);
            if (jsonOrder.end() != it)
            {
                order.stopLossOrderId = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_triggerPrice);
            if (jsonOrder.end() != it)
            {
                order.triggerPrice = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_isTriggered);
            if (jsonOrder.end() != it)
            {
                order.isTriggered = it->get<bool>();
            }
            it = jsonOrder.find(key_timestamp);
            if (jsonOrder.end() != it)
            {
                order.timestamp = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_orderCreated);
            if (jsonOrder.end() != it)
            {
                order.orderCreated = it->get<unsigned long long>();
            }
            it = jsonOrder.find(key_source);
            if (jsonOrder.end() != it)
            {
                order.source = it->get<int>();
            }
            it = jsonOrder.find(key_tag);
            if (jsonOrder.end() != it)
            {
                std::string tag = it->get<std::string>();
                strncpy(&order.tag[0], tag.c_str(), sizeof(order.tag) - 1);
            }
            it = jsonOrder.find(key_triggerType);
            if (jsonOrder.end() != it)
            {
                std::string triggerType = it->get<std::string>();
                if ("MARK_PRICE" == triggerType)
                {
                    order.triggerType = TriggerType::MARK_PRICE;
                }
                else if ("LAST_PRICE" == triggerType)
                {
                    order.triggerType = TriggerType::LAST_PRICE;
                }
                else
                {
                    order.triggerType = TriggerType::TRIGGER_NONE;
                }
            }
            it = jsonOrder.find(key_side);
            if (jsonOrder.end() != it)
            {
                std::string side = it->get<std::string>();
                if ("BUY" == side)
                {
                    order.side = OrderSide::BUY;
                }
                else
                {
                    order.side = OrderSide::SELL;
                }
            }

            it = jsonOrder.find(key_type);
            if (jsonOrder.end() != it)
            {
                std::string type = it->get<std::string>();
                if ("LIMIT" == type)
                {
                    order.type = OrderType::LIMIT;
                }
                else if ("MARKET" == type)
                {
                    order.type = OrderType::MARKET;
                }
                else if ("STOP_LIMIT" == type)
                {
                    order.type = OrderType::STOP_LIMIT;
                }
                else if ("STOP_MARKET" == type)
                {
                    order.type = OrderType::STOP_MARKET;
                }
                else if ("TAKE_PROFIT_LIMIT" == type)
                {
                    order.type = OrderType::TAKE_PROFIT_LIMIT;
                }
                else if ("TAKE_PROFIT_MARKET" == type)
                {
                    order.type = OrderType::TAKE_PROFIT_MARKET;
                }
            }

            it = jsonOrder.find(key_stopCondition);
            if (jsonOrder.end() != it)
            {
                std::string stopCondition = it->get<std::string>();
                if ("GREATER_EQUAL" == stopCondition)
                {
                    order.stopCondition = OrderStopCondition::GREATER_EQUAL;
                }
                else if ("LESS_EQUAL" == stopCondition)
                {
                    order.stopCondition = OrderStopCondition::LESS_EQUAL;
                }
                else
                {
                    order.stopCondition = OrderStopCondition::NONE;
                }
            } else {
                if (OrderType::STOP_LIMIT == order.type || OrderType::STOP_MARKET == order.type)
                {
                    if (OrderSide::BUY == order.side)
                    {
                        order.stopCondition = OrderStopCondition::GREATER_EQUAL;
                    } else {
                        order.stopCondition = OrderStopCondition::LESS_EQUAL;
                    }
                }
                else if (OrderType::TAKE_PROFIT_LIMIT == order.type || OrderType::TAKE_PROFIT_MARKET == order.type)
                {
                    if (OrderSide::BUY == order.side)
                    {
                        order.stopCondition = OrderStopCondition::LESS_EQUAL;
                    } else {
                        order.stopCondition = OrderStopCondition::GREATER_EQUAL;
                    }
                } else {
                    order.stopCondition = OrderStopCondition::NONE;
                }
            }

            it = jsonOrder.find(key_action);
            if (jsonOrder.end() != it)
            {
                std::string action = it->get<std::string>();
                if ("NEW" == action)
                {
                    order.action = OrderActionType::NEW;
                }
                else if ("AMEND" == action)
                {
                    order.action = OrderActionType::AMEND;
                }
                else if ("CANCEL" == action)
                {
                    order.action = OrderActionType::CANCEL;
                }
                else if ("RECOVERY" == action)
                {
                    order.action = OrderActionType::RECOVERY;
                }
                else if ("RECOVERY_END" == action)
                {
                    order.action = OrderActionType::RECOVERY_END;
                }
                else if ( "TRIGGER_BRACKET" == action)
                {
                    order.action = OrderActionType::TRIGGER_BRACKET;
                }
            }
            it = jsonOrder.find(key_timeCondition);
            if (jsonOrder.end() != it)
            {
                std::string timeCondition = it->get<std::string>();
                if ("GTC" == timeCondition)
                {
                    order.timeCondition = OrderTimeCondition::GTC;
                }
                else if ("IOC" == timeCondition)
                {
                    order.timeCondition = OrderTimeCondition::IOC;
                }
                else if ("FOK" == timeCondition)
                {
                    order.timeCondition = OrderTimeCondition::FOK;
                }
                else if ("MAKER_ONLY" == timeCondition)
                {
                    order.timeCondition = OrderTimeCondition::MAKER_ONLY;
                }
                else if ("MAKER_ONLY_REPRICE" == timeCondition)
                {
                    order.timeCondition = OrderTimeCondition::MAKER_ONLY_REPRICE;
                }
                else if ("AUCTION" == timeCondition)
                {
                    order.timeCondition = OrderTimeCondition::AUCTION;
                }
            }

            it = jsonOrder.find(key_selfTradeProtection);
            if (jsonOrder.end() != it)
            {
                std::string selfTradeProtection = it->get<std::string>();
                if ("NONE" == selfTradeProtection)
                {
                    order.selfTradeProtectionType = SelfTradeProtectionType::STP_NONE;
                }
                else if ("EXPIRE_TAKER" == selfTradeProtection)
                {
                    order.selfTradeProtectionType = SelfTradeProtectionType::STP_TAKER;
                }
                else if ("EXPIRE_MAKER" == selfTradeProtection)
                {
                    order.selfTradeProtectionType = SelfTradeProtectionType::STP_MAKER;
                }
                else if ("EXPIRE_BOTH" == selfTradeProtection)
                {
                    order.selfTradeProtectionType = SelfTradeProtectionType::STP_BOTH;
                }
            }

            it = jsonOrder.find(key_matchedType);
            if (jsonOrder.end() != it)
            {
                std::string matchedType = it->get<std::string>();
                if ("MAKER" == matchedType)
                {
                    order.matchedType = OrderMatchedType::MAKER;
                }
                else
                {
                    order.matchedType = OrderMatchedType::TAKER;
                }
            }
            it = jsonOrder.find(key_status);
            if (jsonOrder.end() != it)
            {
                std::string status = it->get<std::string>();
                if ("OPEN" == status)
                {
                    order.status = OrderStatusType::OPEN;
                }
                else if ("PARTIAL_FILL" == status)
                {
                    order.status = OrderStatusType::PARTIAL_FILL;
                }
                else if ("FILLED" == status)
                {
                    order.status = OrderStatusType::FILLED;
                }
                else if ("CANCELED_BY_USER" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_USER;
                }
                else if ("CANCELED_BY_MARKET_ORDER_NOT_FULL_MATCHED" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_MARKET_ORDER_NOT_FULL_MATCHED;
                }
                else if ("CANCELED_BY_MARKET_ORDER_NOTHING_MATCH" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_MARKET_ORDER_NOTHING_MATCH;
                }
                else if ("CANCELED_ALL_BY_IOC" == status)
                {
                    order.status = OrderStatusType::CANCELED_ALL_BY_IOC;
                }
                else if ("CANCELED_PARTIAL_BY_IOC" == status)
                {
                    order.status = OrderStatusType::CANCELED_PARTIAL_BY_IOC;
                }
                else if ("CANCELED_BY_FOK" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_FOK;
                }
                else if ("CANCELED_BY_MAKER_ONLY" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_MAKER_ONLY;
                }
                else if ("CANCELED_ALL_BY_AUCTION" == status)
                {
                    order.status = OrderStatusType::CANCELED_ALL_BY_AUCTION;
                }
                else if ("CANCELED_PARTIAL_BY_AUCTION" == status)
                {
                    order.status = OrderStatusType::CANCELED_PARTIAL_BY_AUCTION;
                }
                else if ("CANCELED_BY_AMEND" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_AMEND;
                }
                else if ("CANCELED_BY_NO_AUCTION" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_NO_AUCTION;
                }
                else if ("CANCELED_BY_PRE_AUCTION" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_PRE_AUCTION;
                }
                else if ("CANCELED_BY_BRACKET_ORDER" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_BRACKET_ORDER;
                }
                else if ("CANCELED_BY_SELF_TRADE_PROTECTION" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_SELF_TRADE_PROTECTION;
                }
                else if ("REJECT_CANCEL_ORDER_ID_NOT_FOUND" == status)
                {
                    order.status = OrderStatusType::REJECT_CANCEL_ORDER_ID_NOT_FOUND;
                }
                else if ("REJECT_AMEND_ORDER_ID_NOT_FOUND" == status)
                {
                    order.status = OrderStatusType::REJECT_AMEND_ORDER_ID_NOT_FOUND;
                }
                else if ("REJECT_DISPLAY_QUANTITY_ZERO" == status)
                {
                    order.status = OrderStatusType::REJECT_DISPLAY_QUANTITY_ZERO;
                }
                else if ("REJECT_DISPLAY_QUANTITY_LARGER_THAN_QUANTITY" == status)
                {
                    order.status = OrderStatusType::REJECT_DISPLAY_QUANTITY_LARGER_THAN_QUANTITY;
                }
                else if ("REJECT_BUY_STOP_TRIGGER_LARGE_THAN_STOP_LIMIT" == status)
                {
                    order.status = OrderStatusType::REJECT_BUY_STOP_TRIGGER_LARGE_THAN_STOP_LIMIT;
                }
                else if ("REJECT_SELL_STOP_TRIGGER_LESS_THAN_STOP_LIMIT" == status)
                {
                    order.status = OrderStatusType::REJECT_SELL_STOP_TRIGGER_LESS_THAN_STOP_LIMIT;
                }
                else if ("REJECT_UNKNOWN_ORDER_ACTION" == status)
                {
                    order.status = OrderStatusType::REJECT_UNKNOWN_ORDER_ACTION;
                }
                else if ("REJECT_QUANTITY_AND_AMOUNT_ZERO" == status)
                {
                    order.status = OrderStatusType::REJECT_QUANTITY_AND_AMOUNT_ZERO;
                }
                else if ("REJECT_LIMIT_ORDER_WITH_MARKET_PRICE" == status)
                {
                    order.status = OrderStatusType::REJECT_LIMIT_ORDER_WITH_MARKET_PRICE;
                }
                else if ("REJECT_AUCTION_SUPPORT_BUY_SELL_ONLY" == status)
                {
                    order.status = OrderStatusType::REJECT_AUCTION_SUPPORT_BUY_SELL_ONLY;
                }
                else if ("REJECT_ORDER_AMENDING_OR_CANCELING" == status)
                {
                    order.status = OrderStatusType::REJECT_ORDER_AMENDING_OR_CANCELING;
                }
                else if ("REJECT_MATCHING_ENGINE_RECOVERING" == status)
                {
                    order.status = OrderStatusType::REJECT_MATCHING_ENGINE_RECOVERING;
                }
                else if ("REJECT_STOP_CONDITION_IS_NONE" == status)
                {
                    order.status = OrderStatusType::REJECT_STOP_CONDITION_IS_NONE;
                }
                else if ("REJECT_STOP_TRIGGER_PRICE_IS_NONE" == status)
                {
                    order.status = OrderStatusType::REJECT_STOP_TRIGGER_PRICE_IS_NONE;
                }
                else if ("REJECT_QUANTITY_AND_AMOUNT_LARGER_ZERO" == status)
                {
                    order.status = OrderStatusType::REJECT_QUANTITY_AND_AMOUNT_LARGER_ZERO;
                }
            }

        }

        static void rapidjsonToOrder(const rapidjson::Document& jsonOrder, OPNX::Order& order)
        {
//            OPNX::CInOutLog cInOutLog("rapidjsonToOrder");

            auto it = jsonOrder.FindMember(key_accountId);
            if (jsonOrder.MemberEnd() != it)
            {
                order.accountId = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_marketId);
            if (jsonOrder.MemberEnd() != it)
            {
                order.marketId = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_price);
            if (jsonOrder.MemberEnd() != it)
            {
                order.price = it->value.GetInt64();
            }
            it = jsonOrder.FindMember(key_quantity);
            if (jsonOrder.MemberEnd() != it)
            {
                order.quantity = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_displayQuantity);
            if (jsonOrder.MemberEnd() != it)
            {
                order.displayQuantity = it->value.GetUint64();
                if (0 == order.displayQuantity)
                {
                    order.displayQuantity = order.quantity;
                }
            } else {
                order.displayQuantity = order.quantity;
            }
            it = jsonOrder.FindMember(key_remainQuantity);
            if (jsonOrder.MemberEnd() != it)
            {
                order.remainQuantity = it->value.GetUint64();
            } else {
                order.remainQuantity = order.quantity;
            }
            it = jsonOrder.FindMember(key_amount);
            if (jsonOrder.MemberEnd() != it)
            {
                order.amount = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_remainAmount);
            if (jsonOrder.MemberEnd() != it)
            {
                order.remainAmount = it->value.GetUint64();
            }
            else
            {
                order.remainAmount = order.amount;
            }
            it = jsonOrder.FindMember(key_upperBound);
            if (jsonOrder.MemberEnd() != it)
            {
                order.upperBound = it->value.GetInt64();
            }
            it = jsonOrder.FindMember(key_lowerBound);
            if (jsonOrder.MemberEnd() != it)
            {
                order.lowerBound = it->value.GetInt64();
            }
            it = jsonOrder.FindMember(key_lastMatchPrice);
            if (jsonOrder.MemberEnd() != it)
            {
                order.lastMatchPrice = it->value.GetInt64();
            }
            it = jsonOrder.FindMember(key_leg2Price);
            if (jsonOrder.MemberEnd() != it)
            {
                order.leg2Price = it->value.GetInt64();
            }
            it = jsonOrder.FindMember(key_lastMatchQuantity);
            if (jsonOrder.MemberEnd() != it)
            {
                order.lastMatchQuantity = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_orderId);
            if (jsonOrder.MemberEnd() != it)
            {
                order.orderId = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_clientOrderId);
            if (jsonOrder.MemberEnd() != it)
            {
                order.clientOrderId = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_lastMatchedOrderId);
            if (jsonOrder.MemberEnd() != it)
            {
                order.lastMatchedOrderId = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_lastMatchedOrderId2);
            if (jsonOrder.MemberEnd() != it)
            {
                order.lastMatchedOrderId2 = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_matchedId);
            if (jsonOrder.MemberEnd() != it)
            {
                order.matchedId = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_sortId);
            if (jsonOrder.MemberEnd() != it)
            {
                order.sortId = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_bracketOrderId);
            if (jsonOrder.MemberEnd() != it)
            {
                order.bracketOrderId = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_takeProfitOrderId);
            if (jsonOrder.MemberEnd() != it)
            {
                order.takeProfitOrderId = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_stopLossOrderId);
            if (jsonOrder.MemberEnd() != it)
            {
                order.stopLossOrderId = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_triggerPrice);
            if (jsonOrder.MemberEnd() != it)
            {
                order.triggerPrice = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_isTriggered);
            if (jsonOrder.MemberEnd() != it)
            {
                order.isTriggered = it->value.GetBool();
            }
            it = jsonOrder.FindMember(key_timestamp);
            if (jsonOrder.MemberEnd() != it)
            {
                order.timestamp = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_orderCreated);
            if (jsonOrder.MemberEnd() != it)
            {
                order.orderCreated = it->value.GetUint64();
            }
            it = jsonOrder.FindMember(key_source);
            if (jsonOrder.MemberEnd() != it)
            {
                order.source = it->value.GetInt();
            }
            it = jsonOrder.FindMember(key_tag);
            if (jsonOrder.MemberEnd() != it)
            {
                std::string tag = it->value.GetString();
                strncpy(&order.tag[0], tag.c_str(), sizeof(order.tag) - 1);
            }
            it = jsonOrder.FindMember(key_triggerType);
            if (jsonOrder.MemberEnd() != it)
            {
                std::string triggerType = it->value.GetString();
                if ("MARK_PRICE" == triggerType)
                {
                    order.triggerType = TriggerType::MARK_PRICE;
                }
                else if ("LAST_PRICE" == triggerType)
                {
                    order.triggerType = TriggerType::LAST_PRICE;
                }
                else
                {
                    order.triggerType = TriggerType::TRIGGER_NONE;
                }
            }
            it = jsonOrder.FindMember(key_side);
            if (jsonOrder.MemberEnd() != it)
            {
                std::string side = it->value.GetString();
                if ("BUY" == side)
                {
                    order.side = OrderSide::BUY;
                }
                else
                {
                    order.side = OrderSide::SELL;
                }
            }

            it = jsonOrder.FindMember(key_type);
            if (jsonOrder.MemberEnd() != it)
            {
                std::string type = it->value.GetString();
                if ("LIMIT" == type)
                {
                    order.type = OrderType::LIMIT;
                }
                else if ("MARKET" == type)
                {
                    order.type = OrderType::MARKET;
                }
                else if ("STOP_LIMIT" == type)
                {
                    order.type = OrderType::STOP_LIMIT;
                }
                else if ("STOP_MARKET" == type)
                {
                    order.type = OrderType::STOP_MARKET;
                }
                else if ("TAKE_PROFIT_LIMIT" == type)
                {
                    order.type = OrderType::TAKE_PROFIT_LIMIT;
                }
                else if ("TAKE_PROFIT_MARKET" == type)
                {
                    order.type = OrderType::TAKE_PROFIT_MARKET;
                }
            }

            it = jsonOrder.FindMember(key_stopCondition);
            if (jsonOrder.MemberEnd() != it)
            {
                std::string stopCondition = it->value.GetString();
                if ("GREATER_EQUAL" == stopCondition)
                {
                    order.stopCondition = OrderStopCondition::GREATER_EQUAL;
                }
                else if ("LESS_EQUAL" == stopCondition)
                {
                    order.stopCondition = OrderStopCondition::LESS_EQUAL;
                }
                else
                {
                    order.stopCondition = OrderStopCondition::NONE;
                }
            } else {
                if (OrderType::STOP_LIMIT == order.type || OrderType::STOP_MARKET == order.type)
                {
                    if (OrderSide::BUY == order.side)
                    {
                        order.stopCondition = OrderStopCondition::GREATER_EQUAL;
                    } else {
                        order.stopCondition = OrderStopCondition::LESS_EQUAL;
                    }
                }
                else if (OrderType::TAKE_PROFIT_LIMIT == order.type || OrderType::TAKE_PROFIT_MARKET == order.type)
                {
                    if (OrderSide::BUY == order.side)
                    {
                        order.stopCondition = OrderStopCondition::LESS_EQUAL;
                    } else {
                        order.stopCondition = OrderStopCondition::GREATER_EQUAL;
                    }
                } else {
                    order.stopCondition = OrderStopCondition::NONE;
                }
            }

            it = jsonOrder.FindMember(key_action);
            if (jsonOrder.MemberEnd() != it)
            {
                std::string action = it->value.GetString();
                if ("NEW" == action)
                {
                    order.action = OrderActionType::NEW;
                }
                else if ("AMEND" == action)
                {
                    order.action = OrderActionType::AMEND;
                }
                else if ("CANCEL" == action)
                {
                    order.action = OrderActionType::CANCEL;
                }
                else if ("RECOVERY" == action)
                {
                    order.action = OrderActionType::RECOVERY;
                }
                else if ("RECOVERY_END" == action)
                {
                    order.action = OrderActionType::RECOVERY_END;
                }
                else if ( "TRIGGER_BRACKET" == action)
                {
                    order.action = OrderActionType::TRIGGER_BRACKET;
                }
            }
            it = jsonOrder.FindMember(key_timeCondition);
            if (jsonOrder.MemberEnd() != it)
            {
                std::string timeCondition = it->value.GetString();
                if ("GTC" == timeCondition)
                {
                    order.timeCondition = OrderTimeCondition::GTC;
                }
                else if ("IOC" == timeCondition)
                {
                    order.timeCondition = OrderTimeCondition::IOC;
                }
                else if ("FOK" == timeCondition)
                {
                    order.timeCondition = OrderTimeCondition::FOK;
                }
                else if ("MAKER_ONLY" == timeCondition)
                {
                    order.timeCondition = OrderTimeCondition::MAKER_ONLY;
                }
                else if ("MAKER_ONLY_REPRICE" == timeCondition)
                {
                    order.timeCondition = OrderTimeCondition::MAKER_ONLY_REPRICE;
                }
                else if ("AUCTION" == timeCondition)
                {
                    order.timeCondition = OrderTimeCondition::AUCTION;
                }
            }

            it = jsonOrder.FindMember(key_selfTradeProtection);
            if (jsonOrder.MemberEnd() != it)
            {
                std::string selfTradeProtection = it->value.GetString();
                if ("NONE" == selfTradeProtection)
                {
                    order.selfTradeProtectionType = SelfTradeProtectionType::STP_NONE;
                }
                else if ("EXPIRE_TAKER" == selfTradeProtection)
                {
                    order.selfTradeProtectionType = SelfTradeProtectionType::STP_TAKER;
                }
                else if ("EXPIRE_MAKER" == selfTradeProtection)
                {
                    order.selfTradeProtectionType = SelfTradeProtectionType::STP_MAKER;
                }
                else if ("EXPIRE_BOTH" == selfTradeProtection)
                {
                    order.selfTradeProtectionType = SelfTradeProtectionType::STP_BOTH;
                }
            }

            it = jsonOrder.FindMember(key_matchedType);
            if (jsonOrder.MemberEnd() != it)
            {
                std::string matchedType = it->value.GetString();
                if ("MAKER" == matchedType)
                {
                    order.matchedType = OrderMatchedType::MAKER;
                }
                else
                {
                    order.matchedType = OrderMatchedType::TAKER;
                }
            }
            it = jsonOrder.FindMember(key_status);
            if (jsonOrder.MemberEnd() != it)
            {
                std::string status = it->value.GetString();
                if ("OPEN" == status)
                {
                    order.status = OrderStatusType::OPEN;
                }
                else if ("PARTIAL_FILL" == status)
                {
                    order.status = OrderStatusType::PARTIAL_FILL;
                }
                else if ("FILLED" == status)
                {
                    order.status = OrderStatusType::FILLED;
                }
                else if ("CANCELED_BY_USER" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_USER;
                }
                else if ("CANCELED_BY_MARKET_ORDER_NOT_FULL_MATCHED" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_MARKET_ORDER_NOT_FULL_MATCHED;
                }
                else if ("CANCELED_BY_MARKET_ORDER_NOTHING_MATCH" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_MARKET_ORDER_NOTHING_MATCH;
                }
                else if ("CANCELED_ALL_BY_IOC" == status)
                {
                    order.status = OrderStatusType::CANCELED_ALL_BY_IOC;
                }
                else if ("CANCELED_PARTIAL_BY_IOC" == status)
                {
                    order.status = OrderStatusType::CANCELED_PARTIAL_BY_IOC;
                }
                else if ("CANCELED_BY_FOK" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_FOK;
                }
                else if ("CANCELED_BY_MAKER_ONLY" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_MAKER_ONLY;
                }
                else if ("CANCELED_ALL_BY_AUCTION" == status)
                {
                    order.status = OrderStatusType::CANCELED_ALL_BY_AUCTION;
                }
                else if ("CANCELED_PARTIAL_BY_AUCTION" == status)
                {
                    order.status = OrderStatusType::CANCELED_PARTIAL_BY_AUCTION;
                }
                else if ("CANCELED_BY_AMEND" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_AMEND;
                }
                else if ("CANCELED_BY_NO_AUCTION" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_NO_AUCTION;
                }
                else if ("CANCELED_BY_PRE_AUCTION" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_PRE_AUCTION;
                }
                else if ("CANCELED_BY_BRACKET_ORDER" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_BRACKET_ORDER;
                }
                else if ("CANCELED_BY_SELF_TRADE_PROTECTION" == status)
                {
                    order.status = OrderStatusType::CANCELED_BY_SELF_TRADE_PROTECTION;
                }
                else if ("REJECT_CANCEL_ORDER_ID_NOT_FOUND" == status)
                {
                    order.status = OrderStatusType::REJECT_CANCEL_ORDER_ID_NOT_FOUND;
                }
                else if ("REJECT_AMEND_ORDER_ID_NOT_FOUND" == status)
                {
                    order.status = OrderStatusType::REJECT_AMEND_ORDER_ID_NOT_FOUND;
                }
                else if ("REJECT_DISPLAY_QUANTITY_ZERO" == status)
                {
                    order.status = OrderStatusType::REJECT_DISPLAY_QUANTITY_ZERO;
                }
                else if ("REJECT_DISPLAY_QUANTITY_LARGER_THAN_QUANTITY" == status)
                {
                    order.status = OrderStatusType::REJECT_DISPLAY_QUANTITY_LARGER_THAN_QUANTITY;
                }
                else if ("REJECT_BUY_STOP_TRIGGER_LARGE_THAN_STOP_LIMIT" == status)
                {
                    order.status = OrderStatusType::REJECT_BUY_STOP_TRIGGER_LARGE_THAN_STOP_LIMIT;
                }
                else if ("REJECT_SELL_STOP_TRIGGER_LESS_THAN_STOP_LIMIT" == status)
                {
                    order.status = OrderStatusType::REJECT_SELL_STOP_TRIGGER_LESS_THAN_STOP_LIMIT;
                }
                else if ("REJECT_UNKNOWN_ORDER_ACTION" == status)
                {
                    order.status = OrderStatusType::REJECT_UNKNOWN_ORDER_ACTION;
                }
                else if ("REJECT_QUANTITY_AND_AMOUNT_ZERO" == status)
                {
                    order.status = OrderStatusType::REJECT_QUANTITY_AND_AMOUNT_ZERO;
                }
                else if ("REJECT_LIMIT_ORDER_WITH_MARKET_PRICE" == status)
                {
                    order.status = OrderStatusType::REJECT_LIMIT_ORDER_WITH_MARKET_PRICE;
                }
                else if ("REJECT_AUCTION_SUPPORT_BUY_SELL_ONLY" == status)
                {
                    order.status = OrderStatusType::REJECT_AUCTION_SUPPORT_BUY_SELL_ONLY;
                }
                else if ("REJECT_ORDER_AMENDING_OR_CANCELING" == status)
                {
                    order.status = OrderStatusType::REJECT_ORDER_AMENDING_OR_CANCELING;
                }
                else if ("REJECT_MATCHING_ENGINE_RECOVERING" == status)
                {
                    order.status = OrderStatusType::REJECT_MATCHING_ENGINE_RECOVERING;
                }
                else if ("REJECT_STOP_CONDITION_IS_NONE" == status)
                {
                    order.status = OrderStatusType::REJECT_STOP_CONDITION_IS_NONE;
                }
                else if ("REJECT_STOP_TRIGGER_PRICE_IS_NONE" == status)
                {
                    order.status = OrderStatusType::REJECT_STOP_TRIGGER_PRICE_IS_NONE;
                }
                else if ("REJECT_QUANTITY_AND_AMOUNT_LARGER_ZERO" == status)
                {
                    order.status = OrderStatusType::REJECT_QUANTITY_AND_AMOUNT_LARGER_ZERO;
                }
            }

        }

    };

    struct TriggerPrice {
    public:
        unsigned long long marketId;
        long long price;
        TriggerPrice() :
                marketId(0),
                price(Order::MAX_PRICE){}

        TriggerPrice(const TriggerPrice &) = default;
        TriggerPrice(TriggerPrice &&) = default;
        TriggerPrice &operator=(const TriggerPrice &) = default;
        TriggerPrice &operator=(TriggerPrice &&) = default;
        ~TriggerPrice() = default;
    };

    using SearchOrderMap = std::unordered_map<unsigned long long, OPNX::Order>;  // key is orderId
    using SortOrderMap = std::map<unsigned long long, OPNX::Order*>;   // key is sortId
    using OrderAscendMap = std::map<long long, SortOrderMap, std::less<long long>>;  // key is price, in ascending order
    using OrderDescendMap = std::map<long long, SortOrderMap, std::greater<long long>>;  // key is price, in descending order

}


#endif /* MATCHING_INC_ORDER_HPP_ */
