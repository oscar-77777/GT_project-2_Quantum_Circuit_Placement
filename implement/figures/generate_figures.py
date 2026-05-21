"""
Report figures for the Quantum Circuit Placement heuristic.
Simplest example: 3-qubit error-correction encoding on acetyl chloride.

Run: python figures/generate_figures.py
Output: figures/output/fig1_*.png  …  fig5_*.png
"""

import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import matplotlib.lines  as mlines
import networkx as nx
import numpy as np

os.makedirs('figures/output', exist_ok=True)

plt.rcParams.update({
    'font.family': 'DejaVu Sans',
    'font.size':   11,
    'figure.dpi':  150,
})

C_FAST   = '#1565C0'   # blue  – fast edge
C_SLOW   = '#C62828'   # red   – slow edge
C_NPHYS  = '#FFF9C4'   # pale yellow – physical nucleus
C_NLOG   = '#E8F5E9'   # pale green  – logical qubit
C_GATE   = '#90CAF9'   # light blue  – gate (T=1)
C_FREE   = '#CFD8DC'   # grey        – free gate (T=0)
C_OK     = '#A5D6A7'   # green       – correct position
C_WRONG  = '#FFCC80'   # orange      – wrong position
THR      = 200


# ─────────────────────────────────────────────────────────────
# Fig 1 – Physical Environment (Acetyl Chloride)
# ─────────────────────────────────────────────────────────────
def fig1_physical_env():
    fig, ax = plt.subplots(figsize=(5.5, 5))
    ax.set_aspect('equal')
    ax.axis('off')
    ax.set_title('Fig 1 - Physical Environment: Acetyl Chloride\n'
                 f'(fast edge = W ≤ threshold = {THR})', fontsize=12, pad=14)

    npos = {'M\n(0)': (0.50, 0.82),
            'C1\n(1)': (0.12, 0.18),
            'C2\n(2)': (0.88, 0.18)}
    W1 = {'M\n(0)': 8, 'C1\n(1)': 8, 'C2\n(2)': 1}

    edges = [('M\n(0)',  'C1\n(1)',  38,  True),
             ('C1\n(1)', 'C2\n(2)', 89,  True),
             ('M\n(0)',  'C2\n(2)', 672, False)]

    for u, v, w, fast in edges:
        x0, y0 = npos[u];  x1, y1 = npos[v]
        col = C_FAST if fast else C_SLOW
        ls  = '-'  if fast else '--'
        lw  = 2.5  if fast else 1.5
        ax.plot([x0, x1], [y0, y1], color=col, lw=lw, ls=ls, zorder=1)
        # edge label, offset perpendicular to edge
        dx, dy = x1 - x0, y1 - y0
        L = (dx**2 + dy**2)**0.5
        ox, oy = -dy/L * 0.09, dx/L * 0.09
        mx, my = (x0+x1)/2 + ox, (y0+y1)/2 + oy
        ax.text(mx, my, f'W = {w}', ha='center', va='center', fontsize=10,
                color=col, fontweight='bold',
                bbox=dict(boxstyle='round,pad=0.2', fc='white', ec='none', alpha=0.85))

    R = 0.095
    for label, (x, y) in npos.items():
        name = label.split('\n')[0]
        idx  = label.split('\n')[1]
        circ = plt.Circle((x, y), R, color=C_NPHYS, ec='#333', lw=2, zorder=2)
        ax.add_patch(circ)
        ax.text(x, y + 0.015, name, ha='center', va='center',
                fontsize=12, fontweight='bold', zorder=3)
        ax.text(x, y - 0.035, f'W(u,u)={W1[label]}', ha='center', va='center',
                fontsize=8.5, color='#444', zorder=3)

    ax.set_xlim(0, 1); ax.set_ylim(0.05, 1.0)
    leg = [mlines.Line2D([], [], color=C_FAST, lw=2.5,
                         label=f'Fast edge  W ≤ {THR}'),
           mlines.Line2D([], [], color=C_SLOW, lw=1.5, ls='--',
                         label=f'Slow edge  W > {THR}')]
    ax.legend(handles=leg, loc='upper left', fontsize=9.5, framealpha=0.9)

    plt.tight_layout()
    fig.savefig('figures/output/fig1_physical_env.png', bbox_inches='tight')
    plt.close(fig)
    print('saved fig1_physical_env.png')


