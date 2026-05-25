"""
plot_search_space.py
====================
Reads the CSV files produced by brute_force.exe and generates a three-panel
figure proving that the placement algorithm finds the minimum (or near-minimum)
runtime over the entire search space.

Requirements: numpy, matplotlib, pandas
Run (from implement/ directory):
    python plot_search_space.py
Output:
    figures/output/search_space_proof.png
"""

import os
import csv
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")          # headless backend (no display needed)
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

# ---------------------------------------------------------------------------
# Load data
# ---------------------------------------------------------------------------

def load_summary(path="brute_force_data/summary.csv"):
    rows = []
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        for r in reader:
            rows.append({
                "row":         int(r["row"]),
                "circuit":     r["circuit"],
                "environment": r["environment"],
                "nLog":        int(r["nLog"]),
                "nPhys":       int(r["nPhys"]),
                "searchSpace": int(r["searchSpace"]),
                "algoResult":  float(r["algoResult"]),
                "bruteMin":    float(r["bruteMin"]),
                "isSampled":   int(r["isSampled"]),
                "nSamples":    int(r["nSamples"]),
            })
    return rows

def load_runtimes(path):
    df = pd.read_csv(path)
    rt = df["runtime"].values
    return rt[rt > 0]   # drop zero-runtime entries (unassigned-qubit guard)

# ---------------------------------------------------------------------------
# Percentile rank of the algorithm result among all sampled runtimes.
# Returns the fraction of samples that are >= algo (i.e. algo beats them).
# ---------------------------------------------------------------------------

def percentile_rank(values, algo):
    return 100.0 * np.mean(values >= algo)

# ---------------------------------------------------------------------------
# Panel helpers
# ---------------------------------------------------------------------------

COLORS = {
    "bar_default": "#90CAF9",   # light blue
    "bar_min":     "#1565C0",   # dark blue (brute-force minimum)
    "algo":        "#D32F2F",   # red  (algorithm result)
    "hist":        "#90CAF9",
    "hist_edge":   "#5C6BC0",
}

def _algo_label(algo, units_per_sec=10000):
    secs = algo / units_per_sec
    return f"Algorithm result\n({secs:.4f} s)"

# --- Row 1: bar chart (P(3,3) = 6 placements) ---

def plot_row1(ax, runtimes, algo, meta):
    labels = [
        "a→M, b→C1, c→C2", "a→M, b→C2, c→C1",
        "a→C1, b→M, c→C2", "a→C1, b→C2, c→M",
        "a→C2, b→M, c→C1", "a→C2, b→C1, c→M",
    ]
    n = len(runtimes)
    # sort by runtime so the minimum is visible
    order = np.argsort(runtimes)
    sorted_rt = runtimes[order]
    sorted_lb = [labels[i] if i < len(labels) else f"perm {i}" for i in order]

    colors = [COLORS["bar_min"] if np.isclose(rt, np.min(runtimes)) else COLORS["bar_default"]
              for rt in sorted_rt]

    bars = ax.barh(range(n), sorted_rt, color=colors, edgecolor="white", height=0.6)

    # Mark algorithm result
    ax.axvline(algo, color=COLORS["algo"], linewidth=2.5, linestyle="--",
               label=_algo_label(algo), zorder=5)

    ax.set_yticks(range(n))
    ax.set_yticklabels(sorted_lb, fontsize=8)
    ax.set_xlabel("Runtime (units, × 10⁻⁴ s)", fontsize=9)
    ax.set_title(
        f"Row 1: error-corr encoding → acetyl chloride\n"
        f"All P({meta['nPhys']},{meta['nLog']}) = {meta['searchSpace']} placements enumerated",
        fontsize=9, fontweight="bold"
    )

    bf_min = np.min(runtimes)
    ax.annotate(
        f"Min = {bf_min:.0f} units\n= {bf_min/10000:.4f} s",
        xy=(bf_min, 0), xytext=(bf_min + (np.max(runtimes)-bf_min)*0.15, 0.5),
        fontsize=8, color=COLORS["bar_min"],
        arrowprops=dict(arrowstyle="->", color=COLORS["bar_min"]),
    )

    proof_text = (
        f"Algorithm result = {algo:.0f}\n"
        f"Brute-force min  = {bf_min:.0f}\n"
        f"{'✓ EXACT minimum' if np.isclose(algo, bf_min) else f'Δ = {algo - bf_min:.1f}'}"
    )
    ax.text(0.98, 0.05, proof_text, transform=ax.transAxes,
            fontsize=8, va="bottom", ha="right",
            bbox=dict(boxstyle="round,pad=0.3", fc="lightyellow", ec="orange", alpha=0.9))

    legend_patches = [
        mpatches.Patch(color=COLORS["bar_min"],     label="Brute-force minimum"),
        mpatches.Patch(color=COLORS["bar_default"],  label="Other placements"),
        plt.Line2D([0], [0], color=COLORS["algo"], lw=2.5, ls="--", label=_algo_label(algo)),
    ]
    ax.legend(handles=legend_patches, fontsize=7, loc="lower right")

# --- Row 2: histogram (P(7,5) = 2520 placements) ---

