# 5-Qubit Visualization Analysis
> Generated 2026-05-21 — based on reading boc_glycine_fluoride.env, five_bit_error_corr.circ, phaseest.circ

---

## 1. 分子環境：BOC-Glycine-Fluoride（5 核）

### 核（Nucleus）索引
| 索引 | 核種 | 說明 |
|------|------|------|
| 0 | F  | ¹⁹F fluoride |
| 1 | C1 | ¹³C alpha-carbon |
| 2 | C2 | ¹³C carbonyl |
| 3 | N  | ¹⁵N nitrogen |
| 4 | H  | amide ¹H |

### 單量子比特權重 W(u,u)
| 核 | W(u,u) |
|----|--------|
| F  | 5.0    |
| C1 | 20.0   |
| C2 | 20.0   |
| N  | 30.0   |
| H  | 10.0   |

### 雙量子比特權重 W(u,v)（全 10 對，K₅）
| 配對    | J 耦合 (Hz) | W 值   | 備註              |
|---------|------------|--------|-------------------|
| F – C1  | ~200       | 13     | ¹J，最快          |
| N – H   | ~90        | 28     | ¹J                |
| C1 – C2 | ~55        | 45     | ¹J                |
| F – C2  | ~20        | 125    | ²J                |
| C2 – N  | ~10        | 250    | ¹J（跨越閾值）    |
| C2 – H  | ~7         | 357    | ²J                |
| C1 – N  | ~5         | 500    | ²J                |
| C1 – H  | ~5         | 500    | ²J                |
| F – N   | ~3         | 833    | ³J                |
| F – H   | ~2         | 1250   | ⁴J，最慢          |

### Fast Graph（threshold = 200）
只有 W ≤ 200 的邊進入 fast graph：

```
F  – C1   (W=13)   ✓ fast
N  – H    (W=28)   ✓ fast
C1 – C2   (W=45)   ✓ fast
F  – C2   (W=125)  ✓ fast
──────────────────────────
C2 – N    (W=250)  ✗ slow
C2 – H    (W=357)  ✗ slow
C1 – N    (W=500)  ✗ slow
C1 – H    (W=500)  ✗ slow
F  – N    (W=833)  ✗ slow
F  – H    (W=1250) ✗ slow
```

**關鍵問題：fast graph 是 disconnected（兩個分量）**

```
分量 1: F – C1 – C2 – F（三角形，加上 F-C2 對角線）
            F
           / \
         C1 – C2

分量 2: N – H（單邊）
```

兩個分量之間沒有任何 fast edge，意味著：
- 需要 qubit 同時在 `{F,C1,C2}` 和 `{N,H}` 之間互動的電路
  **一定無法全部映射到 fast graph 上**
- basicPlacement 在遇到第一個跨分量的 ZZ gate 時就會截斷

---

## 2. 電路分析

### 2a. five_bit_error_corr.circ（5 qubit，25 gates）
**Interaction graph：幾乎是 K₅**

ZZ gates 覆蓋的配對：
```
(0,1), (1,2), (2,3),        ← Phase 2 g1
(1,4), (2,4),               ← Phase 2 g2
(0,2), (3,4),               ← Phase 2 g3（同層 level=6）
(0,3), (1,3), (0,4)         ← Phase 2 g4
```
共 10 對 = K₅ 完全圖（所有 5 qubit 兩兩都有 ZZ）。

**結果**：interaction graph = K₅ ⊄ fast graph（fast graph 有 4 條邊，且不連通）。
basicPlacement 會在第 4～5 個 ZZ gate 就截斷，產生 **多個小 subcircuit**。

### 2b. phaseest.circ（5 qubit，多 phases）
**Interaction graph：也幾乎是 K₅**

ZZ gates 配對：
```
Phase 2 controlled-U: (1,0), (2,0), (3,0), (4,0)   ← star on qubit 0
Phase 3 IQFT step1:   (3,4), (2,4), (2,3)
Phase 3 IQFT step2:   (1,4), (1,3), (1,2)
```
10 對中出現 8 對（少了 (0,2),(0,3),(0,4) 不算入 IQFT），仍接近 K₅。

**特殊點**：T 值不同（T=1, 2, 4, 8, 0.5, 0.25, 0.125）——代表不同旋轉角度的 ZZ gate，runtime 貢獻不同。

---

## 3. 演算法行為差異：3-qubit vs 5-qubit

