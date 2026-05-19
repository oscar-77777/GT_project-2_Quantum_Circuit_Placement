#pragma once
#include <vector>
#include <string>
#include <utility>
#include "types.h"

// Represents the physical molecule/device as a complete weighted graph.
//
// W(u, v) for u != v: time to apply a fixed-angle two-qubit gate on nuclei u and v.
//   Proportional to 1/|coupling_frequency(u,v)|.
//
// W(u, u): time to apply a fixed-angle single-qubit gate on nucleus u.
//   Proportional to 1/|min_chemical_shift_diff(u, others)|.
//
// Units: 1/10000 s, rounded to integer (per Maslov et al. 2008 paper conventions).
class PhysicalEnvironment {
public:
    explicit PhysicalEnvironment(int numNuclei);

    // Set W(u,v) = W(v,u) = w  (symmetric)
    void setTwoQubitWeight(NucleusID u, NucleusID v, Weight w);

    // Set W(u,u) = w  (single-qubit cost on nucleus u)
    void setSingleQubitWeight(NucleusID u, Weight w);

    int    numNuclei() const;

    // W(u,v): two-qubit gate cost between u and v  (u != v)
    Weight twoQubitWeight(NucleusID u, NucleusID v) const;

    // W(u,u): single-qubit gate cost on nucleus u
    Weight singleQubitWeight(NucleusID u) const;

    // GateOperatingTime = W(P(q1), P(q2)) * T(G)  (paper Section III Definition 3)
    // For single-qubit: W(P(q1), P(q1)) * T(G)
    Weight gateOperatingTime(Weight baseTime, NucleusID n1, NucleusID n2 = UNASSIGNED) const;

    // All edges (u,v) with u<v where twoQubitWeight(u,v) <= threshold  ("fast" interactions)
    std::vector<std::pair<NucleusID, NucleusID>> fastEdges(Weight threshold) const;

    // Adjacency list over fast interactions, indexed by NucleusID.
    // adj[u] = list of nuclei v such that twoQubitWeight(u,v) <= threshold
    std::vector<std::vector<NucleusID>> fastAdjacency(Weight threshold) const;

    // Load from plain-text file (see data/environments/acetyl_chloride.env for format)
    static PhysicalEnvironment fromFile(const std::string& path);

private:
    int nNuclei_;
    std::vector<std::vector<Weight>> W_;   // W_[u][v], W_[u][u] = single-qubit cost
};
