#include <iostream>
#include <iomanip>
#include <limits>
#include "circuit_placer.h"
#include "permutation_router.h"

// ---------------------------------------------------------------------------
// Physical environments built in-code (used for verifyExample3 only)
// All other environments are loaded from data/environments/*.env
// ---------------------------------------------------------------------------

// Acetyl chloride (paper Fig. 1): 3 nuclei M=0, C1=1, C2=2
// W values verified from Table I DP trace and Example 3:
//   W(M,C1)=38: confirmed by 90+38+8=136 (optimal runtime, Example 3)
static PhysicalEnvironment buildAcetylChloride() {
    PhysicalEnvironment env(3);
    env.setSingleQubitWeight(0, 8.0);    // M
    env.setSingleQubitWeight(1, 8.0);    // C1
    env.setSingleQubitWeight(2, 1.0);    // C2
    env.setTwoQubitWeight(0, 1,  38.0);  // M-C1  (J≈66 Hz)
    env.setTwoQubitWeight(0, 2, 672.0);  // M-C2  (J≈3.7 Hz)
    env.setTwoQubitWeight(1, 2,  89.0);  // C1-C2 (J≈28 Hz)
    return env;
}

// Error-correction encoding circuit (paper Fig. 2, 3 qubits, 9 gates).
// Reconstructed from Table I DP trace:
//   Y90_a → ZZ_ab → Y90_c → ZZ_bc → Y90_b  (5 non-zero T gates)
//   + 4 free Rz (T=0)
// Verified: suboptimal runtime=770, optimal=136 (paper Example 3).
static QuantumCircuit buildErrorCorrEncoding() {
    QuantumCircuit circ(3);
    circ.addGate(makeSingleGate(0, 1.0, 0));    // Ry(90) on a
    circ.addGate(makeTwoGate   (0, 1, 1.0, 1)); // ZZ(90) on a,b
    circ.addGate(makeSingleGate(2, 1.0, 2));    // Ry(90) on c
    circ.addGate(makeTwoGate   (1, 2, 1.0, 3)); // ZZ(90) on b,c
    circ.addGate(makeSingleGate(1, 1.0, 4));    // Ry(90) on b
    circ.addGate(makeSingleGate(0, 0.0, 5));    // Rz on a (free)
    circ.addGate(makeSingleGate(1, 0.0, 5));    // Rz on b (free)
    circ.addGate(makeSingleGate(2, 0.0, 5));    // Rz on c (free)
    circ.addGate(makeSingleGate(0, 0.0, 6));    // Rz on a (free)
    return circ;
}

// ---------------------------------------------------------------------------
// Helper: run full placement + SWAP routing, return runtime and subcircuit count
// ---------------------------------------------------------------------------

struct RunResult { double totalUnits; int subcircuitCount; };

// swapCost defaults to the minimum fast two-qubit weight in env
static RunResult runPlacement(const PhysicalEnvironment& env,
                               const QuantumCircuit& circ,
                               Weight threshold)
{
    // Find minimum two-qubit weight (fastest SWAP edge)
    Weight swapCost = std::numeric_limits<Weight>::max();
    for (int u = 0; u < env.numNuclei(); ++u)
        for (int v = u + 1; v < env.numNuclei(); ++v) {
            Weight w = env.twoQubitWeight(u, v);
            if (w > 0 && w < swapCost) swapCost = w;
        }
    if (swapCost == std::numeric_limits<Weight>::max()) swapCost = 1.0;

    CircuitPlacer   placer(env, threshold);
    PlacementResult result = placer.place(circ);

    PermutationRouter router(env, threshold);
    std::vector<SwapCircuit> swaps;
    for (int i = 0; i + 1 < static_cast<int>(result.placements.size()); ++i)
        swaps.push_back(router.routeBetween(result.placements[i], result.placements[i+1]));

    double total = placer.totalRuntime(result, swaps, swapCost);
    return { total, static_cast<int>(result.subcircuits.size()) };
}

