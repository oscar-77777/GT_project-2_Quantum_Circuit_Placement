#include "quantum_circuit.h"
#include "placement.h"
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>

QuantumCircuit::QuantumCircuit(int numQubits)
    : nQubits_(numQubits), nLevels_(0)
{}

void QuantumCircuit::addGate(const Gate& g) {
    gates_.push_back(g);
    nLevels_ = std::max(nLevels_, g.level + 1);
}

int QuantumCircuit::numQubits()  const { return nQubits_; }
int QuantumCircuit::numGates()   const { return static_cast<int>(gates_.size()); }
int QuantumCircuit::numLevels()  const { return nLevels_; }

const std::vector<Gate>& QuantumCircuit::allGates() const { return gates_; }

std::vector<Gate> QuantumCircuit::gatesAtLevel(int level) const {
    std::vector<Gate> result;
    for (const Gate& g : gates_)
        if (g.level == level) result.push_back(g);
    return result;
}

QuantumCircuit QuantumCircuit::subcircuit(int beginIdx, int endIdx) const {
    QuantumCircuit sub(nQubits_);
    int baseLevel = (beginIdx < static_cast<int>(gates_.size())) ? gates_[beginIdx].level : 0;
    for (int i = beginIdx; i < endIdx && i < static_cast<int>(gates_.size()); ++i) {
        Gate g = gates_[i];
        g.level -= baseLevel;   // re-base levels from 0
        sub.addGate(g);
    }
    return sub;
}

// Dynamic programming runtime calculation (paper Section III):
//
//   Create time[0..n-1] = 0
//   For each gate G in order:
//     if two-qubit on (t, c):
//       time[c] = max(time[c], time[t]) + W(P(t), P(c)) * T(G)
//       time[t] = time[c]
//     if single-qubit on t:
//       time[t] += W(P(t), P(t)) * T(G)
//   return max(time)
double QuantumCircuit::computeRuntime(const Placement& p, const PhysicalEnvironment& env) const {
    std::vector<double> time(nQubits_, 0.0);

    for (const Gate& g : gates_) {
        if (g.type == GateType::Two) {
            NucleusID nt = p.get(g.q1);
            NucleusID nc = p.get(g.q2);
            if (nt < 0 || nc < 0 || nt >= env.numNuclei() || nc >= env.numNuclei())
                return 0.0; // guard: unassigned qubit, skip
            double cost = env.twoQubitWeight(nt, nc) * g.time;
            double t = std::max(time[g.q1], time[g.q2]) + cost;
            time[g.q1] = time[g.q2] = t;
        } else {
            NucleusID n = p.get(g.q1);
            if (n < 0 || n >= env.numNuclei())
                continue; // guard: unassigned qubit
            time[g.q1] += env.singleQubitWeight(n) * g.time;
        }
    }
    if (time.empty()) return 0.0;
    return *std::max_element(time.begin(), time.end());
}

// File format:
//   Line 1: n   (number of qubits)
//   Subsequent lines: type q1 [q2] time level
//     type = 1 (single-qubit) or 2 (two-qubit)
//   Lines starting with '#' are comments.
QuantumCircuit QuantumCircuit::fromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open circuit file: " + path);

    // Skip leading comment/blank lines, then read qubit count
    std::string line;
    int n = 0;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        if (ss >> n) break;
    }
    QuantumCircuit circ(n);

    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        int type; ss >> type;
        if (type == 1) {
            int q, level; double t;
            ss >> q >> t >> level;
            circ.addGate(makeSingleGate(q, t, level));
        } else if (type == 2) {
            int q1, q2, level; double t;
            ss >> q1 >> q2 >> t >> level;
            circ.addGate(makeTwoGate(q1, q2, t, level));
        }
    }
    return circ;
}
