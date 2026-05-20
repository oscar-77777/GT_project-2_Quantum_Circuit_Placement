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
| Coupling graph | Encodes physical qubit connectivity (nucleus interaction weights) |
| Subgraph monomorphism | Finds valid initial placements (logical graph ↪ physical graph) |
| Hill-climbing fine-tuning | Locally improves placement after monomorphism search |
| Divide-and-conquer SWAP routing | Generates permutation circuits between adjacent placements |

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

## Verification Targets

Running the binary executes three checks against known paper values:

### Example 3 (Paper Section III)

Error-correction encoding circuit on acetyl chloride (3 nuclei):

| Placement | Computed | Target |
|---|---|---|
| Optimal: a→C2, b→C1, c→M | 136 | **136** |
| Suboptimal: a→M, b→C2, c→C1 | 770 | **770** |

### Table II (Row 1)

| Circuit | Environment | Target runtime |
|---|---|---|
| Error-correction encoding (3q, 9 gates) | Acetyl chloride | **0.0136 s** |

### Table III

Sweeps the `Threshold` parameter (50 → 10000) controlling which nucleus interactions are considered "fast", reproducing the subcircuit count and runtime trade-off shown in the paper.

---

## Repository Structure

```
implement/
├── include/               # Shared headers (types, gate, placement, circuit, router)
├── src/
│   ├── physical_env.cpp   # Coupling graph + W-value queries
│   ├── quantum_circuit.cpp# DP runtime computation
│   ├── placement.cpp      # Logical→physical mapping + permutation derivation
│   ├── swap_circuit.cpp   # SWAP layer representation
│   ├── algorithm/
│   │   └── circuit_placer.cpp      # Subcircuit placement (monomorphism + fine-tuning)
│   └── permutation/
│       └── permutation_router.cpp  # Divide-and-conquer SWAP routing
├── data/environments/
│   └── acetyl_chloride.env         # Verified W values (from paper Table I)
└── main.cpp               # Runs all three verification checks
```

---

## Reference

*D. Maslov, S. M. Falconer, and M. Mosca, "Quantum circuit placement," IEEE Trans. Comput.-Aided Des. Integr. Circuits Syst., vol. 27, no. 4, pp. 752–763, Apr. 2008.*
