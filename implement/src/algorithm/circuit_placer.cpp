// ============================================================
// Subcircuit Placement Algorithm (Section V-A)
// ============================================================

#include "circuit_placer.h"
#include "swap_circuit.h"
#include <algorithm>
#include <numeric>
#include <cassert>
#include <limits>
#include <set>

// ---------------------------------------------------------------------------
// Internal: backtracking subgraph monomorphism (VF2-style)
// ---------------------------------------------------------------------------
// Finds injective maps f: patternNodes -> targetNodes preserving adjacency.
// patternAdj[u] = neighbors of u in the "needed" graph (logical qubit pairs).
// targetAdj[u]  = neighbors of u in the "fast" graph (physical nuclei).
// Returns up to maxResults mappings; result[k][i] = f(i).

namespace {

struct MonoState {
    int                              patternSize;
    const std::vector<std::vector<int>>& patternAdj;
    const std::vector<std::vector<int>>& targetAdj;
    std::vector<int>                 mapping;
    std::vector<bool>                used;
    std::vector<std::vector<int>>    results;
    int                              maxResults;

    void backtrack(int node) {
        if (static_cast<int>(results.size()) >= maxResults) return;
        if (node == patternSize) {
            results.push_back(mapping);
            return;
        }
        int targetSize = static_cast<int>(targetAdj.size());
        for (int t = 0; t < targetSize; ++t) {
            if (used[t]) continue;
            bool ok = true;
            for (int prev = 0; prev < node && ok; ++prev) {
                bool neighborInPattern = false;
                for (int nb : patternAdj[node])
                    if (nb == prev) { neighborInPattern = true; break; }
                if (!neighborInPattern) continue;
                int tp = mapping[prev];
                bool neighborInTarget = false;
                for (int nb : targetAdj[t])
                    if (nb == tp) { neighborInTarget = true; break; }
                if (!neighborInTarget) ok = false;
            }
            if (!ok) continue;
            mapping[node] = t;
            used[t] = true;
            backtrack(node + 1);
            used[t] = false;
            mapping[node] = -1;
        }
    }
};

std::vector<std::vector<int>> findMonomorphisms(
        const std::vector<std::vector<int>>& patternAdj,
        const std::vector<std::vector<int>>& targetAdj,
        int maxResults = 100)
{
    int pSize = static_cast<int>(patternAdj.size());
    int tSize = static_cast<int>(targetAdj.size());
    MonoState state{ pSize, patternAdj, targetAdj,
                     std::vector<int>(pSize, -1),
                     std::vector<bool>(tSize, false),
                     {}, maxResults };
    state.backtrack(0);
    return state.results;
}

} // namespace

// ---------------------------------------------------------------------------
// CircuitPlacer
// ---------------------------------------------------------------------------

CircuitPlacer::CircuitPlacer(const PhysicalEnvironment& env, Weight threshold)
    : env_(env), threshold_(threshold)
{}

double CircuitPlacer::subcircuitRuntime(const QuantumCircuit& sub, const Placement& p) const {
    return sub.computeRuntime(p, env_);
}

double CircuitPlacer::totalRuntime(const PlacementResult& result,
                                   const std::vector<SwapCircuit>& swaps,
                                   Weight swapLevelCost) const {
    double total = 0.0;
    for (int i = 0; i < static_cast<int>(result.subcircuits.size()); ++i)
        total += subcircuitRuntime(result.subcircuits[i], result.placements[i]);
    for (const SwapCircuit& sc : swaps)
        total += sc.depth() * swapLevelCost;
    return total;
}

// ---------------------------------------------------------------------------
// Stage 1: Basic Placement
// ---------------------------------------------------------------------------
// Greedily extend workspace C from startGate, adding two-qubit gates one at a
// time. After each new logical-qubit interaction pair, check whether a subgraph
// monomorphism from the interaction graph into the "fast" physical graph exists.
// Stop when no monomorphism is found; return endGate (exclusive).
// Pick the monomorphism that minimises actual subcircuit runtime.

int CircuitPlacer::basicPlacement(const QuantumCircuit& circuit, int startGate, Placement& placement) {
    int nQ = circuit.numQubits();
    int nN = env_.numNuclei();
    auto fastAdj = env_.fastAdjacency(threshold_);

    std::vector<std::vector<int>> patternAdj(nQ);
    std::set<std::pair<int,int>>  patternEdgeSet;
    std::vector<std::vector<int>> bestMonos;
    int endGate = circuit.numGates(); // default: all gates fit

    for (int g = startGate; g < circuit.numGates(); ++g) {
        const Gate& gate = circuit.allGates()[g];
        if (gate.type != GateType::Two) continue;

        int q1 = gate.q1, q2 = gate.q2;
        auto edge = std::make_pair(std::min(q1, q2), std::max(q1, q2));
        if (patternEdgeSet.count(edge)) continue; // same pair seen before, no new constraint

        // Tentatively add edge to pattern
        patternAdj[q1].push_back(q2);
        patternAdj[q2].push_back(q1);
        patternEdgeSet.insert(edge);

        auto monos = findMonomorphisms(patternAdj, fastAdj, 100);
        if (monos.empty()) {
            // Undo and stop: gate g cannot be embedded → workspace ends at g
            patternAdj[q1].pop_back();
            patternAdj[q2].pop_back();
            patternEdgeSet.erase(edge);
            endGate = g;
            break;
        }
        bestMonos = monos;
    }

    // If no two-qubit gates were successfully embedded, fall back to identity
    if (bestMonos.empty()) {
        for (int q = 0; q < nQ && q < nN; ++q)
            placement.assign(q, q);
        return endGate;
    }

    // Choose monomorphism with minimum subcircuit runtime
    QuantumCircuit sub = circuit.subcircuit(startGate, endGate);
    double bestRuntime = std::numeric_limits<double>::max();

    for (auto& mono : bestMonos) {
        Placement candidate(nQ, nN);
        for (int q = 0; q < nQ; ++q)
            candidate.assign(q, mono[q]);
        double rt = sub.computeRuntime(candidate, env_);
        if (rt < bestRuntime) {
            bestRuntime = rt;
            placement   = candidate;
        }
    }
    return endGate;
}

