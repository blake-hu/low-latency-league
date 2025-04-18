#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <queue>

enum class Side : uint8_t { BUY, SELL };

using IdType = uint32_t;
using PriceType = uint16_t;
using QuantityType = uint16_t;

#define PRICE_LEVELS 1024

// You CANNOT change this
struct Order {
  IdType id; // Unique
  PriceType price;
  QuantityType quantity;
  Side side;
};

struct QueueVolume {
  std::queue<IdType> queue;
  QuantityType volume;
};

// TODO: Convert std::list to std::queue
struct Orderbook {
  std::array<std::optional<QueueVolume>, PRICE_LEVELS> buyOrderIdByPrices;
  std::array<std::optional<QueueVolume>, PRICE_LEVELS> sellOrderIdByPrices;
  std::array<std::optional<Order>, 1024> orderArray;
};

extern "C" {

// Takes in an incoming order, matches it, and returns the number of matches
// Partial fills are valid

uint32_t process_orders(Order &incoming, QueueVolume &queueVolume,
                        Orderbook &ob);

uint32_t match_order(Orderbook &orderbook, const Order &incoming);

// Sets the new quantity of an order. If new_quantity==0, removes the order
void modify_order_by_id(Orderbook &orderbook, IdType order_id,
                        QuantityType new_quantity);

// Returns total resting volume at a given price point
uint32_t get_volume_at_level(Orderbook &orderbook, Side side,
                             PriceType quantity);

// Performance of these do not matter. They are only used to check correctness
Order lookup_order_by_id(Orderbook &orderbook, IdType order_id);
bool order_exists(Orderbook &orderbook, IdType order_id);
Orderbook *create_orderbook();
}

void print_order(Order &order);

void print_orderbook(Orderbook &ob);
