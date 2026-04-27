#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include<list>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Order.hpp"
#include "Types.hpp"

namespace lob {
    class OrderBook {
        public:
        struct PlaceResult {
            std::vector<TradeEvent> trades;
            bool accepted{false};
            uint64_t queue_position{0}; 
        };

        struct OrderInfo {
            uint64_t id;
            uint64_t quantity;
        };

        struct BookLevelDetailed {
            double price;
            std::vector<OrderInfo> orders;  // FIFO queue
        };

        struct TopOfBookDetailed {
            std::vector<BookLevelDetailed> bids;
            std::vector<BookLevelDetailed> asks;
        };

        PlaceResult place_order(uint64_t id, Side side, OrderType type, 
                        double price, uint64_t quantity, uint64_t timestamp);

        bool cancel_order(uint64_t id);

        [[nodiscard]] TopOfBook get_order_book(std::size_t depth) const;
        [[nodiscard]] std::vector<TradeEvent> get_recent_trades(std::size_t count) const;
        [[nodiscard]] TopOfBookDetailed get_order_book_detailed(std::size_t depth) const;

        private:
        using LevelList = std::list<Order>;
        using BidBook = std::map<double, LevelList, std::greater<>>;
        using AskBook = std::map<double, LevelList, std::less<>>;

        struct OrderLocator {
            Side side;
            double price; 
            LevelList::iterator order_it;
        };

        PlaceResult match_buy_order(Order incoming);
        PlaceResult match_sell_order(Order incoming);

        void add_to_book(Order&& order);
        void record_trades(const std::vector<TradeEvent>& trades);

        BidBook bids_;
        AskBook asks_;
        std::unordered_map<uint64_t, OrderLocator> order_index_;
        std::vector<TradeEvent> trades_;
        static constexpr std::size_t kMaxTrades = 5000;
    };
}