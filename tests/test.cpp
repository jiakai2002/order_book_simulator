#include <gtest/gtest.h>
#include "order_book.hpp"

using namespace ob;

static OrderId gid = 1;
static Order buy (Price p, Qty q, uint64_t ts = 0) { return {gid++, Side::Buy,  p, q, ts}; }
static Order sell(Price p, Qty q, uint64_t ts = 0) { return {gid++, Side::Sell, p, q, ts}; }

class OrderBookTest : public ::testing::Test {
protected:
    OrderBook book;
};

// ── Best price ───────────────────────────────────────────────────────────────

TEST_F(OrderBookTest, EmptyBookHasNoBestPrices) {
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
}

TEST_F(OrderBookTest, BestBidIsHighestBidPrice) {
    book.match(buy(98, 1));
    book.match(buy(100, 1));
    book.match(buy(99, 1));
    EXPECT_EQ(*book.best_bid(), 100);
}

TEST_F(OrderBookTest, BestAskIsLowestAskPrice) {
    book.match(sell(102, 1));
    book.match(sell(100, 1));
    book.match(sell(101, 1));
    EXPECT_EQ(*book.best_ask(), 100);
}

// ── No match ─────────────────────────────────────────────────────────────────

TEST_F(OrderBookTest, NoMatchWhenSpreadExists) {
    book.match(sell(101, 10));
    auto trades = book.match(buy(100, 10));
    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(*book.best_bid(), 100);
    EXPECT_EQ(*book.best_ask(), 101);
}

// ── Full fill ────────────────────────────────────────────────────────────────

TEST_F(OrderBookTest, FullFillClearsBook) {
    book.match(sell(100, 10));
    auto trades = book.match(buy(100, 10));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[0].qty, 10);
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
}

TEST_F(OrderBookTest, FillPriceIsMakerPrice) {
    book.match(sell(100, 5));
    auto trades = book.match(buy(105, 5));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].price, 100);
}

// ── Partial fill ─────────────────────────────────────────────────────────────

TEST_F(OrderBookTest, PartialFillRestsRemainder) {
    book.match(sell(100, 3));
    auto trades = book.match(buy(100, 10));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].qty, 3);
    EXPECT_EQ(book.level_qty(Side::Buy, 100), 7);
    EXPECT_FALSE(book.best_ask().has_value());
}

// ── Sweep multiple levels ─────────────────────────────────────────────────────

TEST_F(OrderBookTest, SweepMultipleLevels) {
    book.match(sell(100, 3));
    book.match(sell(101, 4));
    book.match(sell(102, 5));
    auto trades = book.match(buy(102, 10));
    ASSERT_EQ(trades.size(), 3u);
    EXPECT_EQ(trades[0].price, 100); EXPECT_EQ(trades[0].qty, 3);
    EXPECT_EQ(trades[1].price, 101); EXPECT_EQ(trades[1].qty, 4);
    EXPECT_EQ(trades[2].price, 102); EXPECT_EQ(trades[2].qty, 3);
    EXPECT_EQ(book.level_qty(Side::Sell, 102), 2);
}

TEST_F(OrderBookTest, SellSweepsBids) {
    book.match(buy(100, 5));
    book.match(buy(99,  5));
    auto trades = book.match(sell(99, 8));
    ASSERT_EQ(trades.size(), 2u);
    EXPECT_EQ(trades[0].price, 100); EXPECT_EQ(trades[0].qty, 5);
    EXPECT_EQ(trades[1].price, 99);  EXPECT_EQ(trades[1].qty, 3);
}

// ── FIFO priority ─────────────────────────────────────────────────────────────

TEST_F(OrderBookTest, FIFOPriorityAtSameLevel) {
    auto s1 = sell(100, 5); OrderId id1 = s1.id;
    auto s2 = sell(100, 5);
    book.match(s1);
    book.match(s2);
    auto trades = book.match(buy(100, 5));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].maker_id, id1);
}

// ── Timestamps ───────────────────────────────────────────────────────────────

TEST_F(OrderBookTest, TradeInheritsTrakerTimestamp) {
    book.match(sell(100, 5, 1000));
    auto trades = book.match(buy(100, 5, 9000));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].ts, 9000u);
}

TEST_F(OrderBookTest, OrderTimestampPreserved) {
    auto o = sell(100, 5, 12345);
    book.add_order(o);
    EXPECT_EQ(book.level_qty(Side::Sell, 100), 5); // order is in book with ts
}

// ── Order types ───────────────────────────────────────────────────────────────

TEST_F(OrderBookTest, MarketOrderFillsAtAnyPrice) {
    book.match(sell(99, 10));
    auto trades = book.match(buy(0, 10), OrderBook::Type::Market);
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].qty, 10);
    EXPECT_FALSE(book.best_ask().has_value());
}

TEST_F(OrderBookTest, MarketOrderNeverRests) {
    auto trades = book.match(buy(0, 10), OrderBook::Type::Market);
    EXPECT_TRUE(trades.empty());
    EXPECT_FALSE(book.best_bid().has_value());
}

// ── Cancel ────────────────────────────────────────────────────────────────────

TEST_F(OrderBookTest, CancelRemovesOrder) {
    auto o = sell(100, 10);
    OrderId id = o.id;
    book.match(o);
    EXPECT_TRUE(book.cancel_order(id));
    EXPECT_FALSE(book.best_ask().has_value());
}

TEST_F(OrderBookTest, CancelUnknownOrderReturnsFalse) {
    EXPECT_FALSE(book.cancel_order(9999));
}

TEST_F(OrderBookTest, CancelBestBidFallsBackToNext) {
    auto o1 = buy(100, 5); OrderId id1 = o1.id;
    book.match(o1);
    book.match(buy(99, 5));
    book.cancel_order(id1);
    EXPECT_EQ(*book.best_bid(), 99);
}

// ── Modify ────────────────────────────────────────────────────────────────────

TEST_F(OrderBookTest, ModifyDecreaseKeepsTimePriority) {
    auto s1 = sell(100, 10); OrderId id1 = s1.id;
    book.match(s1);
    book.match(sell(100, 5));
    book.modify_order(id1, 2);
    auto trades = book.match(buy(100, 2));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].maker_id, id1);
}

TEST_F(OrderBookTest, ModifyIncreaseMovesToBack) {
    auto s1 = sell(100, 5); OrderId id1 = s1.id;
    auto s2 = sell(100, 5); OrderId id2 = s2.id;
    book.match(s1);
    book.match(s2);
    book.modify_order(id1, 10);
    auto trades = book.match(buy(100, 5));
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].maker_id, id2);
}

TEST_F(OrderBookTest, ModifyToZeroCancels) {
    auto o = sell(100, 5); OrderId id = o.id;
    book.match(o);
    EXPECT_TRUE(book.modify_order(id, 0));
    EXPECT_FALSE(book.best_ask().has_value());
}

TEST_F(OrderBookTest, ModifyUnknownOrderReturnsFalse) {
    EXPECT_FALSE(book.modify_order(9999, 5));
}

// ── Validation ───────────────────────────────────────────────────────────────

TEST_F(OrderBookTest, AddZeroQtyThrows) {
    EXPECT_THROW(book.add_order({gid++, Side::Buy, 100, 0}), std::invalid_argument);
}

TEST_F(OrderBookTest, DuplicateOrderIdThrows) {
    auto o = buy(100, 5);
    book.match(o);
    EXPECT_THROW(book.add_order(o), std::invalid_argument);
}