# ─────────────────────────────────────────────────────────────
# Fig 2 – Quantum Circuit Diagram
# ─────────────────────────────────────────────────────────────
def fig2_circuit():
    fig, ax = plt.subplots(figsize=(10.5, 3.8))
    ax.set_xlim(-0.5, 9.0)
    ax.set_ylim(-0.7, 3.3)
    ax.axis('off')
    ax.set_title('Fig 2 - Error-Correction Encoding Circuit (3 qubits, 9 gates)',
                 fontsize=12, pad=10)

    q_y   = {0: 2.5, 1: 1.5, 2: 0.5}
    q_lbl = {0: 'qubit a', 1: 'qubit b', 2: 'qubit c'}

    # Qubit wires
    for q, y in q_y.items():
        ax.plot([0.15, 8.6], [y, y], color='#555', lw=1.4, zorder=1)
        ax.text(-0.05, y, q_lbl[q], ha='right', va='center', fontsize=10)

    def gate1(q, cx, lbl, T):
        y = q_y[q]
        col = C_GATE if T else C_FREE
        r = mpatches.FancyBboxPatch((cx-.32, y-.26), .64, .52,
                                     boxstyle='round,pad=0.06',
                                     fc=col, ec='#333', lw=1.5, zorder=2)
        ax.add_patch(r)
        ax.text(cx, y, lbl, ha='center', va='center', fontsize=9,
                fontweight='bold', zorder=3)

    def gate2(q1, q2, cx, lbl):
        y1, y2 = q_y[q1], q_y[q2]
        ax.plot([cx, cx], [min(y1,y2), max(y1,y2)], color='#333', lw=1.5, zorder=2)
        for y in [y1, y2]:
            c = plt.Circle((cx, y), .14, color=C_FAST, ec='#333', lw=1.5, zorder=3)
            ax.add_patch(c)
        ax.text(cx + .30, (y1+y2)/2, lbl, ha='left', va='center',
                fontsize=9, color='#0D47A1', fontweight='bold', zorder=3)

    # Gates (level x, type, params)
    gate1(0, 0.7,  'Ry(90°)', 1)   # L0
    gate2(0, 1, 1.7, 'ZZ(90°)')    # L1
    gate1(2, 2.7,  'Ry(90°)', 1)   # L2
    gate2(1, 2, 3.7, 'ZZ(90°)')    # L3
    gate1(1, 4.7,  'Ry(90°)', 1)   # L4
    gate1(0, 5.8,  'Rz', 0)        # L5
    gate1(1, 5.8,  'Rz', 0)
    gate1(2, 5.8,  'Rz', 0)
    gate1(0, 7.0,  'Rz', 0)        # L6

    # Level markers
    for lx, lname in [(0.7,'L0'),(1.7,'L1'),(2.7,'L2'),(3.7,'L3'),
                       (4.7,'L4'),(5.8,'L5'),(7.0,'L6')]:
        ax.text(lx, -0.5, lname, ha='center', fontsize=8.5, color='#777')

    leg = [mpatches.Patch(color=C_GATE, label='T = 1  (pulse takes time)'),
           mpatches.Patch(color=C_FREE, label='T = 0  (Rz, free gate)'),
           plt.scatter([], [], s=80, color=C_FAST, label='ZZ two-qubit gate')]
    ax.legend(handles=leg, loc='upper right', fontsize=9, framealpha=0.9)

    # Time DP trace annotation (optimal)
    ax.annotate('DP: time[a]=1→90→90\ntime[c]=8→128→128\ntime[b]=0→0→0→136\n→ runtime = 136',
                xy=(4.7, 1.5), xytext=(6.5, 2.9),
                fontsize=8.5, color='#2E7D32',
                arrowprops=dict(arrowstyle='->', color='#2E7D32', lw=1.2),
                bbox=dict(boxstyle='round,pad=0.3', fc='#E8F5E9', ec='#2E7D32', alpha=0.9))

    plt.tight_layout()
    fig.savefig('figures/output/fig2_circuit.png', bbox_inches='tight')
    plt.close(fig)
    print('saved fig2_circuit.png')


