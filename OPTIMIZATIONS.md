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

## Use queue of IdTypes instead of list

- Results: Add 848, Modify 23, Volume 212, Total 646
- Theoretically, queue is faster because it allocates a linked list of arrays, which has better spatial locality than a linked list of single elements
- However, we get slower runtimes
- This may be because hashing into an unordered_map is slow
- Try converting unordered_map into an array next
