"""
Algorithm flowcharts for Quantum Circuit Placement (Maslov et al. 2008).
Run from implement/: python figures/generate_flowcharts.py
Output: figures/output/flow_*.png
"""

import os
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

os.makedirs('figures/output', exist_ok=True)
plt.rcParams.update({'font.family': 'DejaVu Sans', 'font.size': 9.5, 'figure.dpi': 150})

C_TERM = '#1B5E20'
C_PROC = '#1565C0'
C_DECI = '#BF360C'
C_LOOP = '#880E4F'
C_DATA = '#4A148C'
C_SUB  = '#01579B'
C_OK   = '#2E7D32'
C_WARN = '#B71C1C'


def proc(ax, cx, cy, w, h, text, fc='#E3F2FD', ec=None, fs=9):
    ec = ec or C_PROC
    r = mpatches.FancyBboxPatch((cx-w/2, cy-h/2), w, h,
                                 boxstyle='round,pad=0.08',
                                 fc=fc, ec=ec, lw=1.8, zorder=2)
    ax.add_patch(r)
    ax.text(cx, cy, text, ha='center', va='center', fontsize=fs,
            fontweight='bold', multialignment='center', zorder=3)


def term(ax, cx, cy, w, h, text, fc='#E8F5E9', ec=None, fs=9):
    ec = ec or C_TERM
    r = mpatches.FancyBboxPatch((cx-w/2, cy-h/2), w, h,
                                 boxstyle='round,pad=0.22',
                                 fc=fc, ec=ec, lw=2.4, zorder=2)
    ax.add_patch(r)
    ax.text(cx, cy, text, ha='center', va='center', fontsize=fs,
            fontweight='bold', color=ec, multialignment='center', zorder=3)


def diam(ax, cx, cy, w, h, text, fc='#FBE9E7', ec=None, fs=8.5):
    ec = ec or C_DECI
    pts = np.array([[cx, cy+h/2], [cx+w/2, cy], [cx, cy-h/2], [cx-w/2, cy]])
    ax.add_patch(mpatches.Polygon(pts, closed=True, fc=fc, ec=ec, lw=1.8, zorder=2))
    ax.text(cx, cy, text, ha='center', va='center', fontsize=fs,
            fontweight='bold', multialignment='center', zorder=3)


def arr(ax, x0, y0, x1, y1, col='#333333', lw=1.6, label='', lpos='right'):
    ax.annotate('', xy=(x1, y1), xytext=(x0, y0),
                arrowprops=dict(arrowstyle='->', color=col, lw=lw,
                                connectionstyle='arc3,rad=0'))
    if label:
        mx, my = (x0+x1)/2, (y0+y1)/2
        dx = 0.12 if lpos == 'right' else -0.12
        ha = 'left' if lpos == 'right' else 'right'
        ax.text(mx+dx, my, label, ha=ha, va='center', fontsize=8,
                color=col, fontstyle='italic')


def hline(ax, x0, x1, y, col='#333333', lw=1.6):
    ax.plot([x0, x1], [y, y], color=col, lw=lw, zorder=1)


def vline(ax, x, y0, y1, col='#333333', lw=1.6):
    ax.plot([x, x], [y0, y1], color=col, lw=lw, zorder=1)