// ---------------------------------------------------------------------------
// Depth-2 look-ahead scoring
// ---------------------------------------------------------------------------
// Returns runtime(sub, p) + cost of the next 2 two-qubit gates in fullCircuit
// under placement p.  The extra term steers hill-climbing toward placements that
// also work well for the immediately following gates, reducing SWAP overhead
// at subcircuit boundaries.
// If fullCircuit is null (last subcircuit), falls back to plain runtime.

double CircuitPlacer::scoreplacement(const QuantumCircuit& sub, const Placement& p,
                                     const QuantumCircuit* fullCircuit, int nextStart) const {
    double score = sub.computeRuntime(p, env_);

    if (!fullCircuit) return score;

    int lookaheadCount = 0;
    int total = fullCircuit->numGates();
    for (int g = nextStart; g < total && lookaheadCount < 2; ++g) {
        const Gate& gate = fullCircuit->allGates()[g];
        if (gate.type != GateType::Two) continue;
        NucleusID n1 = p.get(gate.q1);
        NucleusID n2 = p.get(gate.q2);
        if (n1 < 0 || n2 < 0 || n1 >= env_.numNuclei() || n2 >= env_.numNuclei()) continue;
        // Small penalty for slow next-gate interactions (tiebreaker only, ~0-5% effect).
        // Scaled to 0.05 so the current subcircuit runtime stays the dominant objective.
        Weight w = env_.twoQubitWeight(n1, n2);
        if (w > threshold_) score += 0.05 * w * gate.time;
        ++lookaheadCount;
    }
    return score;
}

// ---------------------------------------------------------------------------
// Stage 2: Fine Tuning  (with depth-2 look-ahead)
// ---------------------------------------------------------------------------
// Hill-climbing: repeatedly try reassigning each logical qubit to every
// physical nucleus; accept if the score (runtime + look-ahead penalty) decreases.
// Terminates when no single-qubit reassignment improves the solution.

void CircuitPlacer::fineTuning(const QuantumCircuit& sub, Placement& placement,
                                const QuantumCircuit* fullCircuit, int nextStart) {
    int nQ = sub.numQubits();
    int nN = env_.numNuclei();

    bool improved = true;
    while (improved) {
        improved = false;
        double curScore = scoreplacement(sub, placement, fullCircuit, nextStart);

        for (int qi = 0; qi < nQ; ++qi) {
            NucleusID orig = placement.get(qi);
            for (NucleusID nu = 0; nu < nN; ++nu) {
                if (nu == orig) continue;
                // Ensure nu is not already used by another qubit
                bool inUse = false;
                for (int qj = 0; qj < nQ && !inUse; ++qj)
                    if (qj != qi && placement.get(qj) == nu) inUse = true;
                if (inUse) continue;

                placement.assign(qi, nu);
                double sc = scoreplacement(sub, placement, fullCircuit, nextStart);
                if (sc < curScore) {
                    curScore = sc;
                    orig     = nu;
                    improved = true;
                } else {
                    placement.assign(qi, orig); // revert
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Main placement loop (framework — do not modify)
// ---------------------------------------------------------------------------

PlacementResult CircuitPlacer::place(const QuantumCircuit& circuit) {
    PlacementResult result;

    int startGate = 0;
    int total = circuit.numGates();

    while (startGate < total) {
        Placement p(circuit.numQubits(), env_.numNuclei());

        int endGate = basicPlacement(circuit, startGate, p);
        if (endGate == startGate) endGate = startGate + 1; // always advance

        QuantumCircuit sub = circuit.subcircuit(startGate, endGate);

        // Depth-2 look-ahead: pass full circuit + start of next gates so
        // fineTuning can penalise placements that are bad for the next subcircuit.
        // Nullptr on the last subcircuit (no next gates to look ahead into).
        bool isLast = (endGate >= total);
        fineTuning(sub, p, isLast ? nullptr : &circuit, endGate);

        result.subcircuits.push_back(sub);
        result.placements.push_back(p);
        startGate = endGate;
    }

    return result;
}
