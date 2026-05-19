#pragma once
#include <vector>
#include <string>
#include "gate.h"
#include "physical_env.h"

class Placement;  // forward declaration

// Represents a leveled quantum circuit on n logical qubits.
// Gates at the same level can execute in parallel (NMR model).
class QuantumCircuit {
public:
    explicit QuantumCircuit(int numQubits);

    void addGate(const Gate& g);

    int numQubits()  const;
    int numGates()   const;
    int numLevels()  const;

    const std::vector<Gate>& allGates()              const;
    std::vector<Gate>        gatesAtLevel(int level) const;

    // Returns subcircuit containing gates with indices in [beginIdx, endIdx).
    // Preserves gate order; level indices are re-based from 0.
    QuantumCircuit subcircuit(int beginIdx, int endIdx) const;

    // Circuit runtime under a given placement, computed with the DP algorithm
    // (paper Section III): sums GateOperatingTime across all levels,
    // respecting two-qubit data dependencies. Returns max qubit finish time.
    double computeRuntime(const Placement& p, const PhysicalEnvironment& env) const;

    // Load from plain-text file (see data/circuits/ for format)
    static QuantumCircuit fromFile(const std::string& path);

private:
    int              nQubits_;
    int              nLevels_;
    std::vector<Gate> gates_;
};
