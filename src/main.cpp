#include "order_book.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace ob;

static OrderId gid = 1;

// Probe up to LEVELS non-empty price levels starting from `start`,
// stepping by `step` (+1 for asks, -1 for bids).
static std::vector<std::pair<Price,Qty>>
gather_levels(const OrderBook& book, Side side, Price start, int step, int max) {
    std::vector<std::pair<Price,Qty>> out;
    Price p = start;
    int   scanned = 0;
    while ((int)out.size() < max && scanned < 500) {
        Qty q = book.level_qty(side, p);
        if (q > 0) out.push_back({p, q});
        p += step;
        ++scanned;
    }
    return out;
}

void print_book(const OrderBook& book) {
    const int LEVELS = 5;
    const int PW = 12, QW = 10, SW = 8;
    const int WIDTH = PW + QW + SW + 2;

    auto best_a = book.best_ask();
    auto best_b = book.best_bid();

    auto asks = best_a ? gather_levels(book, Side::Sell, *best_a, +1, LEVELS)
                       : std::vector<std::pair<Price,Qty>>{};
    auto bids = best_b ? gather_levels(book, Side::Buy,  *best_b, -1, LEVELS)
                       : std::vector<std::pair<Price,Qty>>{};

    std::cout << "\n" << std::string(WIDTH, '-') << "\n";
    std::cout << std::setw(PW) << "price"
              << std::setw(QW) << "qty"
              << std::setw(SW) << "side" << "\n";
    std::cout << std::string(WIDTH, '-') << "\n";

    // asks: worst to best (reverse)
    for (int i = (int)asks.size() - 1; i >= 0; --i)
        std::cout << std::setw(PW) << std::fixed << std::setprecision(2) << asks[i].first / 100.0
                  << std::setw(QW) << asks[i].second
                  << std::setw(SW) << "ASK" << "\n";

    if (best_a && best_b)
        std::cout << "  -- spread " << std::fixed << std::setprecision(2)
                  << (*best_a - *best_b) / 100.0 << " --\n";
    else
        std::cout << "  -- empty --\n";

    // bids: best to worst
    for (auto& [p, q] : bids)
        std::cout << std::setw(PW) << std::fixed << std::setprecision(2) << p / 100.0
                  << std::setw(QW) << q
                  << std::setw(SW) << "BID" << "\n";

    std::cout << std::string(WIDTH, '-') << "\n\n";
}

void print_trades(const std::vector<Trade>& trades) {
    if (trades.empty()) { std::cout << "  no fills\n\n"; return; }
    std::cout << "  fills:\n";
    for (auto& t : trades)
        std::cout << "    price=" << std::fixed << std::setprecision(2) << t.price / 100.0
                  << "  qty=" << t.qty << "\n";
    std::cout << "\n";
}

void seed_book(OrderBook& book) {
    struct { Side side; Price price; Qty qty; } orders[] = {
        {Side::Sell, 10050, 100}, {Side::Sell, 10025, 80},
        {Side::Sell, 10010, 60},  {Side::Sell, 10005, 40},
        {Side::Sell, 10002, 20},
        {Side::Buy,  9998,  20},  {Side::Buy,  9995,  40},
        {Side::Buy,  9990,  60},  {Side::Buy,  9975,  80},
        {Side::Buy,  9950,  100},
    };
    for (auto& o : orders)
        book.add_order({gid++, o.side, o.price, o.qty});
}

int main() {
    OrderBook book;
    seed_book(book);

    std::cout << "=== order book simulator ===\n";
    std::cout << "prices in dollars  |  commands:\n";
    std::cout << "  limit <buy|sell> <price> <qty>\n";
    std::cout << "  market <buy|sell> <qty>\n";
    std::cout << "  quit\n";

    print_book(book);

    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;

        std::istringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if (cmd == "quit" || cmd == "q") break;

        if (cmd == "limit") {
            std::string side_s; double price_d; Qty qty;
            if (!(ss >> side_s >> price_d >> qty)) {
                std::cout << "  usage: limit <buy|sell> <price> <qty>\n\n"; continue;
            }
            Side  side  = (side_s == "buy") ? Side::Buy : Side::Sell;
            Price price = static_cast<Price>(price_d * 100 + 0.5);
            print_trades(book.match({gid++, side, price, qty}, OrderBook::Type::Limit));
            print_book(book);

        } else if (cmd == "market") {
            std::string side_s; Qty qty;
            if (!(ss >> side_s >> qty)) {
                std::cout << "  usage: market <buy|sell> <qty>\n\n"; continue;
            }
            Side side = (side_s == "buy") ? Side::Buy : Side::Sell;
            print_trades(book.match({gid++, side, 0, qty}, OrderBook::Type::Market));
            print_book(book);

        } else {
            std::cout << "  unknown: limit / market / quit\n\n";
        }
    }
}