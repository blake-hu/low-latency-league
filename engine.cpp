#include "engine.hpp"
#include <cstddef>
#include <exception>
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
uint32_t process_orders(Order &incoming, QueueVolume &queueVolume,
                        Orderbook &ob) {
  uint32_t matchCount = 0;
  auto &queue = queueVolume.queue;
  while (!queue.empty() && incoming.quantity > 0) {
    auto &restingId = queue.front();

    // check if order is valid
    if (!ob.orderArray[restingId].has_value()) { // order invalid, likely
                                                 // previously deleted
      queue.pop(); // remove deleted order and continue to next orderId
      continue;
    }

    Order &restingOrder = ob.orderArray[restingId].value();
    QuantityType trade = std::min(incoming.quantity, restingOrder.quantity);
    incoming.quantity -= trade;
    restingOrder.quantity -= trade;
    queueVolume.volume -= trade;
    ++matchCount;
    if (restingOrder.quantity == 0) { // order filled so remove
      ob.orderArray[restingId] = {};
      queue.pop();
    }
  }

  return matchCount;
}

uint32_t match_order(Orderbook &ob, const Order &incoming) {
  uint32_t matchCount = 0;
  Order order = incoming; // Create a copy to modify the quantity

  if (order.side == Side::BUY) {
    // For a BUY, match with sell orders priced at or below the order's price.
    for (PriceType price = 0; price <= order.price; price++) {
      if (!ob.sellOrderIdByPrices[price].has_value()) {
        continue;
      }
      auto &queueVolume = ob.sellOrderIdByPrices[price].value();
      matchCount += process_orders(order, queueVolume, ob);
      if (queueVolume.queue.empty()) {
        ob.sellOrderIdByPrices[price] = {};
      }
      if (order.quantity == 0) {
        return matchCount;
      }
    }
    if (order.quantity > 0) {
      if (!ob.buyOrderIdByPrices[order.price].has_value()) {
        ob.buyOrderIdByPrices[order.price].emplace();
      }
      ob.buyOrderIdByPrices[order.price]->queue.push(order.id);
      ob.buyOrderIdByPrices[order.price]->volume += order.quantity;
      ob.orderArray[order.id].emplace(order);
    }
  } else { // Side::SELL
    // For a SELL, match with buy orders priced at or above the order's price.
    for (PriceType price = PRICE_LEVELS; price >= order.price; price--) {
      if (!ob.buyOrderIdByPrices[price].has_value()) {
        continue;
      }
      auto &queueVolume = ob.buyOrderIdByPrices[price].value();
      matchCount += process_orders(order, queueVolume, ob);
      if (queueVolume.queue.empty()) {
        ob.buyOrderIdByPrices[price] = {};
      }
      if (order.quantity == 0) {
        return matchCount;
      }
    }
    if (order.quantity > 0) {
      if (!ob.sellOrderIdByPrices[order.price].has_value()) {
        ob.sellOrderIdByPrices[order.price].emplace();
      }
      ob.sellOrderIdByPrices[order.price]->queue.push(order.id);
      ob.sellOrderIdByPrices[order.price]->volume += order.quantity;
      ob.orderArray[order.id].emplace(order);
    }
  }

  return matchCount;
}

void modify_order_by_id(Orderbook &ob, IdType order_id,
                        QuantityType new_quantity) {
  if (!ob.orderArray[order_id].has_value()) {
    return;
  }
  Order &order = ob.orderArray[order_id].value();
  auto delta = new_quantity - order.quantity;

  if (new_quantity == 0) // delete order
  {
    ob.orderArray[order_id] = {};
    // we will lazily delete order_id in orderByPrices map.
    // basically, when we next encounter the deleted order_id,
    // we will delete it from the orderByPrices map
  } else // modify order
  {
    order.quantity = new_quantity;
  }

  if (order.side == Side::BUY) {
    if (ob.buyOrderIdByPrices[order.price].has_value()) {
      ob.buyOrderIdByPrices[order.price]->volume += delta;
    }
  } else {
    if (ob.sellOrderIdByPrices[order.price].has_value()) {
      ob.sellOrderIdByPrices[order.price]->volume += delta;
    }
  }
}

uint32_t get_volume_at_level(Orderbook &orderbook, Side side, PriceType price) {
  if (side == Side::BUY) {
    return orderbook.buyOrderIdByPrices[price]->volume;
  } else {
    return orderbook.sellOrderIdByPrices[price]->volume;
  }
}

// Functions below here don't need to be performant. Just make sure they're
// correct
Order lookup_order_by_id(Orderbook &orderbook, IdType order_id) {
  if (!orderbook.orderArray[order_id].has_value())
    throw std::runtime_error("Order not found");
  return orderbook.orderArray[order_id].value();
}

bool order_exists(Orderbook &orderbook, IdType order_id) {
  return orderbook.orderArray[order_id].has_value();
}

Orderbook *create_orderbook() { return new Orderbook; }

void print_order(Order &order) {
  std::cout << "ID " << order.id << " | "
            << (order.side == Side::BUY ? "BUY" : "SELL") << " | Price "
            << order.price << " | Quantity " << order.quantity << std::endl;
}

void print_orderbook(Orderbook &ob) {
  std::cout << std::endl << "-- ORDERBOOK --" << std::endl;
  for (std::size_t i = 0; i < ob.orderArray.size(); i++) {
    if (ob.orderArray[i].has_value()) {
      print_order(ob.orderArray[i].value());
    }
  }

  std::cout << "-- BUY VOLUME --" << std::endl;
  for (std::size_t i = 0; i < ob.buyOrderIdByPrices.size(); i++) {
    if (ob.buyOrderIdByPrices[i].has_value()) {
      std::cout << "Price " << i << ": " << ob.buyOrderIdByPrices[i]->volume
                << std::endl;
    }
  }

  std::cout << "-- SELL VOLUME --" << std::endl;
  for (std::size_t i = 0; i < ob.sellOrderIdByPrices.size(); i++) {
    if (ob.sellOrderIdByPrices[i].has_value()) {
      std::cout << "Price " << i << ": " << ob.sellOrderIdByPrices[i]->volume
                << std::endl;
    }
  }
}