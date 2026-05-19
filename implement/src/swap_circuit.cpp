#include "swap_circuit.h"
#include <stdexcept>

void SwapCircuit::addLevel(SwapLevel level) {
    levels_.push_back(std::move(level));
}

int SwapCircuit::numLevels() const {
    return static_cast<int>(levels_.size());
}

const SwapLevel& SwapCircuit::getLevel(int i) const {
    return levels_.at(i);
}

// Apply each SWAP level sequentially to state.
// A SWAP(u,v) exchanges state[u] and state[v].
std::vector<int> SwapCircuit::apply(std::vector<int> state) const {
    for (const SwapLevel& lvl : levels_)
        for (const auto& edge : lvl)
            std::swap(state[edge.first], state[edge.second]);
    return state;
}
