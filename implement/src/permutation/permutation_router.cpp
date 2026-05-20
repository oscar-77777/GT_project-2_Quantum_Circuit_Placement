// ============================================================
// Fast Permutation Circuit (Section V-B)
// ============================================================
// Divide-and-conquer: split the fast-interaction graph into two balanced
// halves G1, G2 connected by a channel edge (u,v).  Phase A bubbles
// misplaced values through the channel; Phase B recurses independently.

#include "permutation_router.h"
#include <algorithm>
#include <queue>
#include <cassert>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

PermutationRouter::PermutationRouter(const PhysicalEnvironment& env, Weight threshold)
    : env_(env), threshold_(threshold),
      adj_(env.fastAdjacency(threshold))
{}

SwapCircuit PermutationRouter::routeBetween(const Placement& from, const Placement& to) {
    std::vector<int> perm = from.permutationTo(to);
    return route(perm);
}

SwapCircuit PermutationRouter::route(const std::vector<int>& perm) {
    int n = static_cast<int>(perm.size());

    // state[i] = logical value currently at nucleus i (starts as identity)
    std::vector<int> state(n);
    std::iota(state.begin(), state.end(), 0);

    std::vector<int> target = perm; // target[i] = value that should end at nucleus i

    std::vector<NucleusID> allNodes(n);
    std::iota(allNodes.begin(), allNodes.end(), 0);

    std::vector<SwapLevel> levels;
    routeSubgraph(state, target, allNodes, adj_, levels, 0);

    SwapCircuit result;
    for (auto& lvl : levels)
        if (!lvl.empty())
            result.addLevel(std::move(lvl));
    return result;
}

// ---------------------------------------------------------------------------
// partition: balanced connected cut of the subgraph induced by `nodes`
// ---------------------------------------------------------------------------
// Uses BFS spanning tree; removes the tree edge whose removal gives the most
// balanced split.  Returns G1, G2, and the channel edge (parent, child).

PermutationRouter::Partition PermutationRouter::partition(
        const std::vector<NucleusID>& nodes,
        const std::vector<std::vector<NucleusID>>& adj) const
{
    int n = static_cast<int>(nodes.size());
    if (n == 2)
        return {{nodes[0]}, {nodes[1]}, {nodes[0], nodes[1]}};

    std::unordered_set<NucleusID> nodeSet(nodes.begin(), nodes.end());

    // BFS spanning tree rooted at nodes[0]
    std::unordered_map<NucleusID, NucleusID> parent;
    std::vector<NucleusID> bfsOrder;
    parent[nodes[0]] = static_cast<NucleusID>(-1);
    std::queue<NucleusID> q;
    q.push(nodes[0]);
    while (!q.empty()) {
        NucleusID x = q.front(); q.pop();
        bfsOrder.push_back(x);
        for (NucleusID nb : adj[x]) {
            if (nodeSet.count(nb) && !parent.count(nb)) {
                parent[nb] = x;
                q.push(nb);
            }
        }
    }

    // Subtree sizes (bottom-up)
    std::unordered_map<NucleusID, int> subtreeSize;
    for (NucleusID nd : nodes) subtreeSize[nd] = 1;
    for (int i = static_cast<int>(bfsOrder.size()) - 1; i >= 1; --i) {
        NucleusID x = bfsOrder[i];
        subtreeSize[parent[x]] += subtreeSize[x];
    }

    // Guard: graph may be disconnected at low thresholds (some nodes unreachable from nodes[0]).
    // In that case, split into reachable (G1) and unreachable (G2) components.
    // Routing across disconnected components is impossible with fast SWAPs; the caller
    // will detect this and skip the channel SWAP step.
    if (static_cast<int>(bfsOrder.size()) < n) {
        std::unordered_set<NucleusID> reachable(bfsOrder.begin(), bfsOrder.end());
        Partition result;
        for (NucleusID x : nodes) {
            if (reachable.count(x)) result.g1.push_back(x);
            else                    result.g2.push_back(x);
        }
        // Channel sentinel: use last of G1 and first of G2 (not a real fast edge)
        result.channel = {result.g1.back(), result.g2.front()};
        return result;
    }

    // Find tree edge giving most balanced split
    NucleusID bestChild = bfsOrder[1];
    int bestDiff = n + 1;
    for (int i = 1; i < static_cast<int>(bfsOrder.size()); ++i) {
        NucleusID x = bfsOrder[i];
        int diff = std::abs(subtreeSize[x] - (n - subtreeSize[x]));
        if (diff < bestDiff) { bestDiff = diff; bestChild = x; }
    }

    // G2 = subtree of bestChild; walk up parent chain to test membership
    std::unordered_set<NucleusID> g2Set;
    for (NucleusID x : bfsOrder) {
        NucleusID cur = x;
        while (cur != static_cast<NucleusID>(-1) && cur != nodes[0]) {
            if (cur == bestChild) { g2Set.insert(x); break; }
            cur = parent[cur];
        }
        if (x == bestChild) g2Set.insert(x);
    }

    Partition result;
    for (NucleusID x : nodes) {
        if (g2Set.count(x)) result.g2.push_back(x);
        else                 result.g1.push_back(x);
    }
    result.channel = {parent[bestChild], bestChild};
    return result;
}

