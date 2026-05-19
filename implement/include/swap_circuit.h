#pragma once
#include <vector>
#include <utility>
#include "types.h"

// One logic level: a set of non-intersecting SWAP operations on physical nuclei.
// All pairs in a single SwapLevel are disjoint (no nucleus appears twice).
using SwapLevel = std::vector<std::pair<NucleusID, NucleusID>>;

// A sequence of SWAP levels that realizes a permutation on physical nuclei.
// Output of PermutationRouter; inserted between consecutive subcircuit placements.
//
// Full circuit structure (paper Section V-A):
//   C1  E_{1,2}  C2  E_{2,3}  ...  E_{t-1,t}  Ct
// where each E_{i,i+1} is a SwapCircuit.
class SwapCircuit {
public:
    void             addLevel(SwapLevel level);

    int              numLevels()         const;
    const SwapLevel& getLevel(int i)     const;

    // Number of logic SWAP levels = primary depth metric (paper minimizes this).
    int depth() const { return numLevels(); }

    // Apply this SWAP circuit to a physical state vector.
    // state[i] = logical qubit currently residing at nucleus i.
    // Returns the updated state after all SWAP levels execute.
    std::vector<int> apply(std::vector<int> state) const;

private:
    std::vector<SwapLevel> levels_;
};
