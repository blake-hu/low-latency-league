#include "engine.hpp"
#include <cstddef>
#include <functional>
#include <iostream>
#include <list>
#include <stdexcept>

// This is an example correct implementation
// It is INTENTIONALLY suboptimal
// You are encouraged to rewrite as much or as little as you'd like

// Templated helper to process matching orders.
// The Condition predicate takes the price level and the incoming order price
// and returns whether the level qualifies.
template <typename OrderIdByPrices, typename OrderArray, typename OrderPresent,
          typename Condition>
uint32_t process_orders(Order &incoming, OrderIdByPrices &orderIdByPrices,
                        OrderArray &orderArray, OrderPresent &orderPresent,
                        Condition cond) {
  uint32_t matchCount = 0;
  auto it = orderIdByPrices.begin();
  while (it != orderIdByPrices.end() && incoming.quantity > 0 &&
         (it->first == incoming.price || cond(it->first, incoming.price))) {
    auto &orderIdQueue = it->second;

    while (!orderIdQueue.empty() && incoming.quantity > 0) {
      auto &restingId = orderIdQueue.front();

      // check if order is valid
      if (!orderPresent[restingId]) { // order invalid, likely previously
                                      // deleted
        orderIdQueue.pop(); // remove deleted order and continue to next orderId
        continue;
      }

      Order &restingOrder = orderArray[restingId];
      QuantityType trade = std::min(incoming.quantity, restingOrder.quantity);
      incoming.quantity -= trade;
      restingOrder.quantity -= trade;
      ++matchCount;
      if (restingOrder.quantity == 0) { // order filled so remove
        orderPresent[restingId] = false;
        orderIdQueue.pop();
      }
    }
    if (orderIdQueue.empty())
      it = orderIdByPrices.erase(it);
    else
      ++it;
  }

  return matchCount;
}

uint32_t match_order(Orderbook &ob, const Order &incoming) {
  uint32_t matchCount = 0;
  Order order = incoming; // Create a copy to modify the quantity

  if (order.side == Side::BUY) {
    // For a BUY, match with sell orders priced at or below the order's price.
    matchCount = process_orders(order, ob.sellOrderIdByPrices, ob.orderArray,
                                ob.orderPresent, std::less<>());
    if (order.quantity > 0) {
      ob.buyOrderIdByPrices[order.price].push(order.id);
      ob.orderArray[order.id] = order;
      ob.orderPresent[order.id] = true;
    }
  } else { // Side::SELL
    // For a SELL, match with buy orders priced at or above the order's price.
    matchCount = process_orders(order, ob.buyOrderIdByPrices, ob.orderArray,
                                ob.orderPresent, std::greater<>());
    if (order.quantity > 0) {
      ob.sellOrderIdByPrices[order.price].push(order.id);
      ob.orderArray[order.id] = order;
      ob.orderPresent[order.id] = true;
    }
  }

  return matchCount;
}

void modify_order_by_id(Orderbook &orderbook, IdType order_id,
                        QuantityType new_quantity) {
  if (!orderbook.orderPresent[order_id])
    return; // order does not exist
  Order &order = orderbook.orderArray[order_id];

  if (new_quantity == 0) // delete order
  {
    orderbook.orderPresent[order_id] = false;
    // we will lazily delete order_id in orderByPrices map.
    // basically, when we next encounter the deleted order_id,
    // we will delete it from the orderByPrices map
  } else // modify order
  {
    order.quantity = new_quantity;
  }
}

template <typename OrderIdByPrices>
uint32_t get_volume_at_level_for_side(OrderIdByPrices &idByPrices,
                                      Orderbook &ob, PriceType price) {
  uint32_t total = 0;
  auto orderIdIt = idByPrices.find(price);
  if (orderIdIt == idByPrices.end()) {
    return 0;
  }
  auto orderIds = orderIdIt->second;
  std::list<IdType> new_orderIds;
  while (!orderIds.empty()) {
    auto id = orderIds.front();
    orderIds.pop();
    new_orderIds.push_back(id);
    if (ob.orderPresent[id])
      total += ob.orderArray[id].quantity;

    // print_order(orderIt->second);
  }
  for (auto &id : new_orderIds) {
    orderIds.push(id);
  }
  return total;
}

uint32_t get_volume_at_level(Orderbook &orderbook, Side side, PriceType price) {
  if (side == Side::BUY) {
    return get_volume_at_level_for_side(orderbook.buyOrderIdByPrices, orderbook,
                                        price);
  }

  return get_volume_at_level_for_side(orderbook.sellOrderIdByPrices, orderbook,
                                      price);
}

// Functions below here don't need to be performant. Just make sure they're
// correct
Order lookup_order_by_id(Orderbook &orderbook, IdType order_id) {
  if (!orderbook.orderPresent[order_id])
    throw std::runtime_error("Order not found");
  return orderbook.orderArray[order_id];
}

bool order_exists(Orderbook &orderbook, IdType order_id) {
  return orderbook.orderPresent[order_id];
}

Orderbook *create_orderbook() { return new Orderbook; }

void print_order(Order &order) {
  std::cout << "ID " << order.id << " | "
            << (order.side == Side::BUY ? "BUY" : "SELL") << " | Price "
            << order.price << " | Quantity " << order.quantity << std::endl;
}

void print_orderbook(Orderbook &ob) {
  std::cout << "-- ORDERBOOK --" << std::endl;
  for (std::size_t i = 0; i < ob.orderArray.size(); i++) {
    if (ob.orderPresent[i])
      print_order(ob.orderArray[i]);
  }
}