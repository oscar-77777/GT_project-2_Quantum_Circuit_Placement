#include "physical_env.h"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <algorithm>

PhysicalEnvironment::PhysicalEnvironment(int numNuclei)
    : nNuclei_(numNuclei),
      W_(numNuclei, std::vector<Weight>(numNuclei, 0.0))
{}

void PhysicalEnvironment::setTwoQubitWeight(NucleusID u, NucleusID v, Weight w) {
    W_[u][v] = W_[v][u] = w;
}

void PhysicalEnvironment::setSingleQubitWeight(NucleusID u, Weight w) {
    W_[u][u] = w;
}

int PhysicalEnvironment::numNuclei() const { return nNuclei_; }

Weight PhysicalEnvironment::twoQubitWeight(NucleusID u, NucleusID v) const {
    return W_[u][v];
}

Weight PhysicalEnvironment::singleQubitWeight(NucleusID u) const {
    return W_[u][u];
}

Weight PhysicalEnvironment::gateOperatingTime(Weight baseTime, NucleusID n1, NucleusID n2) const {
    Weight w = (n2 == UNASSIGNED) ? singleQubitWeight(n1) : twoQubitWeight(n1, n2);
    return w * baseTime;
}

std::vector<std::pair<NucleusID, NucleusID>> PhysicalEnvironment::fastEdges(Weight threshold) const {
    std::vector<std::pair<NucleusID, NucleusID>> edges;
    for (int u = 0; u < nNuclei_; ++u)
        for (int v = u + 1; v < nNuclei_; ++v)
            if (W_[u][v] > 0 && W_[u][v] <= threshold)
                edges.push_back({u, v});
    return edges;
}

std::vector<std::vector<NucleusID>> PhysicalEnvironment::fastAdjacency(Weight threshold) const {
    std::vector<std::vector<NucleusID>> adj(nNuclei_);
    for (int u = 0; u < nNuclei_; ++u)
        for (int v = 0; v < nNuclei_; ++v)
            if (u != v && W_[u][v] > 0 && W_[u][v] <= threshold)
                adj[u].push_back(v);
    return adj;
}

// File format (see data/environments/acetyl_chloride.env):
//   Line 1: n   (number of nuclei)
//   "single" section: "single u w"
//   "two"    section: "two u v w"
//   Lines starting with '#' are comments.
PhysicalEnvironment PhysicalEnvironment::fromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open environment file: " + path);

    int n;
    f >> n;
    PhysicalEnvironment env(n);

    std::string line;
    std::getline(f, line); // consume rest of first line
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string tag;
        ss >> tag;
        if (tag == "single") {
            int u; double w;
            ss >> u >> w;
            env.setSingleQubitWeight(u, w);
        } else if (tag == "two") {
            int u, v; double w;
            ss >> u >> v >> w;
            env.setTwoQubitWeight(u, v, w);
        }
    }
    return env;
}
