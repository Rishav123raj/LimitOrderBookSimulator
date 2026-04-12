#include "OrderBook.hpp"
#include "Logger.hpp"

#include <algorithm>

namespace lob {
    OrderBook::PlaceResult OrderBook::place_order(
        uint64_t id, Side side, OrderType type, double price, 
        uint64_t quantity, uint64_t timestamp) {
            if(quantity == 0 || order_index_.count(id) > 0) {
                return {};
            }

            Order incoming(id, side, type, price, quantity, timestamp);
            auto result = side == Side::Buy ? match_buy_order(incoming) : match_sell_order(incoming);
            record_trades(result.trades);
            result.accepted = true;
            return result;
        }

    OrderBook::PlaceResult OrderBook::match_buy_order(Order incoming) {
        PlaceResult result;
        while(!incoming.is_filled() && !asks_.empty()) {
            auto best_ask_it = asks_.begin();
            const auto best_ask_price = best_ask_it->first;

            if(incoming.type() == OrderType::Limit && incoming.price() < best_ask_price) {
                break;
            }

            auto& level_orders = best_ask_it->second;
            auto resting_it = level_orders.begin();

            const auto fill_qty = std::min(incoming.remaining_quantity(), resting_it->remaining_quantity());
            incoming.fill(fill_qty);
            resting_it->fill(fill_qty);

            result.trades.push_back(
                TradeEvent {
                    best_ask_price, fill_qty, incoming.timestamp(), incoming.id(), resting_it->id()
                }
            );

            Logger::info("TRADE Buy=" + std::to_string(incoming.id()) +
                        " Sell=" + std::to_string(resting_it->id()) +
                        " Price=" + std::to_string(best_ask_price) +
                        " Qty=" + std::to_string(fill_qty));

            if(resting_it->is_filled()) {
                order_index_.erase(resting_it->id());
                level_orders.erase(resting_it);
            }

            if(level_orders.empty()) {
                asks_.erase(best_ask_it);
            }
        }

        if(!incoming.is_filled() && incoming.type() == OrderType::Limit) {
            add_to_book(std::move(incoming));
        }

        return result;
    }
    
    OrderBook::PlaceResult OrderBook::match_sell_order(Order incoming) {
        PlaceResult result;
        while(!incoming.is_filled() && !bids_.empty()) {
            auto best_bid_it = bids_.begin();
            const auto best_bid_price = best_bid_it->first;

            if(incoming.type() == OrderType::Limit && incoming.price() > best_bid_price) {
                break;
            }

            auto& level_orders = best_bid_it->second;
            auto resting_it = level_orders.begin();

            const auto fill_qty = std::min(incoming.remaining_quantity(), resting_it->remaining_quantity());
            incoming.fill(fill_qty);
            resting_it->fill(fill_qty);

            result.trades.push_back(
                TradeEvent {
                    best_bid_price, fill_qty, incoming.timestamp(), resting_it->id(), incoming.id()
                }
            );

            Logger::info("TRADE Sell=" + std::to_string(resting_it->id()) +
                        " Buy=" + std::to_string(incoming.id()) +
                        " Price=" + std::to_string(best_bid_price) +
                        " Qty=" + std::to_string(fill_qty));

            if(resting_it->is_filled()) {
                order_index_.erase(resting_it->id());
                level_orders.erase(resting_it);
            }

            if(level_orders.empty()) {
                bids_.erase(best_bid_it);
            }
        }

        if(!incoming.is_filled() && incoming.type() == OrderType::Limit) {
            add_to_book(std::move(incoming));
        }

        return result;
    }

    
}