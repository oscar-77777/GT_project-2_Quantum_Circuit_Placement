#pragma once
#include <cstdint>

// Logical qubit index (0-based), used in circuit description
using QubitID   = int;

// Physical qubit index (0-based), used in physical environment (molecule nucleus)
using NucleusID = int;

// Execution time cost (units: 1/10000 s, rounded to integer per paper conventions)
using Weight    = double;

// Invalid/unassigned sentinel
constexpr NucleusID UNASSIGNED = -1;
