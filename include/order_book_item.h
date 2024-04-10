//
// Created by Bob   on 2021/12/29.
//

#ifndef MATCHING_ENGINE_ORDER_BOOK_ITEM_H
#define MATCHING_ENGINE_ORDER_BOOK_ITEM_H


namespace OPNX {
    class OrderBookItem {
    public:
        long long price;
        unsigned long long quantity;
        unsigned long long displayQuantity;

        OrderBookItem()
        : price(0)
        , quantity(0)
        , displayQuantity(0)
        {
        }
        OrderBookItem(long long price, unsigned long long quantity,unsigned long long displayQuantity)
        : price(price)
        , quantity(quantity)
        , displayQuantity(displayQuantity)
        {}
        OrderBookItem(const OrderBookItem& obi)
                : price(obi.price)
                , quantity(obi.quantity)
                , displayQuantity(obi.displayQuantity)
        {}
        ~OrderBookItem() = default;
        void operator=(const OrderBookItem &obi)
        {
            price = obi.price;
            quantity = obi.quantity;
            displayQuantity = obi.displayQuantity;
        }
        bool operator==(const OrderBookItem &obi) {
            return ((price == obi.price)
                    && (quantity == obi.quantity)
                    && (displayQuantity == obi.displayQuantity));
        }
        bool operator!=(const OrderBookItem &obi) {
            return !(operator==(obi));
        }
    };

    class SortOrderBook {
    public:
        OrderBookItem obItem;
        OPNX::SortOrderMap sortMapOrder;
    };

    using OrderBookAscendMap = std::map<long long, SortOrderBook, std::less<long long>>;    // key is price
    using OrderBookDescendMap = std::map<long long, SortOrderBook, std::greater<long long>>;  // key is price
}

#endif //MATCHING_ENGINE_ORDER_BOOK_ITEM_H
