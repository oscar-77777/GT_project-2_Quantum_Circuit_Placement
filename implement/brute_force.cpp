// Brute-force placement enumeration for Table II proof of correctness.
//
// For each Table II row, this program tries every possible injective mapping
// (logical qubits -> physical nuclei) and records the runtime with
// computeRuntime() (fixed single placement, no subcircuit splitting, no SWAP).
// Row 3 uses random sampling (P(12,10) = 239.5M is too large to enumerate fully).
//
// Output: brute_force_data/row{1,2,3}.csv  and  brute_force_data/summary.csv
//
// Compile (from implement/ directory):
//   g++ -std=c++17 -I include src/physical_env.cpp src/quantum_circuit.cpp ^
//       src/placement.cpp src/swap_circuit.cpp ^
//       src/algorithm/circuit_placer.cpp src/permutation/permutation_router.cpp ^
//       brute_force.cpp -o brute_force.exe
// Run:
//   .\brute_force.exe

#include "physical_env.h"
#include "quantum_circuit.h"
#include "placement.h"
#include "circuit_placer.h"
#include "permutation_router.h"
#include "gate.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <numeric>
#include <functional>
#include <random>
#include <limits>
#include <algorithm>
#include <iomanip>
#include <direct.h>    // _mkdir (Windows / MinGW)

// ---------------------------------------------------------------------------
// Replicate acetyl chloride (same as main.cpp — needed for row 1)
// ---------------------------------------------------------------------------

static PhysicalEnvironment buildAcetylChloride() {
    PhysicalEnvironment env(3);
    env.setSingleQubitWeight(0, 8.0);
    env.setSingleQubitWeight(1, 8.0);
    env.setSingleQubitWeight(2, 1.0);
    env.setTwoQubitWeight(0, 1,  38.0);
    env.setTwoQubitWeight(0, 2, 672.0);
    env.setTwoQubitWeight(1, 2,  89.0);
    return env;
}

static QuantumCircuit buildErrorCorrEncoding() {
    QuantumCircuit circ(3);
    circ.addGate(makeSingleGate(0, 1.0, 0));
    circ.addGate(makeTwoGate   (0, 1, 1.0, 1));
    circ.addGate(makeSingleGate(2, 1.0, 2));
    circ.addGate(makeTwoGate   (1, 2, 1.0, 3));
    circ.addGate(makeSingleGate(1, 1.0, 4));
    circ.addGate(makeSingleGate(0, 0.0, 5));
    circ.addGate(makeSingleGate(1, 0.0, 5));
    circ.addGate(makeSingleGate(2, 0.0, 5));
    circ.addGate(makeSingleGate(0, 0.0, 6));
    return circ;
}

// ---------------------------------------------------------------------------
// Algorithm result (identical to runPlacement in main.cpp)
// ---------------------------------------------------------------------------

static double algoResult(const PhysicalEnvironment& env, const QuantumCircuit& circ,
                          Weight threshold = 200.0) {
    Weight swapCost = std::numeric_limits<Weight>::max();
    for (int u = 0; u < env.numNuclei(); ++u)
        for (int v = u + 1; v < env.numNuclei(); ++v) {
            Weight w = env.twoQubitWeight(u, v);
            if (w > 0 && w < swapCost) swapCost = w;
        }
    if (swapCost == std::numeric_limits<Weight>::max()) swapCost = 1.0;

    CircuitPlacer   placer(env, threshold);
    PlacementResult result = placer.place(circ);

    PermutationRouter router(env, threshold);
    std::vector<SwapCircuit> swaps;
    for (int i = 0; i + 1 < static_cast<int>(result.placements.size()); ++i)
        swaps.push_back(router.routeBetween(result.placements[i], result.placements[i + 1]));

    return placer.totalRuntime(result, swaps, swapCost);
}

// ---------------------------------------------------------------------------
// Full enumeration of P(nPhys, nLog) placements
// Returns the minimum runtime found across all placements.
// ---------------------------------------------------------------------------

static double enumerateAll(const PhysicalEnvironment& env, const QuantumCircuit& circ,
                            int nLog, int nPhys, const std::string& outPath) {
    std::ofstream f(outPath);
    f << "runtime\n";

    double minRT = std::numeric_limits<double>::max();
    std::vector<int> perm(nLog, -1);
    std::vector<bool> used(nPhys, false);

    std::function<void(int)> enumerate = [&](int pos) {
        if (pos == nLog) {
            Placement p(nLog, nPhys);
            for (int i = 0; i < nLog; ++i) p.assign(i, perm[i]);
            double rt = circ.computeRuntime(p, env);
            f << rt << "\n";
            if (rt < minRT && rt > 0.0) minRT = rt;
            return;
        }
        for (int i = 0; i < nPhys; ++i) {
            if (!used[i]) {
                used[i] = true;
                perm[pos] = i;
                enumerate(pos + 1);
                used[i] = false;
                perm[pos] = -1;
            }
        }
    };

    enumerate(0);
    return minRT;
}