// ---------------------------------------------------------------------------
// Verify Example 3 from paper (Section III)
// ---------------------------------------------------------------------------

static void verifyExample3() {
    std::cout << "\n=== VERIFY Example 3 (paper Section III) ===\n";
    PhysicalEnvironment env  = buildAcetylChloride();
    QuantumCircuit      circ = buildErrorCorrEncoding();

    Placement p(3, 3);
    p.assign(0, 2); p.assign(1, 1); p.assign(2, 0); // a->C2, b->C1, c->M (optimal)
    std::cout << "Optimal   a->C2,b->C1,c->M : " << circ.computeRuntime(p, env)
              << "  (target: 136)\n";

    Placement p2(3, 3);
    p2.assign(0, 0); p2.assign(1, 2); p2.assign(2, 1); // a->M, b->C2, c->C1
    std::cout << "Suboptimal a->M, b->C2,c->C1: " << circ.computeRuntime(p2, env)
              << "  (target: 770)\n";
}

// ---------------------------------------------------------------------------
// Table II: Mapping experimentally constructed circuits into their environments
// ---------------------------------------------------------------------------

static long long searchSpaceSize(int nLogical, int nPhysical) {
    long long r = 1;
    for (int k = nPhysical; k > nPhysical - nLogical; --k) r *= k;
    return r;
}

static void runTableII() {
    std::cout << "\n=== TABLE II: Mapping Circuits Into Their Physical Environment ===\n";

    // Column widths
    const int W1=32, W2=22, W3=20, W4=16;
    std::cout << std::left
              << std::setw(W1) << "Circuit"
              << std::setw(W2) << "Environment"
              << std::setw(W3) << "Est. runtime (s)"
              << std::setw(W4) << "Search space" << "\n"
              << std::string(W1+W2+W3+W4, '-') << "\n";

    // Use threshold=200 (gives 1 subcircuit for error-corr encoding, matching paper row 1)
    const Weight thr = 200.0;

    // Row 1: error correction encoding (3q) → acetyl chloride (3q)
    {
        PhysicalEnvironment env  = buildAcetylChloride();
        QuantumCircuit      circ = buildErrorCorrEncoding();
        RunResult r = runPlacement(env, circ, thr);
        std::cout << std::setw(W1) << "error corr. encoding [14]"
                  << std::setw(W2) << "acetyl chloride [14]"
                  << std::setw(W3) << std::fixed << std::setprecision(4) << r.totalUnits / 10000.0
                  << std::setw(W4) << searchSpaceSize(3, 3) << "\n";
        std::cout << std::string(W1, ' ') << "(target: 0.0136 sec, 1 subcircuit)\n";
    }

    // Row 2: 5-bit error correction (5q) → trans-crotonic acid (7q)
    {
        PhysicalEnvironment env  = PhysicalEnvironment::fromFile("data/environments/trans_crotonic_acid.env");
        QuantumCircuit      circ = QuantumCircuit::fromFile("data/circuits/five_bit_error_corr.circ");
        RunResult r = runPlacement(env, circ, thr);
        std::cout << std::setw(W1) << "5-bit error corr. [12]"
                  << std::setw(W2) << "trans-crotonic acid [12]"
                  << std::setw(W3) << std::fixed << std::setprecision(4) << r.totalUnits / 10000.0
                  << std::setw(W4) << searchSpaceSize(5, 7) << "\n";
        std::cout << std::string(W1, ' ') << "(target: 0.0779 sec)\n";
    }

    // Row 3: pseudo-cat state preparation (10q) → histidine (12q)
    {
        PhysicalEnvironment env  = PhysicalEnvironment::fromFile("data/environments/histidine.env");
        QuantumCircuit      circ = QuantumCircuit::fromFile("data/circuits/pseudo_cat_state.circ");
        RunResult r = runPlacement(env, circ, thr);
        std::cout << std::setw(W1) << "pseudo-cat state prep. [20]"
                  << std::setw(W2) << "histidine [20]"
                  << std::setw(W3) << std::fixed << std::setprecision(4) << r.totalUnits / 10000.0
                  << std::setw(W4) << searchSpaceSize(10, 12) << "\n";
        std::cout << std::string(W1, ' ') << "(target: 0.5170 sec)\n";
    }
}

