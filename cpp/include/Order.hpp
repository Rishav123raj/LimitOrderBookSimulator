#pragma once

#include <cstdint>

#include "Types.hpp"

namespace lob {

    class Order {
        public:
        Order(uint64_t id, Side side, OrderType type, double price, uint64_t quantity, uint64_t timestamp)
            : id_(id), 
            side_(side), 
            type_(type), 
            price_(price), 
            initial_quantity_(quantity), 
            remaining_quantity_(quantity),
            timestamp_(timestamp) {}

        [[nodiscard]] uint64_t id() const { return id_; }
        [[nodiscard]] Side side() const { return side_; }
        [[nodiscard]] OrderType type() const { return type_; }
        [[nodiscard]] double price() const { return price_; }
        [[nodiscard]] uint64_t initial_quantity() const { return initial_quantity_; }
        [[nodiscard]] uint64_t remaining_quantity() const { return remaining_quantity_; }
        [[nodiscard]] uint64_t timestamp() const { return timestamp_; }

        void fill(uint64_t quantity) {
            if(quantity > remaining_quantity_) {
                remaining_quantity_ = 0;
                return;
            }
            remaining_quantity_ -= quantity;
        }

        [[nodiscard]] bool is_filled() const { return remaining_quantity_ == 0; }

        private:
        uint64_t id_;
        Side side_;
        OrderType type_;
        double price_;
        uint64_t initial_quantity_;
        uint64_t remaining_quantity_;
        uint64_t timestamp_;
    };
}