| 特性 | 3-qubit（乙醯氯） | 5-qubit（BOC-glycine-fluoride） |
|------|------------------|--------------------------------|
| fast graph 結構 | 連通（path M–C1–C2）| **不連通**（{F,C1,C2} ∪ {N,H}） |
| interaction graph | path a–b–c（2 edges）| K₅（10 edges） |
| basicPlacement 截斷 | 不截斷（全部電路一個 subcircuit）| 在第一個跨分量 gate 截斷 |
| subcircuit 數量 | 1 | 多個（估計 3–6 個） |
| SWAP routing 複雜度 | 可能不需要（P₁=P₂）| 每對 subcircuit 間都需要 SWAP |
| 搜尋空間 | P(3,3)=6 | P(5,5)=120 |
| 最優解來自 | 6 個映射中找最小 runtime | 120 個映射，但受 fast graph 限制 |

---

## 4. 視覺化挑戰

### 為何「直接平移」3-qubit 圖到 5-qubit 會失敗

1. **節點/邊太多**：5 節點 × 10 邊的 K₅ 互動圖，加上 fast graph 4 條邊，圖會很擠
2. **電路有多個 subcircuit**：需要畫出「在哪裡截斷」以及截斷後的 SWAP 電路
3. **T 值不均勻**（phaseest 中 T=8 等）：runtime DP 計算不是簡單的「每個 gate 加一下」
4. **Fast graph 不連通**：這是 3-qubit 沒有的概念，需要額外解釋

---

## 5. 建議的視覺化方向

### 方案 A：聚焦環境對比（容易畫，論文價值高）

```
Fig A1: BOC-glycine-fluoride 5 節點環境圖
        - 5 個節點，10 條邊（fast=藍/slow=紅虛）
        - 清楚標示兩個 fast 分量（色塊背景）

Fig A2: Interaction Graph vs Fast Graph 對比（3-qubit 和 5-qubit 各一列）
        - 顯示「為何 5-qubit 需要拆 subcircuit」的直觀原因

Fig A3: subcircuit 拆分示意（只畫第一刀）
        - basicPlacement 在哪個 gate 截斷，為什麼
```

### 方案 B：純演算法流程圖（最適合報告）

```
Flow A: basicPlacement 流程
        開始 → 掃描 gate → 加入 pattern edge → 找 monomorphism → 找不到就截斷

Flow B: fineTuning 流程
        初始 placement → 逐個 qubit 嘗試換 nucleus → 計算 score（含 lookahead）→ 接受/拒絕

Flow C: routeSubgraph 流程（permutation router）
        partition → Phase A bubble → Phase B recurse 兩邊

Flow D: 主迴圈
        while 還有 gate → basicPlacement → fineTuning → routeSubgraph → 累積 runtime
```

### 方案 C：兩者都要（最完整）

A + B 全部生成，A 系列放「分子/問題定義」章節，B 系列放「演算法」章節。

---

## 6. 資料準確性說明

### 5-qubit 結果為何與論文有落差

1. **J-coupling 數值是估算值**（.env 中的 `# Approximate` 標記）
   - 正確值需要從 [16] Marx et al. (1999) 原文取得，但該論文研究的是不完全相同的分子
2. **phaseest 電路是重建的近似**
   - 論文沒有提供完整 gate list，是依照 Nielsen & Chuang 架構手動重建
3. **單量子比特 W 值估算方式不同**
   - 論文 Table I 只標示 acetyl chloride 的精確化位移數據

### 3-qubit 結果為何精確（136 = 最優）

- W(M,C1)=38, W(C1,C2)=89, W(M,C2)=672 全部來自論文 Table I 的直接計算
- 電路 gates 序列 (error_corr_encoding.circ) 與論文 Figure 1 完全對應
- DP runtime 可以手動驗證：max(1×38, 0) + 89×1 = 136

---

## 7. 後續行動選項

| 選項 | 工作量 | 論文效果 |
|------|--------|----------|
| 只做方案 A（環境+拆分對比）| 低 | 中 |
| 只做方案 B（演算法流程圖）| 中 | 高 |
| A + B 全做 | 高 | 最高 |
| 維持現有 6 張圖（3-qubit），不新增 | 零 | 現有已足夠 |

---

*此分析檔案基於讀取 boc_glycine_fluoride.env / five_bit_error_corr.circ / phaseest.circ 後整理，
可作為後續生成圖表的參考依據。*
