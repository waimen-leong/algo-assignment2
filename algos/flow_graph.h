#pragma once
#include <vector>

// Directed flow network with integer capacities, solved by Edmonds-Karp.

// "Infinite" capacity. Large enough that no finite cut ever reaches it, and
// small enough that adding a few of them cannot overflow a long long.
constexpr long long FLOW_INF = 1'000'000'000'000'000LL;  // 1e15

struct Edge {
    int to;         // head of the edge
    long long cap;  // REMAINING (residual) capacity
    int rev;        // index of the reverse edge in adj[to]
};

class FlowGraph {
public:
    explicit FlowGraph(int n);

    // Adds u -> v with capacity cap, plus the residual edge v -> u with
    // capacity 0. The residual edge lets a later augmenting path "undo" flow.
    void addEdge(int u, int v, long long cap);

    // Maximum s -> t flow (Edmonds-Karp: repeatedly augment along the
    // SHORTEST residual path, found by BFS). Mutates the residual capacities,
    // so call it once per graph.
    long long maxFlow(int s, int t);

    // After maxFlow: true for every node still reachable from s through edges
    // with cap > 0. That set is the S side of a minimum cut.
    std::vector<bool> reachableFromSource(int s) const;

    int numNodes() const { return static_cast<int>(adj.size()); }
    const std::vector<Edge>& edgesFrom(int u) const { return adj[u]; }

private:
    std::vector<std::vector<Edge>> adj;
};
