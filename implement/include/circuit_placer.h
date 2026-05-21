#pragma once
#include <vector>
#include "quantum_circuit.h"
#include "physical_env.h"
#include "placement.h"

// ============================================================
// PARTNER A OWNS THIS FILE AND src/algorithm/circuit_placer.cpp
// ============================================================

// Result of the placement algorithm (paper Section V-A).
// Decomposes the input circuit into t subcircuits, each with an optimal placement.
struct PlacementResult {
    std::vector<QuantumCircuit> subcircuits;  // C1, C2, ..., Ct
    std::vector<Placement>      placements;   // P1, P2, ..., Pt  (same length)
    // Invariant: subcircuits.size() == placements.size()
};

// Implements the heuristic subcircuit placement algorithm (Section V-A).
//
// High-level outline:
//   1. Preprocessing: identify "fast" interactions using Threshold parameter.
//   2. Basic placement: greedily grow a workspace C of two-qubit gates that can
//      be aligned along fast interactions via subgraph monomorphism.
//   3. Fine tuning: hill-climb over the monomorphism to minimize actual runtime
//      (including single-qubit gate costs and depth-2 lookahead).
//   4. Repeat from the gate that blocked the previous workspace until circuit done.
class CircuitPlacer {
public:
    // threshold: max W(u,v) still considered a "fast" interaction.
    // Gates mapped to edges above threshold will not be aligned in basic placement.
    CircuitPlacer(const PhysicalEnvironment& env, Weight threshold);

    // Main entry point.
    // Decomposes `circuit` into subcircuits and finds a placement for each.
    PlacementResult place(const QuantumCircuit& circuit);

    // Estimated runtime of one subcircuit under its placement (no SWAPs).
    // Delegates to QuantumCircuit::computeRuntime().
    double subcircuitRuntime(const QuantumCircuit& sub, const Placement& p) const;

    // Total estimated runtime: sum of all subcircuit runtimes + SWAP circuit depths
    // (each SWAP level is assumed to cost `swapLevelCost` time units).
    double totalRuntime(const PlacementResult& result,
                        const std::vector<class SwapCircuit>& swaps,
                        Weight swapLevelCost) const;

private:
    // --- Basic placement stage ---
    // Returns the largest prefix of `circuit` (as gate index count) such that
    // the involved two-qubit interactions can all be aligned along fast edges.
    // Writes the resulting monomorphism into `placement`.
    int basicPlacement(const QuantumCircuit& circuit, int startGate, Placement& placement);

    // --- Fine tuning stage ---
    // Hill-climbs the placement to reduce actual runtime.
    // fullCircuit/nextStart enable depth-2 look-ahead: the next 2 two-qubit gates
    // after the current subcircuit are penalised by their cost under the candidate
    // placement, steering the hill-climb toward transitions that need fewer SWAPs.
    void fineTuning(const QuantumCircuit& sub, Placement& placement,
                    const QuantumCircuit* fullCircuit = nullptr, int nextStart = 0);

    // Score = runtime(sub, p) + depth-2 look-ahead penalty (next 2 two-qubit gates).
    double scoreplacement(const QuantumCircuit& sub, const Placement& p,
                          const QuantumCircuit* fullCircuit, int nextStart) const;

    const PhysicalEnvironment& env_;
    Weight threshold_;
};
