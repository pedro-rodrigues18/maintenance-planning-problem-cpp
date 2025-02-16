#ifndef GUROBI_HPP
#define GUROBI_HPP

#include "gurobi_c++.h"
#include "gurobi_c.h"
#include "problem.hpp"
#include <map>
#include <algorithm>

class Gurobi {
public:
    Problem* problem;

    Gurobi(Problem* problem);

    vector<int> Optimize();
};

#endif