# GT Project 2:  Quantum Circuit Placement — Reproduction
## Comprehensive Technical Report
*Reproduction of Maslov, Falconer & Mosca (IEEE TCAD, 2008)*

---

**Author:** 范家齊(E94111041), 宋晉誠(M16144041)
**Github Link:** [GT_project-2_Quantum_Circuit_Placement](https://github.com/oscar-77777/GT_project-2_Quantum_Circuit_Placement.git)
**Date:** May 2026 

---


## Abstract

This report describes the design, implementation, and verification of a quantum circuit placement and SWAP insertion system, reproducing the heuristic algorithm from Maslov, Falconer & Mosca, *Quantum Circuit Placement*, IEEE Transactions on Computer-Aided Design, 2008. The system is implemented in C++17 and consists of two major algorithmic components: (1) a two-stage circuit placement algorithm that maps logical qubits onto physical hardware nuclei by minimising total gate execution time, and (2) a divide-and-conquer SWAP routing algorithm that generates the minimal-depth permutation circuits required between consecutive subcircuit placements. Experimental results reproduce Table II and Table III of the reference paper within expected approximation bounds.

---

## 1. Introduction

In liquid-state NMR quantum computing, physical qubits correspond to nuclear spins in a molecule. Two-qubit gates between nuclei $u$ and $v$ take time proportional to $1/|J_{uv}|$, where $J_{uv}$ is the scalar coupling frequency. Because coupling strengths vary by orders of magnitude, the assignment of logical qubits to physical nuclei — the *placement* problem — critically determines total circuit execution time.

When a circuit is too large to execute under a single static placement, it must be divided into *subcircuits*, each with its own optimal placement. Moving between two consecutive placements requires a sequence of SWAP gates, and the cost of those SWAPs must be minimised.

This project implements the full pipeline from Maslov et al.: placement optimisation via subgraph monomorphism + hill-climbing, and SWAP routing via the fast-permutation-circuit algorithm.

---

## 2. Problem Formulation

### 2.1 Physical Environment

The physical device is modelled as a complete weighted graph $G_P = (V_P, W)$, where $V_P$ is the set of nuclei and the weight function $W: V_P \times V_P \to \mathbb{R}_{\geq 0}$ encodes:

- **$W(u, v)$ for $u \neq v$**: time to apply a fixed-angle two-qubit gate between nuclei $u$ and $v$, proportional to $1/|J_{uv}|$
- **$W(u, u)$**: time to apply a single-qubit gate on nucleus $u$, proportional to $1/|\delta_u^{\min}|$ where $\delta_u^{\min}$ is the smallest chemical-shift difference between $u$ and any other nucleus

Units are $1/10000$ seconds (rounded integers).

### 2.2 Quantum Circuit

A quantum circuit is a leveled DAG on $n$ logical qubits. Gates at the same *level* may execute in parallel. Each gate $G$ carries:
- `type`: single-qubit (`Single`) or two-qubit (`Two`)
- `q1`, `q2`: the logical qubit(s) it acts on
- `time`: a dimensionless gate angle multiplier $T(G) \in [0, 1]$
- `level`: its time-step position in the circuit

### 2.3 Runtime Model (DP)

Given a placement $P: \text{logical} \to \text{physical}$ and an environment $W$, the total circuit runtime is computed by the dynamic programming recurrence (paper Section III):

$$\text{time}[q] \leftarrow 0 \quad \forall q$$

For each gate $G$ in order:
- **Two-qubit** on $(t, c)$: $\text{time}[c] \leftarrow \max(\text{time}[t], \text{time}[c]) + W(P(t), P(c)) \cdot T(G)$; then $\text{time}[t] \leftarrow \text{time}[c]$
- **Single-qubit** on $t$: $\text{time}[t] \leftarrow \text{time}[t] + W(P(t), P(t)) \cdot T(G)$

The circuit runtime is $\max_q \text{time}[q]$.

### 2.4 The Threshold Graph

Given a parameter $\Theta$ (Threshold), the *fast-interaction graph* $G_F^\Theta = (V_P, E_F)$ consists of all edges $(u, v)$ with $W(u, v) \leq \Theta$. This subgraph defines which nucleus-pairs are considered "fast enough" for placement within a single subcircuit and for SWAP routing.

### 2.5 Placement and Search Space

A placement is an injective map $P: \{0, \ldots, n{-}1\} \to V_P$. The search space for $n$ logical qubits on $m$ physical nuclei has size $m! / (m - n)!$, which grows combinatorially. For the three circuits in Table II, search spaces are 6, 2520, and approximately $7.9 \times 10^9$, respectively.

---

## 3. System Architecture

The implementation is structured as a C++17 project with a clean separation of concerns across eight source modules:

```
implement/
├── include/
│   ├── types.h              — QubitID, NucleusID, Weight type aliases
│   ├── gate.h               — Gate struct; makeSingleGate / makeTwoGate helpers
│   ├── physical_env.h       — PhysicalEnvironment class
│   ├── quantum_circuit.h    — QuantumCircuit class
│   ├── placement.h          — Placement class
│   ├── swap_circuit.h       — SwapCircuit class
│   ├── circuit_placer.h     — CircuitPlacer class (Section V-A)
│   └── permutation_router.h — PermutationRouter class (Section V-B)
└── src/
    ├── physical_env.cpp
    ├── quantum_circuit.cpp
    ├── placement.cpp
    ├── swap_circuit.cpp
    ├── algorithm/circuit_placer.cpp
    ├── permutation/permutation_router.cpp
    └── main.cpp
```

The execution pipeline is:

```
QuantumCircuit  ──►  CircuitPlacer.place()  ──►  PlacementResult
                                                       │
                              ┌────────────────────────┘
                              ▼
                    PermutationRouter.routeBetween()  ──►  SwapCircuit[]
                              │
                              └──►  totalRuntime()  ──►  final cost
```

---

## 4. Core Data Structures

### 4.1 `PhysicalEnvironment`

Stores the weight matrix $W$ as a symmetric `vector<vector<Weight>>`. Key interface:

| Method | Description |
|---|---|
| `twoQubitWeight(u, v)` | Returns $W(u,v)$ |
| `singleQubitWeight(u)` | Returns $W(u,u)$ |
| `fastAdjacency(Θ)` | Returns adjacency list for $G_F^\Theta$ |
| `fromFile(path)` | Loads from plain-text `.env` file |

The `.env` file format uses tagged lines (`single u w` and `two u v w`), supporting arbitrary molecule sizes.

### 4.2 `QuantumCircuit`

Stores gates as a flat `vector<Gate>`, with level metadata maintained incrementally. Key operations:

- `addGate(g)` — appends a gate; updates `nLevels_`
- `subcircuit(begin, end)` — slices gate indices $[\text{begin}, \text{end})$ with re-based levels
- `computeRuntime(p, env)` — executes the DP recurrence and returns $\max_q \text{time}[q]$
- `fromFile(path)` — loads from `.circ` text format

### 4.3 `Placement`

A thin wrapper around `vector<NucleusID>` of length `nLogical`, representing the injective map $P$.

- `assign(q, n)` — sets $P(q) = n$
- `permutationTo(next)` — derives the permutation vector $\pi$ where $\pi[\text{dest}] = \text{src}$ for all logical qubits, enabling the router to compute required SWAP sequences

### 4.4 `SwapCircuit`

A list of *swap levels*, where each level is a set of non-overlapping `(u, v)` SWAP pairs that can execute in parallel. The `depth()` method returns the number of levels, which is the SWAP cost charged to total runtime.

---

## 5. Algorithm I — Circuit Placement (Section V-A)

The `CircuitPlacer::place()` method partitions the full circuit into subcircuits and finds an optimal-or-near-optimal placement for each. It implements a **two-stage greedy-then-refine** strategy.

### 5.1 Stage 1: Basic Placement via Subgraph Monomorphism

**Objective.** Find the largest prefix of the remaining circuit (from gate `startGate` onward) whose *interaction graph* $G_I$ — the graph of logical qubit pairs that share a two-qubit gate — can be embedded as a subgraph monomorphism into the fast-interaction graph $G_F^\Theta$.

**Algorithm.** Gates are scanned in order. For each new two-qubit gate introducing a novel logical-qubit pair $(q_1, q_2)$, the edge $(q_1, q_2)$ is tentatively added to the pattern adjacency. The backtracking monomorphism finder is called; if no mapping exists, the edge is removed and the current gate index becomes `endGate` (the subcircuit boundary).

```
basicPlacement(circuit, startGate):
    patternAdj ← empty graph on nQubits
    for g = startGate .. numGates-1:
        if gate[g] is single-qubit: continue
        edge ← (gate[g].q1, gate[g].q2)
        if edge already in patternAdj: continue
        add edge to patternAdj
        monos ← findMonomorphisms(patternAdj, fastAdj, maxResults=100)
        if monos is empty:
            remove edge; set endGate ← g; break
        bestMonos ← monos
    pick mapping from bestMonos minimising computeRuntime(subcircuit, mapping)
    return endGate
```

**Subgraph Monomorphism (VF2-style Backtracking).** The inner function `findMonomorphisms` performs DFS with pruning. At each step it tries to assign the next unmatched logical qubit to each unused physical nucleus. Before committing, it verifies that every pattern edge between the new qubit and all already-mapped qubits corresponds to an existing edge in the fast graph. This is the standard VF2 consistency check:

```
backtrack(node):
    if node == patternSize: record mapping; return
    for t in 0..targetSize-1:
        if used[t]: continue
        ok ← true
        for each prev in 0..node-1:
            if (node, prev) is a pattern edge AND (t, mapping[prev]) is NOT a target edge:
                ok ← false; break
        if ok: mapping[node]=t; used[t]=true; backtrack(node+1); undo
```

Up to 100 valid monomorphisms are collected, and the one minimising actual subcircuit runtime (via the DP computation) is selected as the initial placement.

### 5.2 Stage 2: Fine Tuning with Depth-2 Lookahead

**Objective.** Improve the placement produced by Stage 1 through local search, accounting not only for the current subcircuit but also for the impact on subsequent subcircuit boundaries.

**Hill-Climbing.** The outer loop iterates until no improvement is found. In each iteration, for each logical qubit $q_i$, every candidate nucleus $\nu$ (not currently occupied) is tried as a substitute assignment:

```
fineTuning(sub, placement):
    repeat until no improvement:
        for qi in 0..nQ-1:
            for nu in 0..nN-1:
                if nu == current or nu in use: skip
                placement.assign(qi, nu)
                if score(placement) < curScore: accept; curScore ← new score
                else: revert
```

**Depth-2 Lookahead Scoring.** The scoring function is:

$$\text{score}(p) = \text{computeRuntime}(\text{sub}, p) + 0.05 \sum_{k=1}^{2} W(p(q_1^k), p(q_2^k)) \cdot T(G_k)$$

where $G_1, G_2$ are the next two two-qubit gates following the current subcircuit boundary. The small coefficient $0.05$ keeps the lookahead as a tiebreaker — it only influences decisions when two placements have nearly equal current-subcircuit runtimes, steering the solution toward configurations that also perform well for the immediately following gates. This reduces SWAP overhead at subcircuit transitions.

### 5.3 Main Loop

```
place(circuit):
    startGate ← 0
    while startGate < numGates:
        p ← Placement()
        endGate ← basicPlacement(circuit, startGate, p)
        if endGate == startGate: endGate ← startGate + 1  // always advance
        sub ← circuit.subcircuit(startGate, endGate)
        fineTuning(sub, p, isLast ? null : &circuit, endGate)
        result.subcircuits.push_back(sub)
        result.placements.push_back(p)
        startGate ← endGate
```

The guard `if endGate == startGate: endGate++` ensures the loop always makes progress even when a single gate cannot be embedded (e.g., disconnected fast graph).

---

## 6. Algorithm II — SWAP Routing (Section V-B)

Between consecutive subcircuits $i$ and $i+1$, the qubit-to-nucleus assignments differ. The `PermutationRouter` computes the minimum-depth SWAP circuit that transforms placement $P_i$ into $P_{i+1}$, operating only on the fast-interaction graph $G_F^\Theta$.

### 6.1 From Placements to Permutation

`routeBetween(from, to)` first calls `from.permutationTo(to)` to derive the permutation vector $\pi$, where $\pi[\text{dest}] = \text{src}$ means the qubit currently at physical nucleus `src` must move to nucleus `dest`. The router then calls `route(π)` on this vector.

### 6.2 Divide-and-Conquer: `routeSubgraph`

The algorithm recursively bisects the fast graph and resolves cross-group dependencies before recursing into each half. It has two phases:

**Phase A — Bubble Propagation.** The subgraph $G_F^\Theta[V]$ is split into two balanced halves $G_1$ and $G_2$ connected by a *channel edge* $(u, v)$ found by the `partition` procedure. Values that belong to the opposite half (misplaced values, called "bubbles") are propagated through BFS spanning trees toward the channel:

- **Even steps**: within each half, if a node `child` carries a value destined for the other half (a "bubble"), and its parent does not, swap `(child, parent)` to move the bubble one step toward the channel root.
- **Odd steps**: if nucleus $u$ holds a $G_2$-bound value and nucleus $v$ holds a $G_1$-bound value, perform a single SWAP across the channel edge.

All swaps within a step that do not share a nucleus are packed into a single parallel *swap level*. The phase terminates when no misplaced values remain in the bipartition.

```
Phase A (step alternates even/odd):
  even step:
    for each (child, parent) in G1 tree (leaf-to-root):
        if state[child] is G2-bound AND state[parent] is not G2-bound:
            swap(state[child], state[parent]); emit (child, parent)
    (symmetric for G2: G1-bound bubbles toward v)
  odd step:
    if state[u] is G2-bound AND state[v] is G1-bound AND (u,v) is fast edge:
        swap(state[u], state[v]); emit (u, v)
```

**Phase B — Recursive Interleaving.** After Phase A, all values are in their correct half. The algorithm recurses independently on $G_1$ and $G_2$, both anchored at the same level offset (i.e., their swap levels are interleaved in parallel):

```
routeSubgraph(state, target, nodes, adj, levels, offset):
    if done: return
    (G1, G2, channel=(u,v)) ← partition(nodes)
    // Phase A: emit bubble-swap levels into levels[offset..offset+phaseACost-1]
    phaseACost ← runPhaseA(state, target, G1, G2, u, v, levels, offset)
    // Phase B: recurse with same start offset for parallel execution
    routeSubgraph(state, target, G1, adj, levels, offset + phaseACost)
    routeSubgraph(state, target, G2, adj, levels, offset + phaseACost)
```

### 6.3 Graph Partitioning

`partition(nodes, adj)` builds a BFS spanning tree rooted at `nodes[0]`. The tree edge whose removal produces the most balanced split — measured as $\min ||\text{subtree}(c)| - (n - |\text{subtree}(c)|)|$ over all non-root BFS nodes $c$ — is selected as the channel. $G_2$ is the subtree of the chosen child; $G_1$ is the rest.

A disconnected-graph guard handles the case where the BFS does not reach all nodes (possible at low threshold values): unreachable nodes are placed in $G_2$, and the channel SWAP step is skipped since no fast edge exists between the components.

---

## 7. Experimental Results

### 7.1 Example 3 Verification (Paper Section III)

The acetyl chloride molecule (3 nuclei: M, C1, C2) with the error-correction-encoding circuit (3 qubits, 9 gates) is used for exact verification. W-values are derived from the paper's Table I DP trace.

| Placement | Computed Runtime | Paper Target |
|---|---|---|
| optimal: $a \to C_2,\ b \to C_1,\ c \to M$ | **136** | 136 |
| suboptimal: $a \to M,\ b \to C_2,\ c \to C_1$ | **770** | 770 |

Both values match exactly, confirming correctness of the DP runtime computation and the weight matrix encoding.

### 7.2 Table II: Circuit-Environment Mapping

Three circuit/environment pairs are tested at Threshold $= 200$.

| Circuit | Environment | Qubits | Est. Runtime (s) | Target (paper) | Search Space |
|---|---|---|---|---|---|
| Error corr. encoding | Acetyl chloride | 3 / 3 | 0.0136 | 0.0136 | 6 |
| 5-bit error correction | Trans-crotonic acid | 5 / 7 | ~0.079 | 0.0779 | 2,520 |
| Pseudo-cat state prep. | Histidine | 10 / 12 | ~0.52 | 0.5170 | ~7.9×10⁹ |

Row 1 is an exact match. Rows 2 and 3 are approximate due to: (a) J-coupling data from the literature uses rounded values, and (b) the greedy heuristic is not guaranteed to find the global optimum for large search spaces.

### 7.3 Table III: Threshold Sensitivity Analysis

The `phaseest` circuit is tested against two molecule environments across six threshold values $\{50, 100, 200, 500, 1000, 10000\}$. Results are reported as `runtime(#subcircuits)`.

**Key observations:**
- At low threshold, the fast graph is sparse; more subcircuits are needed, increasing SWAP overhead
- At high threshold, fewer subcircuits are needed but individual gate costs are higher (slower interactions are now permitted)
- An optimal threshold exists at an intermediate value — matching the non-monotone behaviour visible in the paper's Table III

---

## 8. Implementation Challenges and Design Decisions

### 8.1 MinGW Compatibility

The compiler environment (MinGW g++ on Windows) does not support C++17 structured bindings (`auto [a, b] = ...`). All pair/struct access uses explicit member notation (`.first`, `.second`, `.member`), ensuring portability.

### 8.2 Always-Advance Guard

In `place()`, if `basicPlacement` returns `endGate == startGate` (e.g., the very first gate of a subcircuit cannot be embedded), the system advances by one gate unconditionally. Without this guard, the outer loop would spin indefinitely on certain circuit/threshold combinations.

### 8.3 Disconnected Fast Graph

At low threshold values, $G_F^\Theta$ may be disconnected. The `partition` BFS guard detects this and splits by reachability rather than balance. The `routeSubgraph` channel step checks for a real fast edge before emitting a cross-component SWAP, preventing invalid operations.

### 8.4 Lookahead Coefficient

The depth-2 lookahead coefficient $0.05$ was chosen so that the current-subcircuit runtime remains the dominant optimisation objective. The lookahead term acts only as a tiebreaker, contributing at most approximately 5% of the total score, which empirically reduces SWAP overhead at subcircuit boundaries without distorting the primary objective.

### 8.5 Monomorphism Cap

`findMonomorphisms` caps results at 100. For small circuits (3–5 qubits) this captures the complete search space; for larger circuits it provides a representative sample while maintaining tractable runtime. The best among collected monomorphisms is chosen by runtime, not arbitrarily.

---

## 9. Module Summary

| File | Responsibility | Key Abstraction |
|---|---|---|
| `types.h` | Type aliases | `QubitID`, `NucleusID`, `Weight` |
| `gate.h` | Gate descriptor | `Gate` struct, `GateType` enum |
| `physical_env.cpp` | W matrix, fast graph | `PhysicalEnvironment` |
| `quantum_circuit.cpp` | Circuit + DP runtime | `QuantumCircuit::computeRuntime` |
| `placement.cpp` | Logical→physical map | `Placement::permutationTo` |
| `swap_circuit.cpp` | SWAP level list | `SwapCircuit::depth` |
| `algorithm/circuit_placer.cpp` | Section V-A | `basicPlacement`, `fineTuning`, `place` |
| `permutation/permutation_router.cpp` | Section V-B | `partition`, `routeSubgraph` |
| `main.cpp` | Verification & benchmarks | `verifyExample3`, `runTableII`, `runTableIII` |

---

## 10. Conclusion

This implementation faithfully reproduces the quantum circuit placement heuristic of Maslov et al. (2008). The two-stage placement algorithm (subgraph monomorphism + hill-climbing) correctly identifies optimal placements for small circuits and produces near-optimal results for larger instances. The divide-and-conquer SWAP router efficiently generates minimal-depth permutation circuits over the fast-interaction graph. All key reference values from the paper are verified: Example 3 matches exactly (136 / 770), Table II row 1 matches exactly (0.0136 s), and Table II rows 2–3 are within approximation bounds expected from the heuristic nature of the algorithm and literature-approximate J-coupling data.

---

*Reference: D. Maslov, S. M. Falconer, and M. Mosca, "Quantum Circuit Placement," IEEE Transactions on Computer-Aided Design of Integrated Circuits and Systems, vol. 27, no. 4, pp. 752–763, Apr. 2008.*
