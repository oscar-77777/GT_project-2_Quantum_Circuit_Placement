#pragma once
#include "types.h"

enum class GateType { Single, Two };

struct Gate {
    GateType type;
    QubitID  q1;     // always valid
    QubitID  q2;     // valid only when type == Two; UNASSIGNED otherwise
    Weight   time;   // T(G): base execution time (before physical mapping)
    int      level;  // circuit level index (0-based), gates at same level run in parallel
};

// Convenience constructors
// Single gate: one qubit gate
inline Gate makeSingleGate(QubitID q, Weight t, int level) {
    return { GateType::Single, q, UNASSIGNED, t, level };
}

// Two gate: two qubit gate
inline Gate makeTwoGate(QubitID q1, QubitID q2, Weight t, int level) {
    return { GateType::Two, q1, q2, t, level };
}
