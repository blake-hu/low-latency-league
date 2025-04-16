#include "engine.hpp"
#include <functional>
#include <iostream>
#include <list>
#include <optional>
#include <stdexcept>

// This is an example correct implementation
// It is INTENTIONALLY suboptimal
// You are encouraged to rewrite as much or as little as you'd like

// Templated helper to process matching orders.
// The Condition predicate takes the price level and the incoming order price
// and returns whether the level qualifies.
template <typename OrderIdByPrices, typename OrderCatalog, typename Condition>
uint32_t process_orders(Order &incoming, OrderIdByPrices &orderIdByPrices,
                        OrderCatalog &orderCatalog, Condition cond) {
  uint32_t matchCount = 0;
  auto it = orderIdByPrices.begin();
  while (it != orderIdByPrices.end() && incoming.quantity > 0 &&
         (it->first == incoming.price || cond(it->first, incoming.price))) {
    auto &orderIdQueue = it->second;

    while (!orderIdQueue.empty() && incoming.quantity > 0) {
      auto &restingId = orderIdQueue.front();

      // check if order is valud
      auto restingOrderIt = orderCatalog.find(restingId);
      if (restingOrderIt ==
          orderCatalog.end()) { // order invalid, likely previously deleted
        orderIdQueue.pop(); // remove deleted order and continue to next orderId
        continue;
      }

      Order &restingOrder = restingOrderIt->second;
      QuantityType trade = std::min(incoming.quantity, restingOrder.quantity);
      incoming.quantity -= trade;
      restingOrder.quantity -= trade;
      ++matchCount;
      if (restingOrder.quantity == 0) { // order filled so remove
        orderCatalog.erase(restingOrder.id);
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

uint32_t match_order(Orderbook &orderbook, const Order &incoming) {
  uint32_t matchCount = 0;
  Order order = incoming; // Create a copy to modify the quantity

  if (order.side == Side::BUY) {
    // For a BUY, match with sell orders priced at or below the order's price.
    matchCount = process_orders(order, orderbook.sellOrderIdByPrices,
                                orderbook.orderCatalog, std::less<>());
    if (order.quantity > 0) {
      orderbook.buyOrderIdByPrices[order.price].push(order.id);
      orderbook.orderCatalog[order.id] = order;
    }
  } else { // Side::SELL
    // For a SELL, match with buy orders priced at or above the order's price.
    matchCount = process_orders(order, orderbook.buyOrderIdByPrices,
                                orderbook.orderCatalog, std::greater<>());
    if (order.quantity > 0) {
      orderbook.sellOrderIdByPrices[order.price].push(order.id);
      orderbook.orderCatalog[order.id] = order;
    }
  }

  return matchCount;
}

void modify_order_by_id(Orderbook &orderbook, IdType order_id,
                        QuantityType new_quantity) {
  auto orderIt = orderbook.orderCatalog.find(order_id);
  if (orderIt == orderbook.orderCatalog.end())
    return; // order does not exist
  Order &order = orderIt->second;

  if (new_quantity == 0) // delete order
  {
    orderbook.orderCatalog.erase(order_id);
    // we will lazily delete order_id in orderByPrices map.
    // basically, when we next encounter the deleted order_id,
    // we will delete it from the orderByPrices map
  } else // modify order
  {
    order.quantity = new_quantity;
  }
}

template <typename OrderIdByPrices, typename OrderCatalog>
uint32_t get_volume_at_level_for_side(OrderIdByPrices &idByPrices,
                                      OrderCatalog &catalog, PriceType price) {
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
    auto orderIt = catalog.find(id);
    if (orderIt != catalog.end())
      total += orderIt->second.quantity;

    // print_order(orderIt->second);
  }
  for (auto &id : new_orderIds) {
    orderIds.push(id);
  }
  return total;
}

uint32_t get_volume_at_level(Orderbook &orderbook, Side side, PriceType price) {
  if (side == Side::BUY) {
    return get_volume_at_level_for_side(orderbook.buyOrderIdByPrices,
                                        orderbook.orderCatalog, price);
  }
  
  return get_volume_at_level_for_side(orderbook.sellOrderIdByPrices,
                                      orderbook.orderCatalog, price);
}

// Functions below here don't need to be performant. Just make sure they're
// correct
Order lookup_order_by_id(Orderbook &orderbook, IdType order_id) {
  auto orderIt = orderbook.orderCatalog.find(order_id);
  if (orderIt == orderbook.orderCatalog.end())
    throw std::runtime_error("Order not found");
  return orderIt->second;
}

bool order_exists(Orderbook &orderbook, IdType order_id) {
  return orderbook.orderCatalog.find(order_id) != orderbook.orderCatalog.end();
}

Orderbook *create_orderbook() { return new Orderbook; }

void print_order(Order &order) {
  std::cout << "ID " << order.id << " | "
            << (order.side == Side::BUY ? "BUY" : "SELL") << " | Price "
            << order.price << " | Quantity " << order.quantity << std::endl;
}

void print_orderbook(Orderbook &ob) {
  auto catalog = ob.orderCatalog;
  std::cout << "-- ORDERBOOK --" << std::endl;
  for (auto it = catalog.begin(); it != catalog.end(); it++) {
    print_order(it->second);
  }
}