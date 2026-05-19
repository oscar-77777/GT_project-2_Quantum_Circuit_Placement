# Quantum Circuit Placement — Implementation Guide

Based on: *Quantum Circuit Placement*, Maslov, Falconer & Mosca, IEEE TCAD 2008.

---

## 目錄

1. [專案目標](#1-專案目標)
2. [整體架構](#2-整體架構)
3. [檔案結構](#3-檔案結構)
4. [已完成的部分](#4-已完成的部分)
5. [Partner A 待實作：Circuit Placement Algorithm](#5-partner-a-待實作circuit-placement-algorithm)
6. [Partner B 待實作：Fast Permutation Router](#6-partner-b-待實作fast-permutation-router)
7. [介面對接規範](#7-介面對接規範)
8. [建置與執行](#8-建置與執行)
9. [驗證目標（Tables II & III）](#9-驗證目標tables-ii--iii)
10. [論文關鍵資料整理](#10-論文關鍵資料整理)

---

## 1. 專案目標

實作論文 Section V 的 **Heuristic Solution**，分為兩個子演算法：

| 演算法 | 論文章節 | 功能 |
|---|---|---|
| **Algorithm** (Subcircuit Placement) | Section V-A | 將電路切成多個 subcircuit，為每個找最佳 logical→physical qubit 對應 |
| **Fast Permutation Circuits** | Section V-B | 在兩個連續 subcircuit 的 placement 之間，生成用 SWAP gate 實現 permutation 的電路 |

最終電路結構：
```
C₁  E₁₂  C₂  E₂₃  C₃  ...  E_{t-1,t}  Cₜ
```
- `Cᵢ`：第 i 個 subcircuit（已根據 placement Pᵢ 排好）
- `Eᵢ,ᵢ₊₁`：SWAP circuit，負責從 Pᵢ 的排列轉換成 Pᵢ₊₁ 的排列

**模擬目標**：重現論文 Table II（第一列）與 Table III 的數值結果。

---

## 2. 整體架構

### 資料流

```
QuantumCircuit + PhysicalEnvironment + Threshold
        │
        ▼
 ┌──────────────────┐
 │  CircuitPlacer   │  ← Partner A 實作
 │  (Section V-A)   │
 └──────────────────┘
        │ PlacementResult
        │ { subcircuits[], placements[] }
        │
        ├──────────────────────────────────────────────┐
        ▼                                              ▼
 ┌──────────────────┐                    對每對相鄰 placement
 │ PermutationRouter│  ← Partner B 實作  呼叫 routeBetween()
 │  (Section V-B)   │
 └──────────────────┘
        │ SwapCircuit[]
        │
        ▼
 totalRuntime = Σ subcircuit_runtime + Σ swap_depth × swap_cost
```

### 核心數學定義（論文 Section III）

**GateOperatingTime（論文 Def. 3）：**
```
GateOperatingTime(G(qᵢ, qⱼ)) = W(P(qᵢ), P(qⱼ)) × T(G)
```
- `W(u, v)`：物理環境中 nucleus u 和 v 的交互作用時間成本
- `T(G)`：gate 本身的基礎執行時間（NMR 中 T(ZZ(90°))=1, T(Rz)=0）

**Circuit Runtime 的 DP 計算（論文 Section III）：**
```
time[0..n-1] = 0
for each gate G in order:
    if two-qubit on (t, c):
        time[c] = max(time[c], time[t]) + W(P(t), P(c)) × T(G)
        time[t] = time[c]
    if single-qubit on t:
        time[t] += W(P(t), P(t)) × T(G)
return max(time[0..n-1])
```

---

## 3. 檔案結構

```
implement/
├── CMakeLists.txt              ← 建置設定（CMake）
├── IMPLEMENTATION_GUIDE.md     ← 本文件
│
├── include/                    ← 所有 header（兩人共用）
│   ├── types.h                 ✅ 已完成  QubitID, NucleusID, Weight 等基本型別
│   ├── gate.h                  ✅ 已完成  Gate struct，makeSingleGate/makeTwoGate
│   ├── physical_env.h          ✅ 已完成  PhysicalEnvironment 介面
│   ├── quantum_circuit.h       ✅ 已完成  QuantumCircuit 介面
│   ├── placement.h             ✅ 已完成  Placement 介面（A、B 的對接點）
│   ├── swap_circuit.h          ✅ 已完成  SwapCircuit, SwapLevel
│   ├── circuit_placer.h        ✅ 已完成  PlacementResult + CircuitPlacer 介面（Partner A）
│   └── permutation_router.h    ✅ 已完成  PermutationRouter 介面（Partner B）
│
├── src/
│   ├── physical_env.cpp        ✅ 已完成  完整實作，含 fromFile()
│   ├── quantum_circuit.cpp     ✅ 已完成  完整實作，含 computeRuntime() DP
│   ├── placement.cpp           ✅ 已完成  完整實作，含 permutationTo()
│   ├── swap_circuit.cpp        ✅ 已完成  完整實作，含 apply()
│   │
│   ├── algorithm/
│   │   └── circuit_placer.cpp  ⚠️  Partner A 待實作
│   │       框架已搭好；findMonomorphisms() 已寫好；
│   │       basicPlacement() 和 fineTuning() 只有 stub
│   │
│   ├── permutation/
│   │   └── permutation_router.cpp  ⚠️  Partner B 待實作
│   │       框架已搭好；routeBetween() 和 route() 已寫好；
│   │       partition() 和 routeSubgraph() 只有 stub
│   │
│   └── main.cpp                ✅ 已完成  驗證框架（Example 3, Table II/III）
│
└── data/
    └── environments/
        └── acetyl_chloride.env ✅ 已完成  W 值已驗證（見 Section 10）
```

---

## 4. 已完成的部分

### 4.1 共用資料結構（全部完整實作）

#### `types.h`
```cpp
using QubitID   = int;    // 邏輯 qubit 的編號（0-based）
using NucleusID = int;    // 物理 qubit（原子核）的編號（0-based）
using Weight    = double; // 時間成本（單位：1/10000 s）
constexpr NucleusID UNASSIGNED = -1;
```

#### `gate.h` — Gate struct
```cpp
struct Gate {
    GateType type;   // Single 或 Two
    QubitID  q1;     // 主 qubit（一定有效）
    QubitID  q2;     // 次 qubit（只有 Two 時有效，否則 UNASSIGNED）
    Weight   time;   // T(G)：gate 基礎執行時間
    int      level;  // 電路層（同一層可並行）
};
```
提供 `makeSingleGate()` 和 `makeTwoGate()` 工廠函數。

#### `physical_env.h` — PhysicalEnvironment
已實作的方法：
- `setTwoQubitWeight(u, v, w)` / `setSingleQubitWeight(u, w)`
- `twoQubitWeight(u, v)` / `singleQubitWeight(u)` — 查詢 W 值
- `gateOperatingTime(baseTime, n1, n2)` — 計算 W × T
- `fastEdges(threshold)` — 回傳所有 W ≤ threshold 的邊
- `fastAdjacency(threshold)` — 回傳 fast interaction 的 adjacency list
- `fromFile(path)` — 從 `.env` 檔案載入

#### `placement.h` — Placement
已實作的方法：
- `assign(logical, physical)` — 設定映射 P(qᵢ) = νⱼ
- `get(logical)` — 查詢映射
- `isValid()` — 檢查是否為 injective（一對一）
- `permutationTo(next)` — **A→B 對接的關鍵函數**，推導兩個 Placement 之間的 permutation vector

`permutationTo()` 回傳格式：`perm[dest] = src`，代表「目前在 nucleus src 的值，要移到 nucleus dest」。

#### `swap_circuit.h` — SwapCircuit
已實作的方法：
- `addLevel(SwapLevel)` — 新增一個 SWAP 層
- `depth()` — 回傳層數（優化目標，愈少愈好）
- `apply(state)` — 將 SWAP 電路套用到狀態向量，用於驗證正確性

#### `quantum_circuit.h` — QuantumCircuit
已實作的方法：
- `addGate(g)` — 新增 gate
- `subcircuit(begin, end)` — 截取子電路
- `computeRuntime(placement, env)` — **核心 DP 計算**，回傳給定 placement 下的電路執行時間
- `fromFile(path)` — 從 `.circ` 檔案載入

### 4.2 驗證框架（main.cpp）

已完成三個驗證函數：

**`verifyExample3()`** — 驗證論文 Section III 的手動算例：
```
輸出（已確認正確）：
  最佳 placement a→C2, b→C1, c→M：runtime = 136 ✓  (目標: 136)
  次佳 placement a→M,  b→C2, c→C1：runtime = 770 ✓  (目標: 770, 論文 Table I)
```

**`runTableII()`** — Table II 第一列框架已建立：
```
目前輸出：0.0143 sec  (stub 用 identity placement)
完成後目標：0.0136 sec (論文 Table II, row 1)
```

**`runTableIII()`** — Table III 掃描 Threshold 的框架已建立，等待演算法實作。

### 4.3 acetyl_chloride.env — 已驗證的 W 值

W 值由論文 Table I 的 DP 推導，並經 Example 3 交叉驗證：

```
W(M,M)=8,  W(C1,C1)=8, W(C2,C2)=1    (single-qubit)
W(M,C1)=38,  W(M,C2)=672,  W(C1,C2)=89  (two-qubit)
```

---

## 5. Partner A 待實作：Circuit Placement Algorithm

**負責檔案**：`src/algorithm/circuit_placer.cpp`

### 5.1 已提供的工具

`findMonomorphisms()` 函數已在 `circuit_placer.cpp` 的 anonymous namespace 中完整實作（backtracking VF2-style）：

```cpp
// 用法範例（在 basicPlacement 中呼叫）：
auto fastAdj = env_.fastAdjacency(threshold_);  // 取 fast interaction graph
auto monoResults = findMonomorphisms(patternAdj, fastAdj, /*maxResults=*/100);
// monoResults[k][i] = 第 k 個 monomorphism 中，邏輯 qubit i 對應的 physical nucleus
```

### 5.2 `basicPlacement()` 的實作步驟

```
輸入：circuit（完整電路）、startGate（從哪個 gate 開始）、placement（輸出參數）
輸出：endGate（workspace 的最後一個 gate 的下一個 index）
```

**步驟 1**：建立 fast adjacency graph（已有工具）
```cpp
auto fastAdj = env_.fastAdjacency(threshold_);
// fastAdj[u] = 所有與 u 有 fast interaction 的 nucleus
```

**步驟 2**：從 startGate 開始，逐一加入 gate 到 workspace C
```
for (int g = startGate; g < circuit.numGates(); g++) {
    把 gates_[g] 加入 workspace C

    if gates_[g] 是 two-qubit gate {
        更新 patternAdj（C 中所有有 two-qubit gate 的 qubit 對構成的 adjacency）
        呼叫 findMonomorphisms(patternAdj, fastAdj)
        if 回傳空集合:
            return g  // 這個 gate 加不進去，workspace 到此為止
    }
}
return circuit.numGates()  // 全部 gate 都能放入
```

**步驟 3**：從所有 monomorphism 中選出最佳 placement
```cpp
double bestRuntime = INF;
for (auto& mono : monoResults) {
    // mono[i] = qubit i 對應的 nucleus
    Placement candidate(circuit.numQubits(), env_.numNuclei());
    for (int q = 0; q < nQubits; q++)
        candidate.assign(q, mono[q]);

    QuantumCircuit sub = circuit.subcircuit(startGate, endGate);
    double rt = sub.computeRuntime(candidate, env_);
    if (rt < bestRuntime) {
        bestRuntime = rt;
        placement = candidate;
    }
}
```

**步驟 4**（若無 monomorphism）：hill-climbing
```
for each logical qubit qi in workspace C that has a two-qubit gate:
    for each physical nucleus nu in {ν₁,...,νₘ}:
        試著將 qi 改映射到 nu（其他不變）
        if 新的 runtime 更小:
            接受這次改變
重複直到無法改善
```

### 5.3 `fineTuning()` 的實作步驟

**目標**：在 basicPlacement 找到的 placement 基礎上，進一步最小化考慮 single-qubit gate 後的 runtime。

**Paper 的 Fine Tuning（Section V-A 後半）：**

1. 對所有 monomorphism M₁...Mₖ，計算當前 subcircuit cost C_{1,j}
2. 同時計算下一個 subcircuit 的最小 SWAP cost
3. 選擇 `C_{i,j} + min(S_j)` 最小的 Mᵢ

**簡化版（可先實作）：**
```
bool improved = true;
while (improved) {
    improved = false;
    for each qubit qi with a two-qubit gate in sub:
        for each nucleus nu:
            if 改映射 qi→nu 後 computeRuntime() 更小:
                接受，improved = true
}
```

### 5.4 `place()` 的主迴圈（已完成，勿修改）

```cpp
PlacementResult CircuitPlacer::place(const QuantumCircuit& circuit) {
    // 這個框架已完成，只需要讓 basicPlacement + fineTuning 正確實作即可
    while (startGate < total) {
        Placement p(circuit.numQubits(), env_.numNuclei());
        int endGate = basicPlacement(circuit, startGate, p);  // ← Partner A 實作
        QuantumCircuit sub = circuit.subcircuit(startGate, endGate);
        fineTuning(sub, p);                                     // ← Partner A 實作
        result.subcircuits.push_back(sub);
        result.placements.push_back(p);
        startGate = endGate;
    }
    return result;
}
```

### 5.5 驗證方式

實作完成後，`verifyExample3` 區塊不變，`runTableII()` 應輸出：
```
error corr. encoding (3q)  acetyl chloride  0.0136 sec
```
（目前 stub 輸出 0.0143 sec，差距來自使用了 identity placement 而非最佳 a→C2, b→C1, c→M）

---

## 6. Partner B 待實作：Fast Permutation Router

**負責檔案**：`src/permutation/permutation_router.cpp`

### 6.1 已完成的部分

`routeBetween()` 和 `route()` 的主框架已完成，只需實作兩個私有函數。

### 6.2 `partition()` 的實作步驟

```
輸入：nodes（當前 subgraph 的 nucleus 集合）、adj（fast adjacency）
輸出：Partition { g1, g2, channel }
  - g1, g2：兩個連通子圖的節點集合，|g1| ≈ |g2|（盡量平衡）
  - channel：連接 g1 和 g2 的邊 (u∈g1, v∈g2)
```

**演算法（BFS spanning tree 方式）：**
```
1. 從 nodes[0] 開始 BFS，建立 spanning tree
2. 尋找 spanning tree 中，移除後能讓兩側最平衡的邊
   (即讓 |g1| 最接近 |nodes|/2 的邊)
3. 移除該邊，以 BFS 重新確認 g1, g2 各為連通
4. 令 channel = (g1 中距離 g2 最近的節點, g2 中距離 g1 最近的節點)
```

**注意事項：**
- 必須用 `nodes` 過濾 adj，只考慮 nodes 內部的邊（不要考慮到 nodes 外部的 nucleus）
- 論文假設 interaction graph 是 connected，且 separability s=1/2（NMR 分子圖都滿足）
- 若 nodes 只有 2 個節點，直接回傳 g1={nodes[0]}, g2={nodes[1]}, channel=nodes[0]-nodes[1]

### 6.3 `routeSubgraph()` 的實作步驟

```
輸入：
  state[i]   = 目前 nucleus i 存放的 logical qubit 值（初始時 state[i]=i）
  target[i]  = nucleus i 最終應存放的值（即 perm[i]）
  nodes      = 當前 subgraph 的 nucleus 集合
  adj        = fast adjacency（全域，用 nodes 過濾）
  levels     = 輸出的 SWAP 層（append 到此 vector）
  levelOffset = 從第幾層開始寫入
```

**Phase A：把值移到正確的子圖側**

```
1. 呼叫 partition(nodes, adj) → G1, G2, channel=(u,v)

2. 對每個 nucleus n in nodes，判斷顏色：
   - "需要去 G1 但目前在 G2"：白色（bubble）
   - "需要去 G2 但目前在 G1"：黑色（bubble）
   - 已在正確側：不是 bubble

3. 對 G1 建立以 u（channel.first）為根的 spanning tree T1
   對 G2 建立以 v（channel.second）為根的 spanning tree T2

4. 每個時間步驟（step=0,1,...,2k）：
   偶數步：
     對 G1 的 spanning tree：
       從葉節點往根（u）方向，若父節點是 bubble 而子節點不是 bubble，
       就 SWAP(parent, child) → bubble 往根方向移動
     對 G2 同理（往根 v 方向）
   奇數步（使用 channel）：
     若 u 是 white bubble（值需要去 G1），且 v 是 black bubble：
       SWAP(u, v) → 一個值過橋
     [Leaf-target override 優化]：
       若某 leaf 已持有其 target 值，將其從 active set 移除

5. 重複直到所有值都在正確的子圖側（state 中，G1 的節點只持有 target 屬於 G1 的值）

6. 記錄每個步驟的 SWAP 到 levels[levelOffset + step]
```

**Phase B：遞迴**
```
phaseACost = 實際使用的步驟數
routeSubgraph(state, target, G1_nodes, adj, levels, levelOffset + phaseACost)
routeSubgraph(state, target, G2_nodes, adj, levels, levelOffset + phaseACost)
// G1 和 G2 現在獨立，可以同一時間步驟並行（interleave levels）
```

**Paper 中的物理比喻（Figure 3）：**
- G1 = 左容器（air），G2 = 右容器（water）
- White bubbles = 水泡（air），Black bubbles = 水滴（water）
- Channel = 連接兩容器的管道
- 演算法讓水和氣通過管道分別落到各自的容器

### 6.4 驗證方式

**簡單驗證（2-3 nucleus）：**
```cpp
// 手動測試：確認 apply() 後狀態正確
SwapCircuit sc = router.routeBetween(placement_from, placement_to);
std::vector<int> state = {0, 1, 2};  // initial state
std::vector<int> result = sc.apply(state);
// result 應該對應到 placement_to 的排列
```

**depth 驗證（Table III 的間接驗證）：**
- 對 3-qubit acetyl chloride，permutation depth 應 ≤ 2k = 4 levels
- 完整 Table III 實作後，結果應接近論文數值

---

## 7. 介面對接規範

### 7.1 唯一的對接點：PlacementResult

Partner A 生產，main.cpp 消費（呼叫 Partner B）：

```cpp
// main.cpp 中的對接程式碼（已完成，勿改）：
CircuitPlacer    placer(env, threshold);
PlacementResult  result = placer.place(circuit);  // ← A 的產出

PermutationRouter router(env, threshold);
std::vector<SwapCircuit> swaps;
for (int i = 0; i + 1 < result.placements.size(); i++)
    swaps.push_back(router.routeBetween(result.placements[i],
                                        result.placements[i+1]));  // ← B 的產出

double total = placer.totalRuntime(result, swaps, swapLevelCost);
```

### 7.2 permutationTo() 的語意（A→B 的橋梁）

```cpp
// placement_A: { q0→nucleus2, q1→nucleus1, q2→nucleus0 }
// placement_B: { q0→nucleus0, q1→nucleus2, q2→nucleus1 }
std::vector<int> perm = placement_A.permutationTo(placement_B);
// perm[0] = 2  → nucleus 2 的值要移到 nucleus 0
// perm[2] = 1  → nucleus 1 的值要移到 nucleus 2
// perm[1] = 0  → nucleus 0 的值要移到 nucleus 1
```

### 7.3 Threshold 一致性

兩人建構時**必須使用相同的 threshold**：
```cpp
Weight threshold = 200.0;  // 或由 Table III 迴圈給定
CircuitPlacer    placer(env, threshold);   // Partner A
PermutationRouter router(env, threshold);  // Partner B — 同一個 threshold！
```

---

## 8. 建置與執行

### 編譯（MinGW g++ on Windows）

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

### 編譯（CMake，若已安裝）

```bash
cmake -B build -S .
cmake --build build
./build/placer
```

### 目前的執行輸出（stub 狀態）

```
=== VERIFY Example 3 ===
Computed runtime: 136  (target: 136)  ✓
Suboptimal runtime: 770  (target: 770)  ✓

=== TABLE II ===
error corr. encoding (3q)  acetyl chloride  0.0143 sec
  Target: 0.0136 sec

=== TABLE III ===
（所有 threshold 均為 0.0143，因為 stub 的 place() 輸出相同的 identity placement）
```

---

## 9. 驗證目標（Tables II & III）

### Table II — 目標數值

| Circuit | # gates | # qubits | Environment | # nuclei | 目標 runtime | Search space |
|---|---|---|---|---|---|---|
| error correction encoding | 9 | 3 | acetyl chloride | 3 | **0.0136 sec** | 6 |
| 5-bit error correction | 25 | 5 | trans-crotonic acid | 7 | 0.0779 sec | 2,520 |
| pseudo-cat state prep | 54 | 10 | histidine | 12 | 0.5170 sec | 239,500,800 |

目前只有第一列（acetyl chloride）的電路和環境已實作。第二、三列需要另外建立電路和環境資料（參考論文引用 [12], [20]）。

### Table III — 目標數值（部分）

針對 7-qubit trans-crotonic acid（[12]）環境，不同 Threshold：

| Circuit | T=50 | T=100 | T=200 | T=500 | T=1000 | T=10000 |
|---|---|---|---|---|---|---|
| phaseest | .1636 (7) | .0699 (4) | .0699 (4) | .0700 (3) | .2156 (2) | .1812 (1) |
| qft6 | .3766 (9) | .3294 (5) | .2237 (5) | .2308 (5) | .3120 (3) | .4137 (1) |

括號內的數字是 subcircuit 的數量。

---

## 10. 論文關鍵資料整理

### Acetyl Chloride（論文 Fig. 1）

**分子圖（3 nuclei）：**

```
     M (0)
    / \
 38/   \672
  /     \
C1(1)—89—C2(2)
```

**完整 W 值（已驗證）：**

| | M (0) | C1 (1) | C2 (2) |
|---|---|---|---|
| **M (0)** | W(M,M)=8 | W(M,C1)=38 | W(M,C2)=672 |
| **C1 (1)** | 38 | W(C1,C1)=8 | W(C1,C2)=89 |
| **C2 (2)** | 672 | 89 | W(C2,C2)=1 |

**推導來源（論文 Table I, DP 追蹤）：**
```
Placement a→M, b→C2, c→C1（次佳，runtime=770）：
  Step 1 Y90 on a=M:     time[a] = W(M,M)×1 = 8
  Step 2 ZZ on a=M,b=C2: time[a]=time[b] = 8 + W(M,C2)×1 = 680  → W(M,C2)=672
  Step 3 Y90 on c=C1:    time[c] = W(C1,C1)×1 = 8
  Step 4 ZZ on b=C2,c=C1: time[b]=time[c] = 680 + W(C2,C1)×1 = 769 → W(C2,C1)=89
  Step 5 Y90 on b=C2:    time[b] = 769 + W(C2,C2)×1 = 770  → W(C2,C2)=1

Placement a→C2, b→C1, c→M（最佳，runtime=136）：
  Step 1 Y90 on a=C2:    time[a] = 1
  Step 2 ZZ on a=C2,b=C1: time = 1 + 89 = 90
  Step 3 Y90 on c=M:     time[c] = 8
  Step 4 ZZ on b=C1,c=M: time = 90 + W(C1,M)×1 = 128 → W(M,C1)=38
  Step 5 Y90 on b=C1:    time[b] = 128 + 8 = 136  ✓
```

### Error-Correction Encoding Circuit（論文 Fig. 2）

**5 個有效 gate（T > 0）+ 4 個 free Rz（T=0）= 9 gates total：**

```
a ──[Y90]──────────────────────[Rz]──
         │                          
b ──────[ZZ]──────────[ZZ]──[Y90]──[Rz]──
                  │    │              
c ────────────[Y90]──[ZZ]────────────[Rz]──[Rz]──
```

| Gate | 類型 | qubit | T | level |
|---|---|---|---|---|
| Y90 on a | Single | 0 | 1 | 0 |
| ZZ on a,b | Two | 0,1 | 1 | 1 |
| Y90 on c | Single | 2 | 1 | 2 |
| ZZ on b,c | Two | 1,2 | 1 | 3 |
| Y90 on b | Single | 1 | 1 | 4 |
| Rz on a | Single | 0 | 0 | 5 |
| Rz on b | Single | 1 | 0 | 5 |
| Rz on c | Single | 2 | 0 | 5 |
| Rz on a | Single | 0 | 0 | 6 |

### NMR Gate 時間規則

| Gate | T 值 | 原因 |
|---|---|---|
| ZZ(θ) | 1（最快 interaction 歸一化） | Drift Hamiltonian，等待時間 |
| Ry(θ), Rx(θ) | 1（for 90°） | RF pulse，長度 ∝ 旋轉角度 |
| Rz(θ) | **0**（free） | 改變 rotating reference frame，不需實際等待 |

### 演算法複雜度摘要

| 演算法 | 時間複雜度 | 空間複雜度 |
|---|---|---|
| `findMonomorphisms` | O(k × m^n) worst case；小圖快 | O(k) |
| `basicPlacement` | O(k × g²) 其中 g=gates | O(k) |
| `fineTuning` | O(n × m) per iteration | O(1) |
| `routeSubgraph` | O(n) depth levels（論文 eq. 2） | O(n) |
| `partition` | O(n²) BFS | O(n) |

---

## 附錄：待新增的電路與環境資料

完整重現 Table II/III 還需要：

| 資源 | 論文來源 | 用途 |
|---|---|---|
| trans-crotonic acid 環境（7q） | [12] Fig. 3 | Table II row 2, Table III 全部 |
| histidine 環境（12q） | [20] Fig. 2 | Table II row 3, Table III 12q 部分 |
| BOC-glycine 環境（5q） | [16] | Table III 5q |
| pentafluoro 環境（5q） | [24] | Table III 5q |
| 5-bit error correction circuit | [12] Fig. 1 | Table II row 2 |
| pseudo-cat state circuit | [20] Fig. 1 | Table II row 3 |
| phaseest circuit（5q） | — | Table III |
| qft6 circuit（6q） | [21] p.219 | Table III |
| aqft9 circuit（9q） | — | Table III |
| steane-x/z1, steane-x/z2（9q） | [7] Figs. 10.16–10.17 | Table III |
| aqft12 circuit（12q） | — | Table III |

這些電路的資料格式請參考 `data/` 目錄下的 `.circ` 和 `.env` 格式規範（可參考 `quantum_circuit.cpp::fromFile()` 和 `physical_env.cpp::fromFile()` 中的格式說明）。