# ─────────────────────────────────────────────────────────────
# Fig 3 – Subgraph Monomorphism (Basic Placement)
# ─────────────────────────────────────────────────────────────
def fig3_monomorphism():
    fig, axes = plt.subplots(1, 3, figsize=(13, 4.5))
    fig.suptitle('Fig 3 - Basic Placement: Subgraph Monomorphism (threshold = 200)',
                 fontsize=12, y=1.01)

    # ── (a) Logical interaction graph ──
    ax = axes[0]
    ax.set_title('(a) Logical Interaction Graph\n(from ZZ gates)', fontsize=11)
    G = nx.Graph()
    G.add_nodes_from(['a', 'b', 'c'])
    G.add_edges_from([('a','b'), ('b','c')])
    pos = {'a': (0,0), 'b': (1,0), 'c': (2,0)}
    nx.draw_networkx_nodes(G, pos, ax=ax, node_color=C_NLOG, node_size=1600, edgecolors='#333', linewidths=2)
    nx.draw_networkx_labels(G, pos, ax=ax, font_size=13, font_weight='bold')
    nx.draw_networkx_edges(G, pos, ax=ax, edge_color='#333', width=2.5)
    nx.draw_networkx_edge_labels(G, pos, ax=ax, edge_labels={('a','b'):'ZZ', ('b','c'):'ZZ'}, font_size=10)
    ax.set_xlim(-0.5, 2.5); ax.set_ylim(-0.7, 0.7); ax.axis('off')
    ax.text(1, -0.55, 'Path graph  a – b – c', ha='center', fontsize=9.5, color='#555')

    # ── (b) Physical fast graph ──
    ax = axes[1]
    ax.set_title(f'(b) Fast Interaction Graph\n(only W ≤ {THR})', fontsize=11)
    Gp = nx.Graph()
    Gp.add_nodes_from(['M\n(0)', 'C1\n(1)', 'C2\n(2)'])
    Gp.add_edges_from([('M\n(0)','C1\n(1)'), ('C1\n(1)','C2\n(2)')])
    # Also show slow edge M-C2
    Gp.add_edge('M\n(0)', 'C2\n(2)')
    posp = {'M\n(0)': (0,0), 'C1\n(1)': (1,0), 'C2\n(2)': (2,0)}
    fast_e = [('M\n(0)','C1\n(1)'), ('C1\n(1)','C2\n(2)')]
    slow_e = [('M\n(0)','C2\n(2)')]
    nx.draw_networkx_nodes(Gp, posp, ax=ax, node_color=C_NPHYS, node_size=1600, edgecolors='#333', linewidths=2)
    nx.draw_networkx_labels(Gp, posp, ax=ax, font_size=10, font_weight='bold')
    nx.draw_networkx_edges(Gp, posp, ax=ax, edgelist=fast_e, edge_color=C_FAST, width=2.5)
    nx.draw_networkx_edges(Gp, posp, ax=ax, edgelist=slow_e, edge_color=C_SLOW,
                           width=1.5, style='dashed')
    nx.draw_networkx_edge_labels(Gp, posp, ax=ax,
        edge_labels={('M\n(0)','C1\n(1)'):'W=38', ('C1\n(1)','C2\n(2)'):'W=89',
                     ('M\n(0)','C2\n(2)'):'W=672'},
        font_size=9)
    ax.set_xlim(-0.5, 2.5); ax.set_ylim(-0.8, 0.7); ax.axis('off')
    leg = [mlines.Line2D([],[],color=C_FAST,lw=2.5,label='Fast (included)'),
           mlines.Line2D([],[],color=C_SLOW,lw=1.5,ls='--',label='Slow (excluded)')]
    ax.legend(handles=leg, loc='lower center', fontsize=9, bbox_to_anchor=(0.5,-0.22))

    # ── (c) Optimal monomorphism ──
    ax = axes[2]
    ax.set_title('(c) Optimal Monomorphism\nruntime = 136  (optimal)', fontsize=11)
    ax.set_xlim(0, 4); ax.set_ylim(-0.2, 3.5); ax.axis('off')

    log_items  = [('a', C_NLOG),  ('b', C_NLOG),  ('c', C_NLOG)]
    phys_items = [('C2(2)', C_NPHYS), ('C1(1)', C_NPHYS), ('M(0)', C_NPHYS)]
    lx, px = 0.7, 3.3
    ys = [2.8, 1.6, 0.4]
    colors_arrow = ['#1976D2', '#388E3C', '#F57C00']

    for i, ((ln, lc), (pn, pc)) in enumerate(zip(log_items, phys_items)):
        y = ys[i]
        for cx, nm, col in [(lx, ln, lc), (px, pn, pc)]:
            r = mpatches.FancyBboxPatch((cx-.42, y-.28), .84, .56,
                                         boxstyle='round,pad=0.06',
                                         fc=col, ec='#333', lw=1.8)
            ax.add_patch(r)
            ax.text(cx, y, nm, ha='center', va='center', fontsize=11, fontweight='bold')
        ax.annotate('', xy=(px-.42, y), xytext=(lx+.42, y),
                    arrowprops=dict(arrowstyle='->', color=colors_arrow[i], lw=2.2))

    ax.text(lx,   3.25, 'Logical', ha='center', fontsize=10, color='#333', fontstyle='italic')
    ax.text(px,   3.25, 'Physical', ha='center', fontsize=10, color='#333', fontstyle='italic')
    ax.text(2.0,  3.25, 'f( · )', ha='center', fontsize=10, color='#333')
    ax.text(2.0, -0.15, 'Search space: P(3,3) = 6 mappings\nBest: runtime = 136',
            ha='center', fontsize=9, color='#2E7D32',
            bbox=dict(boxstyle='round,pad=0.3', fc='#E8F5E9', ec='#2E7D32'))

    plt.tight_layout()
    fig.savefig('figures/output/fig3_monomorphism.png', bbox_inches='tight')
    plt.close(fig)
    print('saved fig3_monomorphism.png')


