#include "engine.hpp"
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>

// This is an example correct implementation
// It is INTENTIONALLY suboptimal
// You are encouraged to rewrite as much or as little as you'd like

// Templated helper to process matching orders.
// The Condition predicate takes the price level and the incoming order price
// and returns whether the level qualifies.
template <typename OrderIdByPrices, typename OrderCatalog, typename Condition>
uint32_t process_orders(Order &order, OrderIdByPrices &orderIdByPrices,
                        OrderCatalog &orderCatalog, Condition cond) {
  uint32_t matchCount = 0;
  auto it = orderIdByPrices.begin();
  while (it != orderIdByPrices.end() && order.quantity > 0 &&
         (it->first == order.price || cond(it->first, order.price))) {
    auto &orderIdList = it->second;
    
    for (auto idIt = orderIdList.begin();
         idIt != orderIdList.end() && order.quantity > 0;) {
      auto restingOrderIt = orderCatalog.find(*idIt);
      if (restingOrderIt == orderCatalog.end()) { // order previously deleted
        idIt = orderIdList.erase(idIt);
        continue;
      }

      Order &restingOrder = restingOrderIt->second;
      QuantityType trade = std::min(order.quantity, restingOrder.quantity);
      order.quantity -= trade;
      restingOrder.quantity -= trade;
      ++matchCount;
      if (restingOrder.quantity == 0) {
        idIt = orderIdList.erase(idIt);
        orderCatalog.erase(restingOrder.id);
      } else
        ++idIt;
    }
    if (orderIdList.empty())
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
      orderbook.buyOrderIdByPrices[order.price].push_back(order.id);
      orderbook.orderCatalog[order.id] = order;
    }
  } else { // Side::SELL
    // For a SELL, match with buy orders priced at or above the order's price.
    matchCount = process_orders(order, orderbook.buyOrderIdByPrices,
                                orderbook.orderCatalog, std::greater<>());
    if (order.quantity > 0) {
      orderbook.sellOrderIdByPrices[order.price].push_back(order.id);
      orderbook.orderCatalog[order.id] = order;
    }
  }

  return matchCount;
}

// Templated helper to cancel an order within a given orders map.
template <typename OrderIdByPrice>
void delete_order_in_map(OrderIdByPrice &orderIdByPrice, IdType order_id,
                         PriceType price) {
  auto orderListIt = orderIdByPrice.find(price);
  if (orderListIt == orderIdByPrice.end())
    return; // order not found
  auto &orderList = orderListIt->second;

  for (auto idIt = orderList.begin(); idIt != orderList.end(); ++idIt) {
    if (*idIt == order_id) {
      idIt = orderList.erase(idIt);
      break;
    }
  }

  if (orderList.empty())
    orderIdByPrice.erase(price);
}

void modify_order_by_id(Orderbook &orderbook, IdType order_id,
                        QuantityType new_quantity) {
  auto orderIt = orderbook.orderCatalog.find(order_id);
  if (orderIt == orderbook.orderCatalog.end())
    return; // order does not exist
  Order &order = orderIt->second;

  if (new_quantity == 0) // delete order
  {
    if (order.side == Side::BUY)
      delete_order_in_map(orderbook.buyOrderIdByPrices, order_id, order.price);
    else
      delete_order_in_map(orderbook.sellOrderIdByPrices, order_id, order.price);
    orderbook.orderCatalog.erase(order_id);
  } else // modify order
  {
    order.quantity = new_quantity;
  }
}

uint32_t get_volume_at_level(Orderbook &orderbook, Side side, PriceType price) {
  uint32_t total = 0;
  if (side == Side::BUY) {
    auto buy_orders = orderbook.buyOrderIdByPrices.find(price);
    if (buy_orders == orderbook.buyOrderIdByPrices.end()) {
      return 0;
    }
    for (const auto &id : buy_orders->second) {
      total += orderbook.orderCatalog[id].quantity;
    }
  } else if (side == Side::SELL) {
    auto sell_orders = orderbook.sellOrderIdByPrices.find(price);
    if (sell_orders == orderbook.sellOrderIdByPrices.end()) {
      return 0;
    }
    for (const auto &id : sell_orders->second) {
      total += orderbook.orderCatalog[id].quantity;
    }
  }
  return total;
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

void print_orderbook(Orderbook ob) {
  auto catalog = ob.orderCatalog;
  std::cout << "-- ORDERBOOK --" << std::endl;
  for (auto it = catalog.begin(); it != catalog.end(); it++) {
    const auto &order = it->second;
    std::cout << "ID " << order.id << " | Price " << order.price
              << " | Quantity " << order.quantity << std::endl;
  }
}