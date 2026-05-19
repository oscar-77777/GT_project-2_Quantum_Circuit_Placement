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
inline Gate makeSingleGate(QubitID q, Weight t, int level) {
    return { GateType::Single, q, UNASSIGNED, t, level };
}

inline Gate makeTwoGate(QubitID q1, QubitID q2, Weight t, int level) {
    return { GateType::Two, q1, q2, t, level };
}