# ─────────────────────────────────────────────────────────────
# Fig 4 – Overall Heuristic Pipeline
# ─────────────────────────────────────────────────────────────
def fig4_pipeline():
    fig, ax = plt.subplots(figsize=(13, 6))
    ax.set_xlim(0, 13); ax.set_ylim(0, 6)
    ax.axis('off')
    ax.set_title('Fig 4 - Heuristic Solution: End-to-End Pipeline', fontsize=13, pad=10)

    def box(cx, cy, w, h, text, fc='#E3F2FD', ec='#1565C0', fs=10):
        r = mpatches.FancyBboxPatch((cx-w/2, cy-h/2), w, h,
                                     boxstyle='round,pad=0.18',
                                     fc=fc, ec=ec, lw=1.8)
        ax.add_patch(r)
        ax.text(cx, cy, text, ha='center', va='center', fontsize=fs,
                fontweight='bold', multialignment='center')

    def arr(x0, y0, x1, y1, col='#333', label='', rad=0.0):
        style = f'arc3,rad={rad}' if rad else 'arc3,rad=0'
        ax.annotate('', xy=(x1,y1), xytext=(x0,y0),
                    arrowprops=dict(arrowstyle='->', color=col, lw=1.8,
                                    connectionstyle=style))
        if label:
            mx, my = (x0+x1)/2, (y0+y1)/2 + 0.18
            ax.text(mx, my, label, ha='center', fontsize=8.5, color=col)

    # ── Top row: placement loop ──
    box(1.3,  4.8, 2.0, 0.9,  'Input\nCircuit + Env\n+ Threshold',  '#E8F5E9', '#2E7D32')
    arr(2.3, 4.8, 3.1, 4.8)
    box(4.1,  4.8, 1.9, 0.9,  'Basic Placement\n(subgraph\nmonomorphism)',  '#E3F2FD', '#1565C0')
    arr(5.05, 4.8, 5.85, 4.8)
    box(6.8,  4.8, 1.9, 0.9,  'Fine Tuning\n(hill-climbing\n+depth-2 lookahead)', '#E8EAF6', '#283593')
    arr(7.75, 4.8, 8.55, 4.8)
    box(9.5,  4.8, 1.9, 0.9,  'Subcircuit Cᵢ\n+ Placement Pᵢ',  '#FFF9C4', '#F57F17')
    arr(10.45,4.8, 11.25, 4.8)
    box(12.0, 4.8, 1.5, 0.9,  'More\ngates?\n→ repeat',  '#FFEBEE', '#B71C1C')

    # Loop back arrow
    ax.annotate('', xy=(3.5, 4.35), xytext=(11.6, 4.35),
                arrowprops=dict(arrowstyle='->', color='#B71C1C', lw=1.6,
                                connectionstyle='arc3,rad=0.35'))
    ax.text(7.5, 3.72, '← loop: startGate = endGate', ha='center', fontsize=9, color='#B71C1C')

    # ── Middle: handoff to router ──
    arr(9.5, 4.35, 9.5, 3.35, '#6A1B9A',
        'PlacementResult\n{subcircuits, placements}', rad=0.0)
    ax.text(9.5, 3.55, '', ha='center')

    box(6.5, 2.8, 2.4, 0.9,
        'Permutation Router\n(divide-and-conquer\nSWAP routing)',  '#F3E5F5', '#6A1B9A')
    arr(9.5, 3.35, 7.7, 2.8)

    arr(7.7, 2.8, 9.0, 2.8)
    box(9.9, 2.8, 1.6, 0.9,  'SWAP Circuits\nE₁₂, E₂₃, …',  '#FFF9C4', '#F57F17')

    arr(10.7, 2.8, 11.5, 2.8)
    box(12.2, 2.8, 1.4, 0.9,
        'Total\nRuntime',  '#E8F5E9', '#2E7D32')

    # Runtime formula
    ax.text(12.2, 1.9,
            'runtime = Σ runtime(Cᵢ, Pᵢ)\n         + Σ depth(Eᵢ) × swapCost',
            ha='center', va='center', fontsize=8.5,
            bbox=dict(boxstyle='round,pad=0.3', fc='#E8F5E9', ec='#2E7D32'))

    # ── Bottom: final circuit structure ──
    ax.text(0.6, 1.15, 'Final circuit:', ha='center', va='center',
            fontsize=11, fontweight='bold')
    segs = [('C₁','#90CAF9'), ('E₁₂','#FFB74D'), ('C₂','#90CAF9'),
            ('E₂₃','#FFB74D'), ('…','#BDBDBD'), ('Cₙ','#90CAF9')]
    for i, (lbl, col) in enumerate(segs):
        cx = 1.9 + i * 1.6
        r = mpatches.FancyBboxPatch((cx-.5, 0.7), 1.0, 0.9,
                                     boxstyle='round,pad=0.08',
                                     fc=col, ec='#333', lw=1.5)
        ax.add_patch(r)
        ax.text(cx, 1.15, lbl, ha='center', va='center', fontsize=12, fontweight='bold')

    leg = [mpatches.Patch(color='#90CAF9', label='Subcircuit Cᵢ  (optimally placed)'),
           mpatches.Patch(color='#FFB74D', label='SWAP circuit Eᵢ  (bridges placements)')]
    ax.legend(handles=leg, loc='lower right', fontsize=9, framealpha=0.9)

    plt.tight_layout()
    fig.savefig('figures/output/fig4_pipeline.png', bbox_inches='tight')
    plt.close(fig)
    print('saved fig4_pipeline.png')


