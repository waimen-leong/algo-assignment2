#include "algos/flow_graph.h"

#include <algorithm>
#include <queue>
#include <stdexcept>

FlowGraph::FlowGraph(int n) {
    if (n < 0) throw std::invalid_argument("FlowGraph: negative node count");
    adj.resize(n);
}

void FlowGraph::addEdge(int u, int v, long long cap) {
    const int n = numNodes();
    if (u < 0 || u >= n || v < 0 || v >= n)
        throw std::out_of_range("FlowGraph::addEdge: node out of range");
    if (cap < 0) throw std::invalid_argument("FlowGraph::addEdge: negative capacity");

    // Each edge stores where its partner lives. Read both sizes BEFORE pushing:
    // for a self-loop (u == v) the first push_back changes adj[v].size().
    const int idxInU = static_cast<int>(adj[u].size());
    const int idxInV = static_cast<int>(adj[v].size()) + (u == v ? 1 : 0);
    adj[u].push_back({v, cap, idxInV});  // forward
    adj[v].push_back({u, 0, idxInU});    // residual
}

long long FlowGraph::maxFlow(int s, int t) {
    const int n = numNodes();
    if (s < 0 || s >= n || t < 0 || t >= n)
        throw std::out_of_range("FlowGraph::maxFlow: node out of range");
    if (s == t) return 0;

    long long flow = 0;
    // parentNode[v] / parentEdge[v]: the edge adj[parentNode[v]][parentEdge[v]]
    // used to first reach v in the current BFS. parentNode[v] == -1 = unvisited.
    std::vector<int> parentNode(n), parentEdge(n);
    std::queue<int> q;

    while (true) {
        // 1. BFS from s over residual edges (cap > 0).
        std::fill(parentNode.begin(), parentNode.end(), -1);
        parentNode[s] = s;
        q = {};
        q.push(s);
        while (!q.empty() && parentNode[t] == -1) {
            const int u = q.front();
            q.pop();
            for (int i = 0; i < static_cast<int>(adj[u].size()); ++i) {
                const Edge& e = adj[u][i];
                if (e.cap > 0 && parentNode[e.to] == -1) {
                    parentNode[e.to] = u;
                    parentEdge[e.to] = i;
                    q.push(e.to);
                }
            }
        }
        if (parentNode[t] == -1) break;  // no augmenting path left: flow is max

        // 2. Bottleneck = smallest residual capacity on the path s -> t.
        long long bottleneck = FLOW_INF;
        for (int v = t; v != s; v = parentNode[v])
            bottleneck = std::min(bottleneck, adj[parentNode[v]][parentEdge[v]].cap);

        // 3. Push it: forward edges lose capacity, reverse edges gain it.
        for (int v = t; v != s; v = parentNode[v]) {
            Edge& e = adj[parentNode[v]][parentEdge[v]];
            e.cap -= bottleneck;
            adj[v][e.rev].cap += bottleneck;
        }
        flow += bottleneck;
    }
    return flow;
}

std::vector<bool> FlowGraph::reachableFromSource(int s) const {
    const int n = numNodes();
    if (s < 0 || s >= n)
        throw std::out_of_range("FlowGraph::reachableFromSource: node out of range");

    std::vector<bool> seen(n, false);
    std::queue<int> q;
    seen[s] = true;
    q.push(s);
    while (!q.empty()) {
        const int u = q.front();
        q.pop();
        for (const Edge& e : adj[u]) {
            if (e.cap > 0 && !seen[e.to]) {
                seen[e.to] = true;
                q.push(e.to);
            }
        }
    }
    return seen;
}
