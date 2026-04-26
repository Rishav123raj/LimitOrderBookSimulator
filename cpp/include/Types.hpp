#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lob {

    enum class Side : uint8_t {
        Buy = 0,
        Sell = 1
    };

    enum class OrderType : uint8_t {
        Limit = 0,
        Market = 1
    };

    struct TradeEvent {
        double price{};
        uint64_t quantity{};
        uint64_t timestamp{};
        uint64_t buy_order_id{};
        uint64_t sell_order_id{};
    };

    struct BookLevel {
        double price{};
        uint64_t quantity{};    
    };

    struct TopOfBook {
        std::vector<BookLevel> bids;
        std::vector<BookLevel> asks;
    };

    inline std::string side_to_string(Side side) {
        return side == Side::Buy ? "BUY" : "SELL";
    }

    inline std::string type_to_string(OrderType type) {
        return type == OrderType::Limit ? "LIMIT" : "MARKET";
    }
}