# ─────────────────────────────────────────────────────────────
# Fig 5 – Permutation Routing (divide-and-conquer SWAP)
# ─────────────────────────────────────────────────────────────
def fig5_permutation():
    """
    Example: P1 = {a→M, b→C1, c→C2}  →  P2 = {a→C2, b→C1, c→M}
    Permutation (swap M and C2, C1 stays):
      state = [0,1,2], target = [2,1,0]
    Fast graph (path M–C1–C2).
    Partition: G1={M}, G2={C1,C2}, channel=(M,C1).
    SWAP sequence: L0:(C1,C2)  L1:(M,C1)  L2:(C1,C2)  → depth=3
    """
    fig = plt.figure(figsize=(13, 7))
    fig.suptitle('Fig 5 - Permutation Routing: Divide-and-Conquer SWAP Circuit\n'
                 'P₁ = {a→M, b→C1, c→C2}  →  P₂ = {a→C2, b→C1, c→M}',
                 fontsize=12, y=1.01)

    # ── Top half: state evolution ──
    gs = fig.add_gridspec(2, 4, hspace=0.6, wspace=0.4)
    ax_states = [fig.add_subplot(gs[0, i]) for i in range(4)]

    nuc = ['M(0)', 'C1(1)', 'C2(2)']
    val_lbl = ['val_a', 'val_b', 'val_c']   # "value a belongs to a"
    # val_a should end at C2, val_b at C1, val_c at M
    # target[0]=val_c(2), target[1]=val_b(1), target[2]=val_a(0)  → target=[2,1,0]
    target = [2, 1, 0]

    states = [
        ([0, 1, 2], 'Initial state\nstate = [val_a, val_b, val_c]',      None,        'white'),
        ([0, 2, 1], 'After L0: SWAP(C1,C2)\n(bubble val_c toward M)',    [(1, 2)],    '#FFE0B2'),
        ([2, 0, 1], 'After L1: SWAP(M,C1)\n(channel SWAP: val_a crosses)', [(0, 1)],  '#E1BEE7'),
        ([2, 1, 0], 'After L2: SWAP(C1,C2)\n(G2 recurse: sort C1,C2)',  [(1, 2)],    '#DCEDC8'),
    ]

    def draw_state_panel(ax, state, target, title, swap_edges, bg):
        ax.set_facecolor(bg if bg != 'white' else '#FAFAFA')
        ax.set_xlim(-0.3, 2.3); ax.set_ylim(-0.5, 2.2)
        ax.axis('off')
        ax.set_title(title, fontsize=8.5, pad=4)
        xs = [0, 1, 2]
        for i, x in enumerate(xs):
            # nucleus label
            ax.text(x, -0.35, nuc[i], ha='center', fontsize=8.5,
                    fontweight='bold', color='#444')
            val = state[i]
            correct = (val == target[i])
            col = C_OK if correct else C_WRONG
            c = plt.Circle((x, 0.9), 0.36, color=col, ec='#333', lw=1.8, zorder=2)
            ax.add_patch(c)
            ax.text(x, 0.9, val_lbl[val], ha='center', va='center',
                    fontsize=8.5, fontweight='bold', zorder=3)
        if swap_edges:
            for (i, j) in swap_edges:
                xi, xj = xs[i], xs[j]
                ax.annotate('', xy=(xj, 1.45), xytext=(xi, 1.45),
                            arrowprops=dict(arrowstyle='<->', color='#D32F2F', lw=2))
                ax.text((xi+xj)/2, 1.72, 'SWAP', ha='center', fontsize=8.5,
                        color='#D32F2F', fontweight='bold')

    for ax, (state, title, sw, bg) in zip(ax_states, states):
        draw_state_panel(ax, state, target, title, sw, bg)

    # Arrows between state panels
    for i in range(3):
        x_frac = 0.25*(i+1) - 0.01
        fig.add_artist(mpatches.FancyArrowPatch(
            (x_frac + 0.01, 0.77), (x_frac + 0.04, 0.77),
            transform=fig.transFigure,
            arrowstyle='->', color='#333', lw=1.5,
            mutation_scale=16))

    # ── Bottom half: algorithm structure ──
    ax_algo = fig.add_subplot(gs[1, :])
    ax_algo.set_xlim(0, 13); ax_algo.set_ylim(0, 3.2)
    ax_algo.axis('off')
    ax_algo.set_title('Algorithm Structure (divide-and-conquer)', fontsize=11, pad=6)

    def abox(ax, cx, cy, w, h, text, fc, ec='#333', fs=9):
        r = mpatches.FancyBboxPatch((cx-w/2, cy-h/2), w, h,
                                     boxstyle='round,pad=0.12',
                                     fc=fc, ec=ec, lw=1.5)
        ax.add_patch(r)
        ax.text(cx, cy, text, ha='center', va='center', fontsize=fs,
                fontweight='bold', multialignment='center')

    # Partition
    abox(ax_algo, 2.0, 2.6, 3.5, 0.75,
         'partition(M, C1, C2)\nG1={M}  G2={C1,C2}  channel=(M,C1)',
         '#E3F2FD', C_FAST, 9)

    # Phase A
    abox(ax_algo, 6.5, 2.6, 4.5, 0.75,
         'Phase A – bubble propagation  (2 SWAP levels)\nL0: SWAP(C1,C2)   L1: SWAP(M,C1)',
         '#F3E5F5', '#6A1B9A', 9)

    # Phase B
    abox(ax_algo, 3.0, 1.35, 3.2, 0.75,
         'Recurse G1={M}\n(size 1 → done)',
         '#E8F5E9', '#2E7D32', 9)
    abox(ax_algo, 7.5, 1.35, 3.2, 0.75,
         'Recurse G2={C1,C2}\nL2: SWAP(C1,C2)',
         '#FFF9C4', '#F57F17', 9)

    ax_algo.annotate('', xy=(3.0, 1.72), xytext=(5.0, 2.22),
                     arrowprops=dict(arrowstyle='->', color='#2E7D32', lw=1.5))
    ax_algo.annotate('', xy=(7.5, 1.72), xytext=(7.0, 2.22),
                     arrowprops=dict(arrowstyle='->', color='#F57F17', lw=1.5))

    # Leaf-target override note
    abox(ax_algo, 11.2, 2.6, 2.8, 0.75,
         'Leaf-target override:\nskip if state[leaf]=target[leaf]',
         '#FFEBEE', '#B71C1C', 8.5)

    # Result
    abox(ax_algo, 6.5, 0.5, 5.5, 0.65,
         'SWAP Circuit depth = 3  (linear in n, guaranteed by paper eq. 2)',
         '#E8F5E9', '#2E7D32', 9.5)

    plt.tight_layout()
    fig.savefig('figures/output/fig5_permutation.png', bbox_inches='tight')
    plt.close(fig)
    print('saved fig5_permutation.png')


