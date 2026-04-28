#include "MatchingEngine.hpp"

namespace lob {
    
    MatchingEngine::PlaceResult MatchingEngine::place_order(
        uint64_t id, 
        Side side, 
        OrderType type, 
        double price,               
        uint64_t quantity, 
        uint64_t timestamp) {
        return order_book_.place_order(id, side, type, price, quantity, timestamp);
    }

    bool MatchingEngine::cancel_order(uint64_t id) {
        return order_book_.cancel_order(id);
    }

    TopOfBook MatchingEngine::get_order_book(std::size_t depth) const {
        return order_book_.get_order_book(depth);
    }

    std::vector<TradeEvent> MatchingEngine::get_recent_trades(std::size_t count) const {
        return order_book_.get_recent_trades(count);
    }

    OrderBook::TopOfBookDetailed MatchingEngine::get_order_book_detailed(std::size_t depth) const {
        return order_book_.get_order_book_detailed(depth);
    }

    double MatchingEngine::get_obi(std::size_t depth) const {
    return order_book_.compute_obi(depth);
}
}