def plot_row2(ax, runtimes, algo, meta):
    bf_min = np.min(runtimes)
    rank   = percentile_rank(runtimes, algo)

    n_bins = min(60, len(np.unique(runtimes)))
    ax.hist(runtimes, bins=n_bins, color=COLORS["hist"], edgecolor=COLORS["hist_edge"],
            linewidth=0.4, alpha=0.85, label="All placements")
    ax.axvline(algo, color=COLORS["algo"], linewidth=2.5, linestyle="--",
               label=_algo_label(algo), zorder=5)
    ax.axvline(bf_min, color=COLORS["bar_min"], linewidth=1.8, linestyle=":",
               label=f"Brute-force min = {bf_min:.0f}", zorder=4)

    ax.set_xlabel("Runtime (units, × 10⁻⁴ s)", fontsize=9)
    ax.set_ylabel("Number of placements", fontsize=9)
    ax.set_title(
        f"Row 2: 5-bit error corr → trans-crotonic acid\n"
        f"All P({meta['nPhys']},{meta['nLog']}) = {meta['searchSpace']:,} placements enumerated",
        fontsize=9, fontweight="bold"
    )

    proof_text = (
        f"Algorithm result = {algo:.0f}\n"
        f"Brute-force min  = {bf_min:.0f}\n"
        f"Beats {rank:.1f}% of all placements"
    )
    ax.text(0.98, 0.95, proof_text, transform=ax.transAxes,
            fontsize=8, va="top", ha="right",
            bbox=dict(boxstyle="round,pad=0.3", fc="lightyellow", ec="orange", alpha=0.9))

    ax.legend(fontsize=7, loc="upper left")

# --- Row 3: histogram (100 000 sampled placements) ---

def plot_row3(ax, runtimes, algo, meta):
    smp_min = np.min(runtimes)
    rank    = percentile_rank(runtimes, algo)
    ss      = meta["searchSpace"]

    n_bins = min(80, len(np.unique(runtimes)))
    ax.hist(runtimes, bins=n_bins, color=COLORS["hist"], edgecolor=COLORS["hist_edge"],
            linewidth=0.4, alpha=0.85, label=f"{meta['nSamples']:,} random samples")
    ax.axvline(algo, color=COLORS["algo"], linewidth=2.5, linestyle="--",
               label=_algo_label(algo), zorder=5)
    ax.axvline(smp_min, color=COLORS["bar_min"], linewidth=1.8, linestyle=":",
               label=f"Sample min = {smp_min:.0f} (single-placement†)", zorder=4)

    ax.set_xlabel("Runtime (units, × 10⁻⁴ s)", fontsize=9)
    ax.set_ylabel("Number of placements", fontsize=9)
    ax.set_title(
        f"Row 3: pseudo-cat state → histidine\n"
        f"P({meta['nPhys']},{meta['nLog']}) = {ss:,}  "
        f"({meta['nSamples']:,} random samples shown)",
        fontsize=9, fontweight="bold"
    )

    proof_text = (
        f"Algorithm result = {algo:.0f}  ({algo/10000:.4f} s)\n"
        f"Sample min†      = {smp_min:.0f}  (no SWAP cost)\n"
        f"Beats {rank:.1f}% of {meta['nSamples']:,} samples\n"
        f"Gap = heuristic limit (100 candidates)"
    )
    ax.text(0.98, 0.95, proof_text, transform=ax.transAxes,
            fontsize=8, va="top", ha="right",
            bbox=dict(boxstyle="round,pad=0.3", fc="lightyellow", ec="orange", alpha=0.9))

    # Footnote inside the axes
    ax.text(0.02, 0.01,
            "† Single fixed-placement baseline (no SWAP transitions).\n"
            "  Algorithm uses subcircuit decomposition with fast-interaction\n"
            "  constraint; heuristic evaluates 100 candidate monomorphisms.",
            transform=ax.transAxes, fontsize=6.5, va="bottom", ha="left",
            color="#555555",
            bbox=dict(boxstyle="round,pad=0.3", fc="white", ec="#cccccc", alpha=0.85))

    ax.legend(fontsize=7, loc="upper left")

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    out_dir = os.path.join("figures", "output")
    os.makedirs(out_dir, exist_ok=True)

    summary = load_summary()
    meta = {r["row"]: r for r in summary}

    rt1 = load_runtimes("brute_force_data/row1.csv")
    rt2 = load_runtimes("brute_force_data/row2.csv")
    rt3 = load_runtimes("brute_force_data/row3.csv")

    fig, axes = plt.subplots(1, 3, figsize=(17, 5.5))
    fig.suptitle(
        "Table II: Algorithm Finds Minimum Runtime Over Search Space\n"
        "(Brute-force = fixed single-placement, no SWAP;  "
        "Algorithm = subcircuit decomposition + optimal per-subcircuit placement)",
        fontsize=10, y=1.02
    )

    plot_row1(axes[0], rt1, meta[1]["algoResult"], meta[1])
    plot_row2(axes[1], rt2, meta[2]["algoResult"], meta[2])
    plot_row3(axes[2], rt3, meta[3]["algoResult"], meta[3])

    fig.tight_layout(pad=1.5)

    out_path = os.path.join(out_dir, "search_space_proof.png")
    fig.savefig(out_path, dpi=150, bbox_inches="tight")
    print(f"Saved: {out_path}")

    # Print proof summary to console
    print("\n=== Proof of Correctness Summary ===")
    for r in summary:
        algo = r["algoResult"]
        bmin = r["bruteMin"]
        sp   = r["searchSpace"]
        sampled = "(sampled)" if r["isSampled"] else "(exhaustive)"
        status = "EXACT MIN" if np.isclose(algo, bmin, rtol=1e-6) else f"beats {100*sum(1 for x in (rt1 if r['row']==1 else rt2 if r['row']==2 else rt3) if x >= algo) / len(rt1 if r['row']==1 else rt2 if r['row']==2 else rt3):.1f}%"
        print(f"  Row {r['row']}: algo={algo:.1f}  BF_min={bmin:.1f}  "
              f"search_space={sp:,} {sampled}  -> {status}")

if __name__ == "__main__":
    main()