// ---------------------------------------------------------------------------
// routeSubgraph: recursive divide-and-conquer SWAP routing
// ---------------------------------------------------------------------------
// Phase A: propagate "bubbles" (misplaced values) through spanning trees
//   toward the channel (u,v), then swap across the channel.
//   Even steps: bubble moves within G1/G2 trees toward channel roots.
//   Odd steps: swap across channel edge.
// Phase B: recurse independently on G1 and G2 (interleaved levels).

void PermutationRouter::routeSubgraph(
        std::vector<int>&                      state,
        const std::vector<int>&                target,
        const std::vector<NucleusID>&          nodes,
        const std::vector<std::vector<NucleusID>>& adj,
        std::vector<SwapLevel>&                levels,
        int                                    levelOffset)
{
    if (static_cast<int>(nodes.size()) <= 1) return;

    // Early exit if already sorted within this subgraph
    bool done = true;
    for (NucleusID n : nodes) if (state[n] != target[n]) { done = false; break; }
    if (done) return;

    auto part = partition(nodes, adj);
    const auto& G1 = part.g1;
    const auto& G2 = part.g2;
    NucleusID u = part.channel.first;   // G1 root, adjacent to G2
    NucleusID v = part.channel.second;  // G2 root, adjacent to G1

    std::unordered_set<NucleusID> g1Set(G1.begin(), G1.end());
    std::unordered_set<NucleusID> g2Set(G2.begin(), G2.end());

    // invTarget[val] = nucleus where `val` should eventually reside
    std::unordered_map<int, NucleusID> invTarget;
    for (NucleusID n : nodes) invTarget[target[n]] = n;

    auto isG1Bound = [&](int val) -> bool {
        auto it = invTarget.find(val);
        return it != invTarget.end() && g1Set.count(it->second) > 0;
    };
    auto isG2Bound = [&](int val) -> bool {
        auto it = invTarget.find(val);
        return it != invTarget.end() && g2Set.count(it->second) > 0;
    };

    // Build BFS spanning tree of a group, rooted at `root`
    auto buildTree = [&](const std::vector<NucleusID>& group, NucleusID root) {
        std::unordered_map<NucleusID, NucleusID> par;
        std::unordered_set<NucleusID> groupSet(group.begin(), group.end());
        par[root] = static_cast<NucleusID>(-1);
        std::queue<NucleusID> bq;
        bq.push(root);
        while (!bq.empty()) {
            NucleusID x = bq.front(); bq.pop();
            for (NucleusID nb : adj[x])
                if (groupSet.count(nb) && !par.count(nb)) { par[nb] = x; bq.push(nb); }
        }
        return par;
    };

    // BFS order from root, reversed (leaves first = leaf-to-root order)
    auto leafToRootOrder = [&](const std::vector<NucleusID>& group, NucleusID root,
                               const std::unordered_map<NucleusID,NucleusID>& par) {
        std::vector<NucleusID> order;
        std::unordered_set<NucleusID> visited;
        std::queue<NucleusID> bq;
        bq.push(root); visited.insert(root);
        while (!bq.empty()) {
            NucleusID x = bq.front(); bq.pop();
            order.push_back(x);
            for (NucleusID nd : group)
                if (!visited.count(nd) && par.count(nd) && par.at(nd) == x) {
                    visited.insert(nd); bq.push(nd);
                }
        }
        std::reverse(order.begin(), order.end()); // leaf first
        return order;
    };

    auto tree1   = buildTree(G1, u);
    auto tree2   = buildTree(G2, v);
    auto order1  = leafToRootOrder(G1, u, tree1);
    auto order2  = leafToRootOrder(G2, v, tree2);

    auto hasMisplaced = [&]() {
        for (NucleusID n : G1) if (isG2Bound(state[n])) return true;
        for (NucleusID n : G2) if (isG1Bound(state[n])) return true;
        return false;
    };

    int maxSteps = 2 * (static_cast<int>(nodes.size()) + 2);
    int step = 0;

    while (hasMisplaced() && step < maxSteps) {
        SwapLevel lvl;
        std::unordered_set<NucleusID> usedThisStep;

        if (step % 2 == 0) {
            // Within G1: move G2-bound values (black bubbles) toward root u
            for (NucleusID child : order1) {
                if (child == u) continue;
                NucleusID par = tree1.at(child);
                if (usedThisStep.count(child) || usedThisStep.count(par)) continue;
                // Leaf-target override: skip if child already holds its target
                if (state[child] == target[child]) continue;
                // Bubble toward root: child has G2-bound, parent has G1-bound
                if (isG2Bound(state[child]) && !isG2Bound(state[par])) {
                    std::swap(state[child], state[par]);
                    lvl.push_back({child, par});
                    usedThisStep.insert(child);
                    usedThisStep.insert(par);
                }
            }
            // Within G2: move G1-bound values (white bubbles) toward root v
            for (NucleusID child : order2) {
                if (child == v) continue;
                NucleusID par = tree2.at(child);
                if (usedThisStep.count(child) || usedThisStep.count(par)) continue;
                if (state[child] == target[child]) continue;
                // Bubble toward root: child has G1-bound, parent has G2-bound
                if (isG1Bound(state[child]) && !isG1Bound(state[par])) {
                    std::swap(state[child], state[par]);
                    lvl.push_back({child, par});
                    usedThisStep.insert(child);
                    usedThisStep.insert(par);
                }
            }
        } else {
            // Channel step: swap across (u, v) only if it's an actual fast edge.
            // When the graph is disconnected (u-v not adjacent), skip this step.
            bool channelFast = false;
            for (NucleusID nb : adj[u]) if (nb == v) { channelFast = true; break; }

            if (channelFast && !usedThisStep.count(u) && !usedThisStep.count(v) &&
                isG2Bound(state[u]) && isG1Bound(state[v])) {
                std::swap(state[u], state[v]);
                lvl.push_back({u, v});
            }
        }

        if (!lvl.empty()) {
            while (static_cast<int>(levels.size()) <= levelOffset + step)
                levels.push_back({});
            for (auto& sw : lvl)
                levels[levelOffset + step].push_back(sw);
        }
        ++step;
    }

    int phaseACost = step;

    // Phase B: recurse on G1 and G2 with same level offset (interleaved = parallel)
    if (static_cast<int>(G1.size()) > 1)
        routeSubgraph(state, target, G1, adj, levels, levelOffset + phaseACost);
    if (static_cast<int>(G2.size()) > 1)
        routeSubgraph(state, target, G2, adj, levels, levelOffset + phaseACost);
}
