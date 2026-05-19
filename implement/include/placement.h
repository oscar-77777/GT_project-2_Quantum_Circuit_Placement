#pragma once
#include <vector>
#include "types.h"

// An injective mapping P : {q0,...,q_{n-1}} -> {nu_0,...,nu_{m-1}}
// mapping logical qubits to physical nuclei.
//
// This is the core data type shared between both implementation partners:
// - Partner A (Algorithm) produces a Placement per subcircuit.
// - Partner B (Permutation) reads consecutive Placements to build SWAP circuits.
class Placement {
public:
    Placement() = default;
    Placement(int numLogical, int numPhysical);

    // P(logical) = physical
    void      assign(QubitID logical, NucleusID physical);
    NucleusID get(QubitID logical)        const;   // returns UNASSIGNED if not mapped
    bool      isAssigned(QubitID logical) const;
    bool      isValid()                   const;   // checks injectivity over all assigned qubits

    void clear();

    int numLogical()  const;
    int numPhysical() const;

    // Derive the permutation that transforms THIS placement into `next`.
    //
    // Returns a vector `perm` of size numPhysical() where perm[dest] = src means
    // "the value currently at nucleus src must end up at nucleus dest."
    // Nuclei not involved in either placement map to themselves (identity).
    //
    // This is the input to PermutationRouter::route().
    std::vector<int> permutationTo(const Placement& next) const;

private:
    int nLogical_{0};
    int nPhysical_{0};
    std::vector<NucleusID> map_;   // map_[logical] = physical
};