# ─────────────────────────────────────────────────────────────
# Fig 6 – Fine Tuning Hill-Climbing (with Depth-2 Lookahead)
# ─────────────────────────────────────────────────────────────
def fig6_finetuning():
    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    fig.suptitle('Fig 6 - Fine Tuning: Hill-Climbing with Depth-2 Look-Ahead',
                 fontsize=12, y=1.01)

    # ── Left: hill-climbing trajectory ──
    ax = axes[0]
    ax.set_title('(a) Hill-Climbing Search Space\n(all 6 placements for 3 qubits → 3 nuclei)',
                 fontsize=10)

    placements = [
        ('a→M, b→C1, c→C2', 770, False),
        ('a→M, b→C2, c→C1', 680, False),
        ('a→C1, b→M, c→C2', 680, False),
        ('a→C1, b→C2, c→M', 136, True),   # ← OPTIMAL
        ('a→C2, b→M, c→C1', 136+38, False),
        ('a→C2, b→C1, c→M', 136, True),   # ← optimal (same runtime)
    ]
    labels   = [p[0] for p in placements]
    runtimes = [p[1] for p in placements]
    optimal  = [p[2] for p in placements]
    colors   = [C_OK if o else '#90CAF9' for o in optimal]

    y_pos = np.arange(len(labels))
    bars = ax.barh(y_pos, runtimes, color=colors, edgecolor='#333', linewidth=1.2, height=0.65)
    ax.set_yticks(y_pos)
    ax.set_yticklabels(labels, fontsize=9)
    ax.set_xlabel('Circuit Runtime (time units)', fontsize=10)
    ax.axvline(136, color='#2E7D32', lw=2, ls='--', label='Optimal = 136')
    for i, (bar, rt) in enumerate(zip(bars, runtimes)):
        ax.text(rt + 8, bar.get_y() + bar.get_height()/2, str(rt),
                va='center', fontsize=9, fontweight='bold')
    leg = [mpatches.Patch(color=C_OK, label='Optimal placement'),
           mpatches.Patch(color='#90CAF9', label='Sub-optimal'),
           mlines.Line2D([],[],color='#2E7D32',lw=2,ls='--',label='runtime = 136')]
    ax.legend(handles=leg, fontsize=8.5, loc='lower right')
    ax.set_xlim(0, 900)

    # ── Right: depth-2 lookahead concept ──
    ax = axes[1]
    ax.set_title('(b) Depth-2 Look-Ahead Scoring\nscore = runtime(Cᵢ) + 0.05 × Σ cost(next 2 gates)',
                 fontsize=10)
    ax.set_xlim(0, 5); ax.set_ylim(0, 5); ax.axis('off')

    def tbox(cx, cy, w, h, text, fc, ec='#333', fs=9.5):
        r = mpatches.FancyBboxPatch((cx-w/2, cy-h/2), w, h,
                                     boxstyle='round,pad=0.15',
                                     fc=fc, ec=ec, lw=1.5)
        ax.add_patch(r)
        ax.text(cx, cy, text, ha='center', va='center', fontsize=fs,
                fontweight='bold', multialignment='center')

    tbox(2.5, 4.4, 4.5, 0.7,
         'Candidate placement p  (during hill-climb)', '#E3F2FD', C_FAST)

    ax.annotate('', xy=(2.5, 3.85), xytext=(2.5, 4.05),
                arrowprops=dict(arrowstyle='->', color='#333', lw=1.5))

    tbox(1.2, 3.3, 2.0, 0.75,
         'runtime(Cᵢ, p)\n← primary term', '#E8F5E9', '#2E7D32')
    tbox(3.8, 3.3, 2.0, 0.75,
         '0.05 × cost(next 2\nZZ gates under p)\n← lookahead penalty', '#FFF9C4', '#F57F17')

    ax.text(2.5, 2.75, '+', ha='center', va='center', fontsize=18, fontweight='bold')

    tbox(2.5, 2.2, 3.5, 0.7,
         'score(p)  =  runtime  +  penalty', '#E8EAF6', '#283593')

    ax.annotate('', xy=(2.5, 1.75), xytext=(2.5, 1.85),
                arrowprops=dict(arrowstyle='->', color='#333', lw=1.5))

    tbox(2.5, 1.3, 4.2, 0.75,
         'Accept if score(p_new) < score(p_curr)\nRevert otherwise', '#FFEBEE', '#B71C1C')

    ax.text(2.5, 0.65,
            'Effect: tiebreaker that prefers placements\n'
            'requiring fewer SWAPs at subcircuit boundaries\n'
            '(paper Section V-C: 0–5% improvement)',
            ha='center', va='center', fontsize=8.5, color='#555',
            bbox=dict(boxstyle='round,pad=0.3', fc='#F5F5F5', ec='#BBB'))

    plt.tight_layout()
    fig.savefig('figures/output/fig6_finetuning.png', bbox_inches='tight')
    plt.close(fig)
    print('saved fig6_finetuning.png')


# ─────────────────────────────────────────────────────────────
if __name__ == '__main__':
    fig1_physical_env()
    fig2_circuit()
    fig3_monomorphism()
    fig4_pipeline()
    fig5_permutation()
    fig6_finetuning()
    print('\nAll figures saved to figures/output/')
