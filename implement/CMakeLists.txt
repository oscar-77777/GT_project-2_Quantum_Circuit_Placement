cmake_minimum_required(VERSION 3.16)
project(QuantumCircuitPlacement CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Shared library of common data structures (used by both partners)
add_library(qcp_common
    src/physical_env.cpp
    src/quantum_circuit.cpp
    src/placement.cpp
    src/swap_circuit.cpp
)
target_include_directories(qcp_common PUBLIC include)

# Partner A: subcircuit placement algorithm
add_library(qcp_algorithm
    src/algorithm/circuit_placer.cpp
)
target_include_directories(qcp_algorithm PUBLIC include)
target_link_libraries(qcp_algorithm PUBLIC qcp_common)

# Partner B: fast permutation / SWAP routing
add_library(qcp_permutation
    src/permutation/permutation_router.cpp
)
target_include_directories(qcp_permutation PUBLIC include)
target_link_libraries(qcp_permutation PUBLIC qcp_common)

# Main executable
add_executable(placer src/main.cpp)
target_link_libraries(placer PRIVATE qcp_algorithm qcp_permutation)