# ─────────────────────────────────────────────────────────────
# Flow 1  Overall Pipeline
# ─────────────────────────────────────────────────────────────
def flow1_pipeline():
    fig, ax = plt.subplots(figsize=(8, 13))
    ax.set_xlim(0, 8); ax.set_ylim(0, 13); ax.axis('off')
    ax.set_title('Flowchart 1 - Overall Heuristic Pipeline', fontsize=12, pad=8)

    cx, W, H = 4.0, 4.2, 0.70
    DW, DH   = 3.8, 0.85

    # ── nodes ──
    term(ax, cx, 12.5, W, 0.55, 'START   Input: Circuit C,  Environment E,  Threshold tau')
    arr(ax, cx, 12.22, cx, 11.7)

    proc(ax, cx, 11.32, W, H,
         'Compute fast interaction graph F\n{ (u,v)  |  W(u,v) <= tau }',
         '#E3F2FD', C_PROC)
    arr(ax, cx, 10.97, cx, 10.45)

    proc(ax, cx, 10.07, W, H, 'startGate = 0', '#E3F2FD', C_PROC)
    arr(ax, cx, 9.72, cx, 9.15)

    # decision
    diam(ax, cx, 8.72, DW, DH, 'startGate < total gates?')
    arr(ax, cx, 8.29, cx, 7.75, C_OK, label='Yes', lpos='right')

    proc(ax, cx, 7.35, W, H,
         'basicPlacement(C, startGate)\n=> endGate,  Placement P',
         '#DCEDC8', C_OK)
    arr(ax, cx, 6.99, cx, 6.45)

    proc(ax, cx, 6.05, W, H,
         'sub = subcircuit(startGate, endGate)\nfineTuning(sub, P)  [depth-2 lookahead]',
         '#E8EAF6', '#283593')
    arr(ax, cx, 5.69, cx, 5.15)

    proc(ax, cx, 4.75, W, H,
         'Record (sub_i, P_i)\nstartGate = endGate',
         '#FFF9C4', '#F57F17')

    # loop-back arrow (left side)
    vline(ax, 1.0, 4.40, 8.72, C_LOOP, 1.8)
    hline(ax, 1.0, cx-W/2, 4.40, C_LOOP, 1.8)
    hline(ax, 1.0, cx-W/2, 8.72, C_LOOP, 1.8)
    ax.text(0.55, 6.6, 'repeat', fontsize=8.5, color=C_LOOP,
            fontstyle='italic', rotation=90, va='center')

    # NO branch (right side, then down)
    hline(ax, cx+DW/2, 7.2, 8.72, C_WARN, 1.8)
    vline(ax, 7.2, 3.55, 8.72, C_WARN, 1.8)
    ax.text(6.0, 8.80, 'No', fontsize=8, color=C_WARN, fontstyle='italic')
    ax.annotate('', xy=(cx+W/2+0.05, 3.55), xytext=(7.2, 3.55),
                arrowprops=dict(arrowstyle='->', color=C_WARN, lw=1.8,
                                connectionstyle='arc3,rad=0'))

    arr(ax, cx, 4.40, cx, 3.85)

    proc(ax, cx, 3.45, W, H,
         'For consecutive (P_i, P_{i+1}):\n  routeSubgraph  =>  SWAP circuit E_i',
         '#F3E5F5', '#6A1B9A')
    arr(ax, cx, 3.09, cx, 2.55)

    proc(ax, cx, 2.15, W, H,
         'total runtime  =  sum runtime(C_i, P_i)\n'
         '               +  sum depth(E_i) x swapCost',
         '#E8F5E9', C_OK)
    arr(ax, cx, 1.79, cx, 1.25)

    term(ax, cx, 0.95, W, 0.50, 'END   Output: runtime estimate + SWAP-free schedule')

    plt.tight_layout()
    fig.savefig('figures/output/flow1_pipeline.png', bbox_inches='tight')
    plt.close(fig)
    print('saved flow1_pipeline.png')