// ---------------------------------------------------------------------------
// Table III: Placement with different Threshold values (paper Table III format)
// ---------------------------------------------------------------------------
// Format: each cell = "X.XXXX(N)" where N = number of subcircuits

static void printTableIIIHeader(const std::vector<Weight>& thresholds) {
    std::cout << std::left << std::setw(14) << "Circuit";
    for (Weight t : thresholds)
        std::cout << std::setw(13) << static_cast<int>(t);
    std::cout << "\n" << std::string(14 + 13 * static_cast<int>(thresholds.size()), '-') << "\n";
}

static void printTableIIIRow(const std::string& label,
                              const PhysicalEnvironment& env,
                              const QuantumCircuit& circ,
                              const std::vector<Weight>& thresholds)
{
    std::cout << std::left << std::setw(14) << label;
    for (Weight thr : thresholds) {
        RunResult r = runPlacement(env, circ, thr);
        double secs = r.totalUnits / 10000.0;
        // Format: X.XXXX(N), or N/A if graph disconnected and runtime == 0
        std::ostringstream cell;
        cell << std::fixed << std::setprecision(4) << secs
             << "(" << r.subcircuitCount << ")";
        std::cout << std::setw(13) << cell.str();
    }
    std::cout << "\n";
}

static void runTableIII() {
    const std::vector<Weight> thresholds = {50, 100, 200, 500, 1000, 10000};

    std::cout << "\n=== TABLE III: Placement with Different Threshold Values ===\n";
    std::cout << "(Format: estimated_runtime_sec(#subcircuits))\n";
    std::cout << "NOTE: environments use approximate J-coupling values;\n"
              << "      exact paper values require coupling matrices from refs [16][12].\n";

    // --- Block 1: 5-qubit BOC-(13C2-15N-2D2-glycine)-fluoride [16] ---
    {
        PhysicalEnvironment env  = PhysicalEnvironment::fromFile("data/environments/boc_glycine_fluoride.env");
        QuantumCircuit      circ = QuantumCircuit::fromFile("data/circuits/phaseest.circ");

        std::cout << "\nPlacement with the 5-qubit BOC-(13C2-15N-2D2-glycine)-fluoride molecule [16]\n";
        printTableIIIHeader(thresholds);
        printTableIIIRow("phaseest", env, circ, thresholds);

        std::cout << "(paper): ";
        const char* ref[] = {".9980(8)", ".9980(8)", ".8167(4)", ".8167(4)", ".4314(3)", ".5632(1)"};
        for (const char* v : ref) std::cout << std::setw(13) << v;
        std::cout << "\n";
    }

    // --- Block 2: 7-qubit trans-crotonic acid [12] ---
    {
        PhysicalEnvironment env  = PhysicalEnvironment::fromFile("data/environments/trans_crotonic_acid.env");
        QuantumCircuit      circ = QuantumCircuit::fromFile("data/circuits/phaseest.circ");

        std::cout << "\nPlacement with the 7-qubit trans-crotonic acid molecule [12]\n";
        printTableIIIHeader(thresholds);
        printTableIIIRow("phaseest", env, circ, thresholds);

        std::cout << "(paper): ";
        const char* ref[] = {".1636(7)", ".0699(4)", ".0699(4)", ".0700(3)", ".2156(2)", ".1812(1)"};
        for (const char* v : ref) std::cout << std::setw(13) << v;
        std::cout << "\n";
    }
}

// ---------------------------------------------------------------------------

int main() {
    verifyExample3();
    runTableII();
    try {
        runTableIII();
    } catch (const std::exception& e) {
        std::cerr << "\nERROR in runTableIII: " << e.what() << "\n";
    }
    return 0;
}
