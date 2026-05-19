// ============================================================
// PARTNER B: Fast Permutation Circuit (Section V-B)
// ============================================================
//
// Goal: realize a permutation on n physical nuclei using non-intersecting
// SWAP gates along the "fast" adjacency graph, minimizing depth (# levels).
//
// Core algorithm (recursive divide-and-conquer, paper Section V-B):
//
//   route(nodes, state, target, adj):
//     if |nodes| <= 1: done
//
//     (G1, G2, channel) = partition(nodes, adj)
//       // partition splits nodes into two balanced connected components;
//       // channel = single cut edge (u ∈ G1, v ∈ G2) used for cross-transfers
//
//     Phase A — bring all "white" (target ∈ G1) vertices to G1,
//                all "black" (target ∈ G2) vertices to G2:
//       Color each nucleus in nodes:
//         white  if target[nucleus] is currently in G2 but should end in G1
//         black  if target[nucleus] is currently in G1 but should end in G2
//       Build a rooted spanning tree for G1 rooted at channel.u.
//       Build a rooted spanning tree for G2 rooted at channel.v.
//       Propagate "bubbles" (misplaced values) toward the channel using
//       the tree structure; alternate using the channel every other step.
//       Cost: at most 2k SWAP levels where k = max(|G1|, |G2|) - 1.
//
//     Phase B — recurse:
//       route(G1, state', target, adj|G1)
//       route(G2, state', target, adj|G2)   // independent, can interleave levels
//
// Heuristic: "leaf-target value override" (paper Section V-C):
//   At each stage, inspect leaf nodes of the spanning tree.
//   If a leaf already holds its target value, exclude it from routing
//   (remove from active set). This reduces depth by 0-5%.
//
// The resulting SwapCircuit has O(n) levels (linear in number of nuclei).
//
// Physical interpretation (paper Figure 3):
//   Think of G1 as "left container" (air) and G2 as "right container" (water).
//   White elements are air bubbles rising through the channel;
//   black elements are water droplets falling through. The channel is the pipe.

#include "permutation_router.h"
#include <algorithm>
#include <queue>
#include <cassert>
#include <numeric>
#include <unordered_map>

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

    // state[i] = which value currently sits at nucleus i
    // Initially: state[i] = i (identity — each nucleus holds its own value)
    // target[i] = perm[i] (where that value needs to come from)
    std::vector<int> state(n);
    std::iota(state.begin(), state.end(), 0);

    std::vector<int> target = perm;

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
// TODO (Partner B): implement routeSubgraph and partition
// ---------------------------------------------------------------------------

PermutationRouter::Partition PermutationRouter::partition(
        const std::vector<NucleusID>& nodes,
        const std::vector<std::vector<NucleusID>>& adj) const
{
    // TODO:
    // Find a cut edge (u, v) where u and v are in the subgraph induced by `nodes`,
    // such that removing this edge splits the subgraph into two connected components
    // G1 and G2 with |G1| ≈ |G2| (maximally balanced).
    //
    // Simple approach for bounded-degree graphs:
    //   BFS/DFS from any node; the first cross-edge in a BFS tree that
    //   creates a balanced split is the channel edge.
    //
    // For the NMR molecules in the paper (3-12 qubits), a greedy search suffices.
    //
    // The paper proves that the interaction graphs for NMR molecules have
    // separability parameter s = 1/2 (every bounded degree graph is well-separable).
    (void)nodes; (void)adj;
    Partition p;
    // placeholder: split first half / second half (NOT correct, just compiles)
    int half = static_cast<int>(nodes.size()) / 2;
    p.g1.assign(nodes.begin(), nodes.begin() + half);
    p.g2.assign(nodes.begin() + half, nodes.end());
    if (!p.g1.empty() && !p.g2.empty())
        p.channel = { p.g1.back(), p.g2.front() };
    return p;
}

void PermutationRouter::routeSubgraph(
        std::vector<int>&                      state,
        const std::vector<int>&                target,
        const std::vector<NucleusID>&          nodes,
        const std::vector<std::vector<NucleusID>>& adj,
        std::vector<SwapLevel>&                levels,
        int                                    levelOffset)
{
    if (nodes.size() <= 1) return;

    // TODO:
    // 1. Call partition(nodes, adj) to get G1, G2, channel.
    //
    // 2. Build rooted spanning tree of G1 rooted at channel.first,
    //    rooted spanning tree of G2 rooted at channel.second.
    //
    // 3. Color each nucleus in nodes:
    //    white  if the value that SHOULD be there (target[nucleus]) currently
    //           sits in G2 (i.e., state[g2_nucleus] == target[nucleus] for some g2_nucleus)
    //    black  if the value currently AT this nucleus needs to end up in G2
    //
    // 4. "Bubble" phase: propagate misplaced values toward the channel.
    //    Every even step: move bubbles within G1/G2 toward channel.
    //    Every odd step: swap across channel edge.
    //    Detect "leaf-target value override": if a leaf already holds its
    //    target value, freeze it.
    //    Record each SWAP as an entry in the appropriate SwapLevel.
    //    Grow levels vector as needed (levels[levelOffset + step]).
    //
    // 5. Recurse:
    //    routeSubgraph(state, target, G1, adj, levels, levelOffset + phaseACost)
    //    routeSubgraph(state, target, G2, adj, levels, levelOffset + phaseACost)
    //    (G1 and G2 are independent after phase A, so their levels can be interleaved)
    (void)state; (void)target; (void)nodes; (void)adj; (void)levels; (void)levelOffset;
}
