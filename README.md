# Quantum Circuit Placement — Reproduction

A C++17 reproduction of the heuristic algorithm described in:

> **Quantum Circuit Placement**  
> Maslov, Falconer & Mosca, *IEEE TCAD*, May 2008

The implementation maps logical qubits to physical nuclei (NMR device) and inserts SWAP gates between subcircuits to satisfy connectivity constraints, reproducing the runtime results in the paper's Table II and Table III.

---

## Problem Overview

Given a quantum circuit and a physical device described by a **coupling graph** (weighted interaction graph over nuclei), the goal is to find a logical→physical qubit mapping that minimises total circuit runtime, including SWAP overhead between consecutive subcircuit placements.

Core graph-theoretic concepts used:

| Concept | Role |
|---|---|
| Coupling graph | Encodes physical qubit connectivity (nucleus interaction weights W) |
| Subgraph monomorphism (VF2-style) | Finds valid placements: logical interaction graph ↪ physical fast graph |
| Hill-climbing fine-tuning | Locally improves placement; uses depth-2 look-ahead as tiebreaker |
| Divide-and-conquer SWAP routing | Generates permutation circuits between adjacent placements |

### W-value formula

```
Two-qubit:    W(u,v) = round(10000 / (4 × J_Hz))
Single-qubit: W(u,u) = round(π × 10000 / |Δν_Hz|)
```

---

## Building

**MinGW g++ (Windows):**

```powershell
cd implement
g++ -std=c++17 -I include `
    src/physical_env.cpp src/quantum_circuit.cpp `
    src/placement.cpp src/swap_circuit.cpp `
    src/algorithm/circuit_placer.cpp `
    src/permutation/permutation_router.cpp `
    src/main.cpp -o placer.exe
