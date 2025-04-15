# OPTIMIZATIONS

## Important Considerations

- Data structure choices
- Algorithm choices
- Cache locality: L2 cache misses cause large latency spikes
- Branch prediction: Add Likely and Unlikely attributes

## Basic Optimizations

### No optimizations

- Results: Add 789, Modify 11150, Volume 44, Total 50283

### modify_order: Avoid iterating over all price levels

- Results: Add 633, Modify 50, Volume 43, Total 519
- Store map of `(order_id, order)` to get order from order_id
- Use order price to find orders at price level directly instead of iterating over all price levels