# ─────────────────────────────────────────────────────────────
# Flow 2  basicPlacement
# ─────────────────────────────────────────────────────────────
def flow2_basic_placement():
    fig, ax = plt.subplots(figsize=(9, 13))
    ax.set_xlim(0, 9); ax.set_ylim(0, 13); ax.axis('off')
    ax.set_title('Flowchart 2 - basicPlacement (Stage 1: Greedy Subcircuit Growth)',
                 fontsize=11, pad=8)

    cx, W, H  = 4.5, 4.6, 0.72
    DW, DH    = 4.0, 0.88
    RX        = 8.0   # right side x for skip arrows
    LX        = 0.9   # left side x for break / undo

    term(ax, cx, 12.45, W, 0.52, 'INPUT:  circuit C,  startGate,  fast graph F')
    arr(ax, cx, 12.18, cx, 11.65)

    proc(ax, cx, 11.25, W, H,
         'Init:  patternAdj = {},  bestMonos = []\n g = startGate',
         '#E3F2FD', C_PROC)
    arr(ax, cx, 10.89, cx, 10.35)

    diam(ax, cx, 9.90, DW, DH, 'g < numGates?')
    arr(ax, cx, 9.46, cx, 8.90, C_OK, label='Yes')

    diam(ax, cx, 8.45, DW, DH, 'gate[g]  is  two-qubit?')

    # No -> skip to g++ on right
    hline(ax, cx+DW/2, RX, 8.45, '#555', 1.5)
    ax.text(cx+DW/2+0.1, 8.55, 'No', fontsize=7.5, color='#555', fontstyle='italic')

    arr(ax, cx, 8.01, cx, 7.45, C_OK, label='Yes')

    diam(ax, cx, 6.98, DW, DH, 'edge (q1,q2)  already in pattern?')

    # Yes -> skip to g++ on right
    hline(ax, cx+DW/2, RX, 6.98, '#555', 1.5)
    ax.text(cx+DW/2+0.1, 7.08, 'Yes', fontsize=7.5, color='#555', fontstyle='italic')

    # Right-side g++ box
    proc(ax, RX, 7.7, 1.5, 0.58, 'g++', '#ECEFF1', '#546E7A', fs=9)
    vline(ax, RX, 6.98, 7.41, '#555', 1.5)
    vline(ax, RX, 8.0,  8.17, '#555', 1.5)
    # g++ arrow goes back up to top decision
    vline(ax, RX+0.55, 7.70, 9.90, '#555', 1.5)
    hline(ax, RX, RX+0.55, 7.70, '#555', 1.5)
    hline(ax, RX+0.55, cx+DW/2, 9.90, '#555', 1.5)

    arr(ax, cx, 6.54, cx, 5.98, C_OK, label='No')

    proc(ax, cx, 5.56, W, H,
         'Tentatively add edge (q1,q2) to patternAdj',
         '#E8EAF6', '#283593')
    arr(ax, cx, 5.20, cx, 4.64)

    proc(ax, cx, 4.22, W, H,
         'monos = findMonomorphisms(patternAdj, F)',
         '#E8EAF6', '#283593)'.replace(')', ''))
    arr(ax, cx, 3.86, cx, 3.30)

    diam(ax, cx, 2.84, DW, DH, 'monos  is  empty?')

    # Yes -> left: undo + break
    hline(ax, LX, cx-DW/2, 2.84, C_WARN, 1.5)
    ax.text(cx-DW/2-0.12, 2.95, 'Yes', fontsize=7.5, color=C_WARN,
            fontstyle='italic', ha='right')
    proc(ax, LX, 2.84, 1.55, 0.62,
         'Undo edge\nendGate = g\nBREAK',
         '#FFEBEE', C_WARN, fs=8.5)

    # No -> down
    arr(ax, cx, 2.40, cx, 1.82, C_OK, label='No')
    proc(ax, cx, 1.40, W, H,
         'bestMonos = monos ;  g++',
         '#DCEDC8', C_OK)

    # g++ loops back to top decision (left side loop)
    vline(ax, 0.45, 1.40, 9.90, '#888', 1.4)
    hline(ax, 0.45, cx-W/2, 1.40, '#888', 1.4)
    hline(ax, 0.45, cx-DW/2, 9.90, '#888', 1.4)

    # Bottom: after loop ends (no more gates OR break)
    arr(ax, cx, 9.46, cx, 9.90)   # the "No" path of top decision
    hline(ax, cx, cx+DW/2+0.6, 9.90, C_WARN, 1.5)
    ax.text(cx+DW/2+0.05, 9.98, 'No', fontsize=7.5, color=C_WARN, fontstyle='italic')

    # "select best" is placed below undo box
    proc(ax, cx, 0.52, W, H,
         'Choose P = argmin runtime(sub, candidate)\nover all mappings in bestMonos',
         '#E8F5E9', C_OK)

    # arrow from break box down to select best
    ax.annotate('', xy=(cx-W/2, 0.52), xytext=(LX, 2.53),
                arrowprops=dict(arrowstyle='->', color=C_WARN, lw=1.4,
                                connectionstyle='arc3,rad=-0.25'))

    # "No gates" path from top decision
    vline(ax, cx+DW/2+0.6, 0.52, 9.90, C_WARN, 1.4)
    ax.annotate('', xy=(cx+W/2, 0.52), xytext=(cx+DW/2+0.6, 0.52),
                arrowprops=dict(arrowstyle='->', color=C_WARN, lw=1.4,
                                connectionstyle='arc3,rad=0'))

    term(ax, cx, -0.1, W, 0.45, 'RETURN  endGate,  Placement P')

    plt.tight_layout()
    fig.savefig('figures/output/flow2_basic_placement.png', bbox_inches='tight')
    plt.close(fig)
    print('saved flow2_basic_placement.png')


