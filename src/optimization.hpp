#ifndef OPTIMIZATION_HPP
#define OPTIMIZATION_HPP

#include "problem.hpp"

class Optimization {
public:
    Problem* problem;

    Optimization(Problem* problem);

    vector<pair<string, int>> OptimizationStep();
    tuple<bool, float> ConstraintSatisfied(vector<int> start_times);
    tuple<float, float, float> ObjectiveFunction(vector<int> start_times, float penalty = 0.0);

private:
    tuple<bool, float> InterventionConstraint(vector<int> start_times);
    tuple<bool, float> ResourceConstraint(vector<int> start_times);
    tuple<bool, float> ExclusionConstraint(vector<int> start_times);

};

#endif