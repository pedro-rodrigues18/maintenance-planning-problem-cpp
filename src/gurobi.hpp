#ifndef GUROBI_HPP
#define GUROBI_HPP

#include "gurobi_c++.h"
#include "gurobi_c.h"
#include "optimization.hpp"
#include "problem.hpp"
#include <map>
#include <algorithm>

class Gurobi {
public:
    Problem* problem;

    Gurobi(Problem* problem);

    vector<int> Optimize(chrono::time_point<chrono::high_resolution_clock> start_time);
};

#endif