# ─────────────────────────────────────────────────────────────
# Flow 3  fineTuning  +  scoreplacement (two-panel)
# ─────────────────────────────────────────────────────────────
def flow3_fine_tuning():
    fig, (axL, axR) = plt.subplots(1, 2, figsize=(13, 10))
    fig.suptitle('Flowchart 3 - fineTuning (Hill-Climbing) with Depth-2 Look-Ahead',
                 fontsize=12, y=1.01)

    # ── LEFT: hill-climbing loop ──────────────────────────────
    ax = axL
    ax.set_xlim(0, 7); ax.set_ylim(0, 10); ax.axis('off')
    ax.set_title('(a) fineTuning: Hill-Climbing Loop', fontsize=10, pad=6)

    cx, W, H = 3.5, 5.0, 0.72
    DW, DH   = 4.4, 0.85

    term(ax, cx, 9.45, W, 0.52,
         'INPUT: sub,  placement P,\nfullCircuit,  nextStart')
    arr(ax, cx, 9.18, cx, 8.65)

    proc(ax, cx, 8.23, W, H, 'improved = True', '#E3F2FD', C_PROC)
    arr(ax, cx, 7.87, cx, 7.30)

    diam(ax, cx, 6.84, DW, DH, 'improved == True?')
    arr(ax, cx, 6.40, cx, 5.82, C_OK, label='Yes')

    proc(ax, cx, 5.40, W, H,
         'improved = False\ncurScore = scoreplacement(sub, P)',
         '#E3F2FD', C_PROC)
    arr(ax, cx, 5.04, cx, 4.48)

    proc(ax, cx, 4.06, W, H,
         'For each qubit qi,  for each nucleus nu:\n'
         '  if nu already used by another qubit: skip',
         '#E8EAF6', '#283593')
    arr(ax, cx, 3.70, cx, 3.14)

    proc(ax, cx, 2.72, W, H,
         'P.assign(qi, nu)  [tentative]\n'
         'sc = scoreplacement(sub, P)',
         '#E8EAF6', '#283593')
    arr(ax, cx, 2.36, cx, 1.78)

    diam(ax, cx, 1.33, DW, DH, 'sc  <  curScore?')

    # Yes -> accept
    arr(ax, cx-DW/2, 1.33, 0.7, 1.33, C_OK)
    proc(ax, 0.35, 1.33, 0.55, 0.58, '', '#DCEDC8', C_OK, fs=7)
    ax.text(0.35, 1.33, 'Accept\ncurScore=sc\nimproved=True',
            ha='center', va='center', fontsize=7.5,
            fontweight='bold', color=C_OK, zorder=5)
    # No -> revert
    arr(ax, cx+DW/2, 1.33, 6.55, 1.33, C_WARN)
    proc(ax, 6.72, 1.33, 0.55, 0.58, '', '#FFEBEE', C_WARN, fs=7)
    ax.text(6.72, 1.33, 'Revert\nP.assign(qi,\n orig)',
            ha='center', va='center', fontsize=7.5,
            fontweight='bold', color=C_WARN, zorder=5)
    ax.text(cx-DW/2-0.1, 1.43, 'Yes', fontsize=7.5, color=C_OK,
            fontstyle='italic', ha='right')
    ax.text(cx+DW/2+0.05, 1.43, 'No', fontsize=7.5, color=C_WARN, fontstyle='italic')

    # loop back to outer "improved?" check
    vline(ax, 0.18, 1.33, 6.84, C_LOOP, 1.5)
    hline(ax, 0.18, cx-DW/2, 6.84, C_LOOP, 1.5)
    ax.text(0.0, 4.0, 'retry', fontsize=7.5, color=C_LOOP,
            fontstyle='italic', rotation=90, va='center')

    # No from improved? -> end
    hline(ax, cx+DW/2, 6.5, 6.84, C_WARN, 1.5)
    ax.annotate('', xy=(6.5, 0.35), xytext=(6.5, 6.84),
                arrowprops=dict(arrowstyle='->', color=C_WARN, lw=1.5,
                                connectionstyle='arc3,rad=0'))
    ax.text(cx+DW/2+0.05, 6.93, 'No', fontsize=7.5, color=C_WARN, fontstyle='italic')

    term(ax, cx, 0.22, W, 0.45, 'RETURN  optimized Placement P')

    # ── RIGHT: scoreplacement breakdown ──────────────────────
    ax = axR
    ax.set_xlim(0, 7); ax.set_ylim(0, 10); ax.axis('off')
    ax.set_title('(b) scoreplacement: Score Computation', fontsize=10, pad=6)

    cx, W, H = 3.5, 5.0, 0.72
    DW, DH   = 4.4, 0.85

    term(ax, cx, 9.45, W, 0.52,
         'INPUT: sub,  P,  fullCircuit,  nextStart')
    arr(ax, cx, 9.18, cx, 8.65)

    proc(ax, cx, 8.23, W, H,
         'score = sub.computeRuntime(P, env)\n'
         '[primary term: actual subcircuit runtime]',
         '#E8F5E9', C_OK)
    arr(ax, cx, 7.87, cx, 7.30)

    diam(ax, cx, 6.84, DW, DH, 'fullCircuit  is  not null?')

    # No -> skip lookahead
    hline(ax, cx+DW/2, 6.4, 6.84, C_WARN, 1.5)
    ax.text(cx+DW/2+0.05, 6.93, 'No', fontsize=7.5, color=C_WARN, fontstyle='italic')

    arr(ax, cx, 6.40, cx, 5.82, C_OK, label='Yes')

    proc(ax, cx, 5.40, W, H,
         'lookaheadCount = 0\ng = nextStart',
         '#E3F2FD', C_PROC)
    arr(ax, cx, 5.04, cx, 4.48)

    diam(ax, cx, 4.02, DW, DH, 'g < total  AND  count < 2?')
    arr(ax, cx, 3.58, cx, 3.00, C_OK, label='Yes')

    proc(ax, cx, 2.58, W, H,
         'if gate[g] is two-qubit:\n'
         '  n1,n2 = mapped nuclei;  w = W(n1,n2)\n'
         '  if w > tau:  score += 0.05 * w * T\n'
         '  lookaheadCount++\ng++',
         '#FFF9C4', '#F57F17')

    # loop back
    vline(ax, 0.3, 2.58, 4.02, '#F57F17', 1.4)
    hline(ax, 0.3, cx-DW/2, 4.02, '#F57F17', 1.4)
    hline(ax, 0.3, cx-W/2, 2.58, '#F57F17', 1.4)

    # No -> fall through to return
    hline(ax, cx+DW/2, 6.4, 4.02, C_WARN, 1.5)
    ax.text(cx+DW/2+0.05, 4.11, 'No', fontsize=7.5, color=C_WARN, fontstyle='italic')
    ax.annotate('', xy=(6.4, 1.15), xytext=(6.4, 4.02),
                arrowprops=dict(arrowstyle='->', color=C_WARN, lw=1.5,
                                connectionstyle='arc3,rad=0'))
    ax.annotate('', xy=(cx+W/2, 1.15), xytext=(6.4, 1.15),
                arrowprops=dict(arrowstyle='->', color=C_WARN, lw=1.5,
                                connectionstyle='arc3,rad=0'))

    arr(ax, cx, 2.22, cx, 1.50)

    proc(ax, cx, 1.08, W, H,
         'score = primary_runtime  +  lookahead_penalty\n'
         '[penalty is ~0-5% of score: tiebreaker only]',
         '#E8EAF6', '#283593')
    arr(ax, cx, 0.72, cx, 0.22)

    term(ax, cx, 0.08, W, 0.38, 'RETURN  score')

    plt.tight_layout()
    fig.savefig('figures/output/flow3_fine_tuning.png', bbox_inches='tight')
    plt.close(fig)
    print('saved flow3_fine_tuning.png')


