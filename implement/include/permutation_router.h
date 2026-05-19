#pragma once
#include <vector>
#include "physical_env.h"
#include "placement.h"
#include "swap_circuit.h"

// ============================================================
// PARTNER B OWNS THIS FILE AND src/permutation/permutation_router.cpp
// ============================================================

// Implements the fast permutation circuit construction (paper Section V-B).
//
// Problem: given a permutation of values across the physical nuclei (derived from
// two consecutive placements P_i and P_{i+1}), produce a SWAP circuit that realizes
// it using only "fast" edges, minimizing the number of logic levels (depth).
//
// Algorithm outline (divide-and-conquer, paper Section V-B):
//   1. Cut the adjacency graph G of fast interactions into two balanced connected
//      subgraphs G1 and G2 via a "communication channel" (one cut edge).
//   2. Color vertices: white if target is G1, black if target is G2.
//   3. Bring all white vertices to G1, all black to G2 using the channel.
//      This phase costs at most 2k SWAP levels (k = |G1| - 1).
//   4. Recurse independently on G1 and G2.
//   Total depth: O(n) levels, provably linear (paper eq. 2: C(n) <= 3*a*n + const).
//
// Heuristic addition: "leaf-target value override" reduces depth by 0-5%
// by directly placing a leaf vertex's target value when possible.
class PermutationRouter {
public:
    // threshold: used to build the adjacency graph of fast interactions.
    // Must match the threshold used by CircuitPlacer.
    PermutationRouter(const PhysicalEnvironment& env, Weight threshold);

    // Primary interface: construct SWAP circuit that transforms placement `from`
    // into placement `to`.
    // Calls permutationTo() internally and routes the resulting permutation.
    SwapCircuit routeBetween(const Placement& from, const Placement& to);

    // Lower-level interface: route an explicit permutation on all nuclei.
    // perm[dest] = src means "value at nucleus src should move to nucleus dest."
    // Identity entries (perm[i] == i) are no-ops.
    SwapCircuit route(const std::vector<int>& perm);

private:
    // Recursively build SWAP levels for subgraph `nodes` under current `state`.
    // state[i] = which logical value currently sits at nucleus i.
    // target[i] = which logical value should sit at nucleus i after this call.
    // levels: output, indexed from levelOffset.
    void routeSubgraph(std::vector<int>&                      state,
                       const std::vector<int>&                target,
                       const std::vector<NucleusID>&          nodes,
                       const std::vector<std::vector<NucleusID>>& adj,
                       std::vector<SwapLevel>&                levels,
                       int                                    levelOffset);

    // Partition `nodes` into two balanced connected subgraphs.
    // Returns { G1_nodes, G2_nodes, channel_edge }.
    // channel_edge is the single cut edge connecting G1 and G2.
    struct Partition {
        std::vector<NucleusID>              g1, g2;
        std::pair<NucleusID, NucleusID>     channel;
    };
    Partition partition(const std::vector<NucleusID>&          nodes,
                        const std::vector<std::vector<NucleusID>>& adj) const;

    const PhysicalEnvironment&         env_;
    Weight                             threshold_;
    std::vector<std::vector<NucleusID>> adj_;  // fast adjacency, built in constructor
};
