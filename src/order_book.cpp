#include "order_book.hpp"
#include <algorithm>

namespace ob {

template<typename Map>
void OrderBook::erase_from(Map& levels, Price price, OrderId id, Qty qty) {
    auto it = levels.find(price);
    if (it == levels.end()) return;
    auto& lv = it->second;
    // decrease level qty
    lv.total -= qty;
    auto& q = lv.q;
    // remove order from queue
    q.erase(std::remove(q.begin(), q.end(), id), q.end());
    if (lv.total == 0) levels.erase(it);
}

std::optional<Price> OrderBook::best_bid() const {
    return bids_.empty() ? std::nullopt : std::optional{bids_.begin()->first};
}

std::optional<Price> OrderBook::best_ask() const {
    return asks_.empty() ? std::nullopt : std::optional{asks_.begin()->first};
}

Qty OrderBook::level_qty(Side side, Price price) const {
    auto get = [&](const auto& m) {
        auto it = m.find(price);
        return it != m.end() ? it->second.total : 0;
    };
    return side == Side::Buy ? get(bids_) : get(asks_);
}

void OrderBook::add_order(Order o) {
    if (o.qty<=0) throw std::invalid_argument("qty must be positive");
    if (orders_.count(o.id)) throw std::invalid_argument("duplicate order id");

    orders_.emplace(o.id, o);

    Level& lv = (o.side == Side::Buy) ? bids_[o.price] : asks_[o.price];
    lv.total += o.qty;
    lv.q.push_back(o.id);
}

bool OrderBook::cancel_order(OrderId id) {
    auto it = orders_.find(id);
    if (it == orders_.end()) return false;

    auto& o = it->second;
    if (o.side == Side::Buy) {
        erase_from(bids_, o.price, o.id, o.qty);
    }
    else {
        erase_from(asks_, o.price, o.id, o.qty);
    }

    orders_.erase(id);
    return true;
}

bool OrderBook::modify_order(OrderId id, Qty new_qty) {
    if (new_qty <= 0) return cancel_order(id);
    auto it = orders_.find(id);
    if (it == orders_.end()) return false;
    auto& o = it->second;

    Qty delta = new_qty - o.qty;
    Level& lv = (o.side == Side::Buy) ? bids_[o.price] : asks_[o.price];
    lv.total += delta;
    // if increase qty must queue again
    if (delta > 0) {
        auto& q = lv.q;
        q.erase(std::remove(q.begin(), q.end(), id), q.end());
        q.push_back(id);
    }
    o.qty = new_qty;
    return true;
}

std::vector<Trade> OrderBook::match(Order taker, Type type) {
    std::vector<Trade> trades;
    Qty rem = taker.qty;

    auto fill = [&](auto& levels) {
        while (rem > 0 && !levels.empty()) {
            // best level
            auto& [maker_price, lv] = *levels.begin();
            // buyer won't pay more for limit, seller won't sell less
            if (type != Type::Market) {
                if (taker.side == Side::Buy && maker_price > taker.price) break;
                if (taker.side == Side::Sell && maker_price < taker.price) break;
            }
            // fill within level
            while (rem > 0 && !lv.q.empty()) {
                // fill w maker
                OrderId mid = lv.q.front();
                auto& maker = orders_.at(mid);
                Qty qty = std::min(rem, maker.qty);
                trades.push_back({mid, taker.id, maker_price, qty, taker.ts});
                maker.qty -= qty;
                lv.total -= qty;
                rem -= qty;
                // remove maker if empty
                if (maker.qty == 0) { orders_.erase(mid); lv.q.pop_front(); }
            }
            // remove level if empty
            if (lv.total == 0) levels.erase(levels.begin());
        }
    };

    taker.side == Side::Buy ? fill(asks_) : fill(bids_);

    // limit orders partial filled, add back to queue
    if (rem > 0 && type == Type::Limit) {
        taker.qty = rem;
        add_order(taker);
    }

    return trades;
}

} // namespace ob