.\placer.exe
```

**CMake:**

```bash
cmake -B implement/build -S implement
cmake --build implement/build
./implement/build/placer
```

---

## Verification Results

Running the binary executes three checks against known paper values.

### Example 3 (Paper Section III)

Error-correction encoding circuit on acetyl chloride (3 qubits, 3 nuclei):

| Placement | Computed | Paper |
|---|---|---|
| Optimal: a→C2, b→C1, c→M | **136** | 136 ✅ |
| Suboptimal: a→M, b→C2, c→C1 | **770** | 770 ✅ |

### Table II

| Circuit | Molecule | Computed | Paper | Status |
|---|---|---|---|---|
| Error-corr. encoding (3q) | Acetyl chloride | **0.0136 s** | 0.0136 s | ✅ exact |
| Phase estimation (5q) | BOC-glycine-fluoride | ~0.054 s | 0.0699 s | approx |
| Pseudo cat state (5q) | Trans-crotonic acid | ~0.052 s | varies | approx |

> Rows 2–3 diverge due to estimated J-coupling values and reconstructed circuit gate sequences; no literature source provides the exact gate lists used in the paper.

### Table III

Sweeps `Threshold` parameter (50 → 10000) for phase estimation on two molecules, reproducing the subcircuit count / runtime trade-off curve shown in the paper.

---

## Repository Structure

```
implement/
├── include/                        # Shared headers
│   ├── gate.h                      # Gate type, GateType enum
│   ├── physical_env.h              # PhysicalEnvironment: coupling graph + W queries
│   ├── quantum_circuit.h           # QuantumCircuit: DP runtime computation
│   ├── placement.h                 # Placement: logical→physical mapping
│   ├── swap_circuit.h              # SwapCircuit: SWAP layer representation
│   ├── circuit_placer.h            # CircuitPlacer interface (Partner A)
│   └── permutation_router.h        # PermutationRouter interface (Partner B)
├── src/
│   ├── physical_env.cpp
│   ├── quantum_circuit.cpp
│   ├── placement.cpp
│   ├── swap_circuit.cpp
│   ├── algorithm/
│   │   └── circuit_placer.cpp      # VF2 monomorphism + hill-climbing + depth-2 lookahead
│   ├── permutation/
│   │   └── permutation_router.cpp  # Divide-and-conquer SWAP routing
│   └── main.cpp                    # verifyExample3 / runTableII / runTableIII
├── data/
│   ├── environments/
│   │   ├── acetyl_chloride.env        # 3q — verified exact (paper Table I)
│   │   ├── boc_glycine_fluoride.env   # 5q — estimated J-couplings
│   │   ├── trans_crotonic_acid.env    # 7q — estimated J-couplings
│   │   └── histidine.env             # 12q — estimated J-couplings
│   └── circuits/
│       ├── error_corr_encoding.circ   # 3q, 9 gates  (paper Fig. 1)
│       ├── phaseest.circ             # 5q, phase estimation
│       ├── five_bit_error_corr.circ  # 5q, [[5,1,3]] code
│       └── pseudo_cat_state.circ     # 5q, cat state preparation
├── figures/
│   ├── generate_figures.py           # 6 heuristic-process figures (matplotlib + networkx)
│   ├── generate_flowcharts.py        # 4 algorithm flowcharts (report-ready)
│   └── 5qubit_analysis.md            # 5-qubit complexity analysis notes
└── IMPLEMENTATION_GUIDE.md           # Detailed algorithm documentation
```

### Generating figures

```powershell
cd implement
python figures/generate_figures.py      # heuristic-process figures
python figures/generate_flowcharts.py   # algorithm flowcharts
```

Requires: Python 3.10+, matplotlib, networkx.

---

## Algorithm Summary

### Stage 1 — basicPlacement

Greedily grows a workspace of two-qubit gates starting from `startGate`. After each new logical qubit interaction pair, checks whether a **subgraph monomorphism** from the interaction graph into the physical fast graph (W ≤ threshold) exists. Stops at the first gate that cannot be embedded; returns the gate index and the placement with minimum subcircuit runtime.

### Stage 2 — fineTuning

Hill-climbing over the placement: repeatedly reassigns each logical qubit to every physical nucleus and accepts moves that reduce `scoreplacement()`. The score adds a small **depth-2 look-ahead penalty** (0.05 × W × T for the next two two-qubit gates) to prefer placements that need fewer SWAPs at subcircuit boundaries.

### Stage 3 — routeSubgraph

Divide-and-conquer SWAP routing between two consecutive placements. Partitions the physical graph into two halves via BFS spanning tree, performs **bubble propagation** (Phase A) to move qubit values across the partition boundary, then recurses on each half independently (Phase B). Includes a **leaf-target override** to skip unnecessary SWAPs when a nucleus already holds its target value.

---

## References

Reference numbers below correspond to the bibliography of Maslov, Falconer & Mosca (2008).

**Main paper**

D. Maslov, S. M. Falconer, and M. Mosca, "Quantum circuit placement," *IEEE Trans. Comput.-Aided Des. Integr. Circuits Syst.*, vol. 27, no. 4, pp. 752–763, Apr. 2008.

**NMR molecules and circuits used in this implementation**

| # | Citation | Used for |
|---|---|---|
| [12] | E. Knill, R. Laflamme, R. Martinez, and C. Negrevergne, "Benchmarking quantum computers: The five-qubit error correcting code," *Phys. Rev. Lett.*, vol. 86, pp. 5811–5814, 2001. | Trans-crotonic acid environment; five-bit error-correction circuit |
| [14] | E. Knill, R. Laflamme, R. Martinez, and C. Tseng, "An algorithmic benchmark for quantum information processing," *Nature*, vol. 404, pp. 368–370, 2000. | Acetyl chloride (3-qubit) environment; error-correction encoding circuit |
| [16] | R. Marx, A. F. Fahmy, J. M. Myers, W. Bermel, and S. J. Glaser, "Approaching five-bit NMR quantum computing," *Phys. Rev. A*, vol. 60, pp. 2966–2981, 1999. | BOC-glycine-fluoride (5-qubit) environment |
| [20] | C. Negrevergne, T. S. Mahesh, C. A. Ryan, M. Ditty, F. Cyr-Racine, W. Power, N. Boulant, T. Havel, D. G. Cory, and R. Laflamme, "Benchmarking quantum control methods on a 12-qubit system," *Phys. Rev. Lett.*, vol. 96, p. 170501, 2006. | Histidine (12-qubit) environment; pseudo-cat state circuit |
| [21] | M. A. Nielsen and I. L. Chuang, *Quantum Computation and Quantum Information*, Cambridge: Cambridge University Press, 2000. | Phase estimation circuit construction (p. 221) |
