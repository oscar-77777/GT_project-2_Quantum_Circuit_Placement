#include "placement.h"
#include <stdexcept>
#include <unordered_set>

Placement::Placement(int numLogical, int numPhysical)
    : nLogical_(numLogical), nPhysical_(numPhysical),
      map_(numLogical, UNASSIGNED)
{}

void Placement::assign(QubitID logical, NucleusID physical) {
    map_[logical] = physical;
}

NucleusID Placement::get(QubitID logical) const {
    return map_[logical];
}

bool Placement::isAssigned(QubitID logical) const {
    return map_[logical] != UNASSIGNED;
}

bool Placement::isValid() const {
    std::unordered_set<NucleusID> used;
    for (NucleusID n : map_) {
        if (n == UNASSIGNED) continue;
        if (used.count(n)) return false;   // duplicate -> not injective
        used.insert(n);
    }
    return true;
}

void Placement::clear() {
    std::fill(map_.begin(), map_.end(), UNASSIGNED);
}

int Placement::numLogical()  const { return nLogical_; }
int Placement::numPhysical() const { return nPhysical_; }

// Builds perm[dest] = src such that applying perm moves values from `this`
// layout to `next` layout.
//
// Example: this has qubit 0 -> nucleus 2, next has qubit 0 -> nucleus 5.
// Means: value currently at nucleus 2 must end up at nucleus 5.
// So perm[5] = 2.
std::vector<int> Placement::permutationTo(const Placement& next) const {
    // Initialize to identity
    std::vector<int> perm(nPhysical_);
    for (int i = 0; i < nPhysical_; ++i) perm[i] = i;

    for (QubitID q = 0; q < nLogical_; ++q) {
        NucleusID src  = get(q);
        NucleusID dest = next.get(q);
        if (src == UNASSIGNED || dest == UNASSIGNED) continue;
        if (src != dest) perm[dest] = src;
    }
    return perm;
}
