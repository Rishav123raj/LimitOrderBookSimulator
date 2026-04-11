#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "OrderBook.hpp"

namespace lob {

    class MatchingEngine {
        public:
        using PlaceResult = OrderBook::PlaceResult;

        PlaceResult place_order(
            uint64_t id, 
            Side side, 
            OrderType type, 
            double price,               
            uint64_t quantity, 
            uint64_t timestamp
        );
        
        bool cancel_order(uint64_t id);
        
        TopOfBook get_order_book(std::size_t depth) const;
        
        std::vector<TradeEvent> get_recent_trades(std::size_t count) const;

        private:
        OrderBook order_book_;
    };
}