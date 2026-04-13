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

    void OrderBook::add_to_book(Order&& order) {
        if(order.side() == Side::Buy) {
            auto [lvl_it, _] = bids_.try_emplace(order.price());
            lvl_it->second.emplace_back(std::move(order));
            auto order_it = std::prev(lvl_it->second.end());
            order_index_[order_it->id()] = {Side::Buy, lvl_it->first, order_it};
        }

        else {
            auto [lvl_it, _] = asks_.try_emplace(order.price());
            lvl_it->second.emplace_back(std::move(order));
            auto order_it = std::prev(lvl_it->second.end());
            order_index_[order_it->id()] = {Side::Sell, lvl_it->first, order_it};
        }
    }

    bool OrderBook::cancel_order(uint64_t id) {
        auto found = order_index_.find(id);
        if(found == order_index_.end()) {
            return false;
        }

        const auto locator = found->second;
        if(locator.side == Side::Buy) {
            auto level_it = bids_.find(locator.price);
            if(level_it != bids_.end()) {
                level_it->second.erase(locator.order_it);
                if(level_it->second.empty()) {
                    bids_.erase(level_it);
                }
            }
        }

        else {
            auto level_it = asks_.find(locator.price);
            if(level_it != asks_.end()) {
                level_it->second.erase(locator.order_it);
                if(level_it->second.empty()) {
                    asks_.erase(level_it);
                }
            }
        }

        order_index_.erase(found);
        return true;
    }

    TopOfBook OrderBook::get_order_book(std::size_t depth) const {
        TopOfBook snapshot;
        snapshot.bids.reserve(depth);
        snapshot.asks.reserve(depth);

        std::size_t count = 0;
        for(const auto& [price, orders] : bids_) {
            if(count++ >= depth) break;
            uint64_t sum = 0;
            for(const auto& order : orders) sum += order.remaining_quantity();
            snapshot.bids.push_back(BookLevel{price, sum});
        }

        count = 0;
        for(const auto& [price, orders] : asks_) {
            if(count++ >= depth) break;
            uint64_t sum = 0;
            for(const auto& order : orders) sum += order.remaining_quantity();
            snapshot.asks.push_back(BookLevel{price, sum});
        }

        return snapshot;
    }

    std::vector<TradeEvent> OrderBook::get_recent_trades(std::size_t count) const {
        if(count >= trades_.size()) {
            return trades_;
        }
        return std::vector<TradeEvent>(trades_.end() - static_cast<long>(count), trades_.end());
    }

    void OrderBook::record_trades(const std::vector<TradeEvent>& trades) {
        if(trades.empty()) {
            return;
        }

        trades_.insert(trades_.end(), trades.begin(), trades.end());
        if(trades_.size() > kMaxTrades) {
            const auto remove_count = trades_.size() - kMaxTrades;
            trades_.erase(trades_.begin(), trades_.begin() + static_cast<long>(remove_count));
        }
    }
}