# ─────────────────────────────────────────────────────────────
# Flow 4  routeSubgraph (divide-and-conquer SWAP)
# ─────────────────────────────────────────────────────────────
def flow4_router():
    fig, ax = plt.subplots(figsize=(10, 12))
    ax.set_xlim(0, 10); ax.set_ylim(0, 12); ax.axis('off')
    ax.set_title('Flowchart 4 - routeSubgraph: Divide-and-Conquer SWAP Routing',
                 fontsize=11, pad=8)

    cx, W, H = 5.0, 5.8, 0.72
    DW, DH   = 5.2, 0.88

    term(ax, cx, 11.45, W, 0.52,
         'INPUT: physical graph G,  state[],  target[]\n'
         '(state[n] = which qubit value sits at nucleus n)')
    arr(ax, cx, 11.18, cx, 10.62)

    diam(ax, cx, 10.18, DW, DH, '|G| == 1?  (base case)')
    arr(ax, cx, 9.74, cx, 9.20, C_OK, label='No')

    # base case on right
    hline(ax, cx+DW/2, 9.0, 10.18, C_TERM, 1.5)
    ax.text(cx+DW/2+0.05, 10.27, 'Yes', fontsize=7.5, color=C_TERM, fontstyle='italic')
    proc(ax, 9.4, 10.18, 1.1, 0.55,
         'Base:\nreturn []', '#E8F5E9', C_TERM, fs=8)

    proc(ax, cx, 8.78, W, H,
         'partition(G)  using BFS spanning tree\n'
         '=> G1,  G2,  channel edge (u in G1, v in G2)',
         '#E3F2FD', C_PROC)
    arr(ax, cx, 8.42, cx, 7.88)

    # Phase A block
    proc(ax, cx, 7.46, W, 0.78,
         'PHASE A  -  Bubble Propagation\n'
         'For each leaf c of G1 (c != u):\n'
         '  if state[c] == target[c]:  skip  [leaf-target override]\n'
         '  else:  bubble target[c] from c toward u  (SWAP chain in G1)',
         '#E8EAF6', '#283593', fs=8.5)

    arr(ax, cx, 7.07, cx, 6.50)

    proc(ax, cx, 6.08, W, H,
         'Channel SWAP: SWAP(u, v)\n'
         '[moves qubit value across G1/G2 boundary]',
         '#F3E5F5', '#6A1B9A')

    arr(ax, cx, 5.72, cx, 5.18)

    proc(ax, cx, 4.76, W, 0.78,
         'For each leaf c of G2 (c != v):\n'
         '  if state[c] == target[c]:  skip  [leaf-target override]\n'
         '  else:  bubble target[c] from c toward v  (SWAP chain in G2)',
         '#E8EAF6', '#283593', fs=8.5)

    arr(ax, cx, 4.37, cx, 3.83)

    # Phase B split
    proc(ax, cx, 3.41, W, H,
         'PHASE B  -  Parallel Recursion\n'
         'After Phase A:  G1 and G2 are independent',
         '#FFF9C4', '#F57F17')

    # split arrows
    ax.annotate('', xy=(2.5, 2.55), xytext=(cx, 3.05),
                arrowprops=dict(arrowstyle='->', color='#F57F17', lw=1.8,
                                connectionstyle='arc3,rad=0.2'))
    ax.annotate('', xy=(7.5, 2.55), xytext=(cx, 3.05),
                arrowprops=dict(arrowstyle='->', color='#F57F17', lw=1.8,
                                connectionstyle='arc3,rad=-0.2'))

    proc(ax, 2.5, 2.15, 3.6, 0.72,
         'routeSubgraph(G1,\n  state, target)',
         '#DCEDC8', C_OK)
    proc(ax, 7.5, 2.15, 3.6, 0.72,
         'routeSubgraph(G2,\n  state, target)',
         '#DCEDC8', C_OK)

    # merge arrows
    ax.annotate('', xy=(cx, 1.18), xytext=(2.5, 1.79),
                arrowprops=dict(arrowstyle='->', color=C_OK, lw=1.8,
                                connectionstyle='arc3,rad=-0.2'))
    ax.annotate('', xy=(cx, 1.18), xytext=(7.5, 1.79),
                arrowprops=dict(arrowstyle='->', color=C_OK, lw=1.8,
                                connectionstyle='arc3,rad=0.2'))

    proc(ax, cx, 0.78, W, H,
         'Merge SWAP levels from G1 and G2  (parallel)\n'
         'Append all levels to output',
         '#E8F5E9', C_OK)
    arr(ax, cx, 0.42, cx, 0.04)

    term(ax, cx, -0.1, W-0.5, 0.38,
         'RETURN  list of SWAP levels  (each level = set of parallel SWAPs)')

    # annotation: leaf-target override explanation
    note_x, note_y = 9.2, 4.76
    r = mpatches.FancyBboxPatch((7.9, 4.25), 1.9, 1.02,
                                 boxstyle='round,pad=0.08',
                                 fc='#FFEBEE', ec=C_WARN, lw=1.4, zorder=2)
    ax.add_patch(r)
    ax.text(8.85, 4.76, 'Leaf-target\nOverride:\navoids unnecessary\nSWAP at boundary',
            ha='center', va='center', fontsize=7.5, color=C_WARN,
            fontweight='bold', zorder=3)
    ax.annotate('', xy=(cx+W/2, 5.07), xytext=(7.9, 4.88),
                arrowprops=dict(arrowstyle='->', color=C_WARN, lw=1.2,
                                connectionstyle='arc3,rad=-0.15'))

    plt.tight_layout()
    fig.savefig('figures/output/flow4_router.png', bbox_inches='tight')
    plt.close(fig)
    print('saved flow4_router.png')


# ─────────────────────────────────────────────────────────────
if __name__ == '__main__':
    flow1_pipeline()
    flow2_basic_placement()
    flow3_fine_tuning()
    flow4_router()
    print('All flowcharts saved to figures/output/')
