#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "Logger.hpp"
#include "MatchingEngine.hpp"

namespace {
    std::string to_json_book(const lob::TopOfBook& book) {
        std::ostringstream out;
        out << "{\"event\" : \"book\", \"bids\" : [";
        for(std::size_t i = 0; i < book.bids.size(); i++) {
            if(i) out << ",";
            out << "{\"Price\":" << std::fixed << std::setprecision(2) << book.bids[i].price << ", \"Quantity\":" << book.bids[i].quantity << '}';
        }
        out << "], \"asks\":[";
        for(std::size_t i = 0; i < book.asks.size(); i++) {
            if(i) out << ",";
            out << "{\"Price\":" << std::fixed << std::setprecision(2) << book.asks[i].price << ", \"Quantity\":" << book.asks[i].quantity << '}';
        }
        out << "]}";
        return out.str();
    }

    std::string to_json_book_detailed(const lob::OrderBook::TopOfBookDetailed& book) {
        std::ostringstream out;
        out << "{\"event\":\"book_detailed\",\"bids\":[";

        for (std::size_t i = 0; i < book.bids.size(); i++) {
            if (i) out << ",";
            out << "{\"price\":" << std::fixed << std::setprecision(2) << book.bids[i].price << ",\"orders\":[";

            for (std::size_t j = 0; j < book.bids[i].orders.size(); j++) {
                if (j) out << ",";
                const auto& o = book.bids[i].orders[j];
                out << "{\"id\":" << o.id << ",\"quantity\":" << o.quantity << "}";
            }

            out << "]}";
        }

        out << "],\"asks\":[";

        for (std::size_t i = 0; i < book.asks.size(); i++) {
            if (i) out << ",";
            out << "{\"price\":" << std::fixed << std::setprecision(2) << book.asks[i].price << ",\"orders\":[";

            for (std::size_t j = 0; j < book.asks[i].orders.size(); j++) {
                if (j) out << ",";
                const auto& o = book.asks[i].orders[j];
                out << "{\"id\":" << o.id << ",\"quantity\":" << o.quantity << "}";
            }

            out << "]}";
        }

        out << "]}";
        return out.str();
    }

    void emit_trades(const std::vector<lob::TradeEvent>& trades, const std::string& aggressor_side) {
        for(const auto& trade : trades) {
            std::cout << "{"
                    << "\"event\":\"trade\","
                    << "\"price\":" << std::fixed << std::setprecision(2) << trade.price << ","
                    << "\"quantity\":" << trade.quantity << ","
                    << "\"timestamp\":" << trade.timestamp << ","
                    << "\"side\":\"" << aggressor_side << "\","   
                    << "\"buy_order_id\":" << trade.buy_order_id << ","
                    << "\"sell_order_id\":" << trade.sell_order_id
                    << "}" << std::endl;
        }
    }

    lob::Side parse_side(const std::string& value) {
        return value == "Buy" ? lob::Side::Buy : lob::Side::Sell;
    }

    lob::OrderType parse_type(const std::string& value) {
        return value == "Limit" ? lob::OrderType::Limit : lob::OrderType::Market;
    }
} //namespace

int main() {
    lob::MatchingEngine engine;
    std::string line;
    lob::Logger::info("LOB Matching Engine Started.");

    while(std::getline(std::cin, line)) {
        if(line.empty()) continue;

        std::istringstream in(line);
        std::string cmd;
        in >> cmd;

        if(cmd == "PLACE") {
            uint64_t id, qty, ts;
            double price;
            std::string side, type;
            in >> id >> side >> type >> price >> qty >> ts;
            auto result = engine.place_order(id, parse_side(side), parse_type(type), price, qty, ts);
            std::cout << "{\"event\":\"ack\", \"ok\":" << (result.accepted ? "true" : "false") << ", \"orderId\":" << id << "}" << std::endl;
            emit_trades(result.trades, side);
            std::cout << to_json_book(engine.get_order_book(10)) << std::endl;
            std::cout << to_json_book_detailed(engine.get_order_book_detailed(10)) << std::endl;

            double obi = engine.get_obi(10);
            std::cout << "{" << "\"event\":\"obi\", " << "\"value\":" << obi << "}" << std::endl;
        }
        
        else if(cmd == "CANCEL") {
            uint64_t id;
            in >> id;
            bool success = engine.cancel_order(id);
            std::cout << "{\"event\":\"cancel_ack\", \"ok\":" << (success ? "true" : "false") << ", \"orderId\":" << id << "}" << std::endl;
            std::cout << to_json_book(engine.get_order_book(10)) << std::endl;
            std::cout << to_json_book_detailed(engine.get_order_book_detailed(10)) << std::endl;
            
            double obi = engine.get_obi(10);
            std::cout << "{" << "\"event\":\"obi\"," << "\"value\":" << obi << "}" << std::endl;
        }

        else if(cmd == "BOOK") {
            std::size_t depth;
            in >> depth;
            std::cout << to_json_book(engine.get_order_book(depth)) << std::endl;
            std::cout << to_json_book_detailed(engine.get_order_book_detailed(10)) << std::endl;

            double obi = engine.get_obi(depth);
            std::cout << "{" << "\"event\":\"obi\"," << "\"value\":" << obi << "}" << std::endl;
        }

        else if(cmd == "TRADES") {
            std::size_t count;
            in >> count;
            emit_trades(engine.get_recent_trades(count), "UNKNOWN");
        }
    }

    return 0;
}