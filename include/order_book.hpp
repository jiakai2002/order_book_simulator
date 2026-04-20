#pragma once
#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace ob {

using OrderId = uint64_t;
using Price = int64_t;
using Qty = int64_t;

enum class Side : uint8_t { Buy, Sell };

struct Order {
    OrderId id;
    Side side;
    Price price;
    Qty qty;
    uint64_t ts = 0;
};

struct Trade {
    OrderId maker_id;
    OrderId taker_id;
    Price price;
    Qty qty;
    uint64_t ts = 0;
};

class OrderBook {
public:
    enum class Type { Limit, Market};

    void add_order(Order o);
    bool cancel_order(OrderId id);
    bool modify_order(OrderId id, Qty new_qty);

    std::vector<Trade> match(Order incoming, Type type = Type::Limit);

    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;
    Qty level_qty(Side side, Price price) const;

private:
    struct Level {
        Qty total = 0;
        std::deque<OrderId> q;
    };

    std::map<Price, Level, std::greater<Price>> bids_;
    std::map<Price, Level, std::less<Price>> asks_;
    std::unordered_map<OrderId, Order> orders_;

    template<typename Map>
    void erase_from(Map& levels, Price price, OrderId id, Qty qty);
};

} // namespace ob