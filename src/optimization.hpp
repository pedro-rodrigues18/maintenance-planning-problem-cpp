#ifndef OPTIMIZATION_HPP
#define OPTIMIZATION_HPP

#include "problem.hpp"
#include "gurobi.hpp"
#include "de.hpp"

const double TIME_LIMIT = 60.0 * 15.0;

class Optimization {
public:
    Problem* problem;

    Optimization(Problem* problem);

    vector<pair<string, int>> OptimizationStep();
    tuple<bool, float> ConstraintSatisfied(vector<int> start_times);
    tuple<float, float, float> ObjectiveFunction(vector<int> start_times, float penalty = 0.0);
    void PrintSolution(vector<pair<string, int>> solution);

private:
    tuple<bool, float> InterventionConstraint(vector<int> start_times);
    tuple<bool, float> ResourceConstraint(vector<int> start_times);
    tuple<bool, float> ExclusionConstraint(vector<int> start_times);
};

#endif