// ---------------------------------------------------------------------------
// Random sampling of placements (for row 3 where full enumeration is infeasible)
// Returns the minimum runtime found among sampled placements.
// ---------------------------------------------------------------------------

static double sampleRandom(const PhysicalEnvironment& env, const QuantumCircuit& circ,
                             int nLog, int nPhys, int nSamples, const std::string& outPath) {
    std::ofstream f(outPath);
    f << "runtime\n";

    std::mt19937 rng(42);
    std::vector<int> base(nPhys);
    std::iota(base.begin(), base.end(), 0);

    double minRT = std::numeric_limits<double>::max();
    for (int s = 0; s < nSamples; ++s) {
        std::shuffle(base.begin(), base.end(), rng);
        Placement p(nLog, nPhys);
        for (int i = 0; i < nLog; ++i) p.assign(i, base[i]);
        double rt = circ.computeRuntime(p, env);
        f << rt << "\n";
        if (rt < minRT && rt > 0.0) minRT = rt;
    }
    return minRT;
}

// ---------------------------------------------------------------------------
// Compute P(nPhys, nLog) = nPhys * (nPhys-1) * ... * (nPhys-nLog+1)
// ---------------------------------------------------------------------------

static long long permCount(int nPhys, int nLog) {
    long long r = 1;
    for (int k = nPhys; k > nPhys - nLog; --k) r *= k;
    return r;
}

// ---------------------------------------------------------------------------

int main() {
    _mkdir("brute_force_data");

    std::cout << "=== Brute-force placement search for Table II ===\n\n";
    std::ofstream summary("brute_force_data/summary.csv");
    summary << "row,circuit,environment,nLog,nPhys,searchSpace,"
               "algoResult,bruteMin,isSampled,nSamples\n";

    // -----------------------------------------------------------------------
    // Row 1: error-correction encoding (3q) -> acetyl chloride (3q)
    // P(3,3) = 6  -- full enumeration
    // -----------------------------------------------------------------------
    {
        std::cout << "Row 1: error-corr encoding -> acetyl chloride\n"
                  << "  Enumerating all P(3,3) = 6 placements...\n";
        PhysicalEnvironment env  = buildAcetylChloride();
        QuantumCircuit      circ = buildErrorCorrEncoding();

        double algo  = algoResult(env, circ);
        double bfMin = enumerateAll(env, circ, 3, 3, "brute_force_data/row1.csv");

        std::cout << std::fixed << std::setprecision(4)
                  << "  Algo result : " << algo  << " units\n"
                  << "  BF minimum  : " << bfMin << " units\n\n";

        summary << "1,error_corr_encoding,acetyl_chloride,3,3,6,"
                << algo << "," << bfMin << ",0,6\n";
    }

    // -----------------------------------------------------------------------
    // Row 2: 5-bit error correction (5q) -> trans-crotonic acid (7q)
    // P(7,5) = 2520  -- full enumeration
    // -----------------------------------------------------------------------
    {
        std::cout << "Row 2: 5-bit error corr -> trans-crotonic acid\n"
                  << "  Enumerating all P(7,5) = 2520 placements...\n";
        PhysicalEnvironment env  = PhysicalEnvironment::fromFile("data/environments/trans_crotonic_acid.env");
        QuantumCircuit      circ = QuantumCircuit::fromFile("data/circuits/five_bit_error_corr.circ");

        double algo  = algoResult(env, circ);
        double bfMin = enumerateAll(env, circ, 5, 7, "brute_force_data/row2.csv");

        std::cout << std::fixed << std::setprecision(4)
                  << "  Algo result : " << algo  << " units\n"
                  << "  BF minimum  : " << bfMin << " units\n\n";

        summary << "2,five_bit_error_corr,trans_crotonic_acid,5,7,2520,"
                << algo << "," << bfMin << ",0,2520\n";
    }

    // -----------------------------------------------------------------------
    // Row 3: pseudo-cat state (10q) -> histidine (12q)
    // P(12,10) = 239,500,800  -- random sampling (100,000 samples)
    // -----------------------------------------------------------------------
    {
        const int nSamples = 100000;
        long long ss = permCount(12, 10);
        std::cout << "Row 3: pseudo-cat state -> histidine\n"
                  << "  Search space = P(12,10) = " << ss << "\n"
                  << "  Sampling " << nSamples << " random placements...\n";

        PhysicalEnvironment env  = PhysicalEnvironment::fromFile("data/environments/histidine.env");
        QuantumCircuit      circ = QuantumCircuit::fromFile("data/circuits/pseudo_cat_state.circ");

        double algo   = algoResult(env, circ);
        double smpMin = sampleRandom(env, circ, 10, 12, nSamples,
                                      "brute_force_data/row3.csv");

        std::cout << std::fixed << std::setprecision(4)
                  << "  Algo result  : " << algo   << " units\n"
                  << "  Sample min   : " << smpMin << " units\n\n";

        summary << "3,pseudo_cat_state,histidine,10,12," << ss << ","
                << algo << "," << smpMin << ",1," << nSamples << "\n";
    }

    std::cout << "Output written to brute_force_data/\n"
              << "Next: python plot_search_space.py\n";
    return 0;
}
