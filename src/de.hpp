#ifndef DIFFERENTIAL_EVOLUTION_HPP
#define DIFFERENTIAL_EVOLUTION_HPP

#include <iostream>
#include <algorithm>
#include <random>
#include <chrono>
#include <vector>
#include <functional>
#include <omp.h>
#include "problem.hpp"
#include "optimization.hpp"

using namespace std;

class DifferentialEvolution {
public:
    using ObjectiveFunc = function<tuple<float, float, float>(vector<int>, float)>;
    using ConstraintFunc = function<tuple<bool, float>(vector<int>)>;
    using GeneratePopulationFunc = function<vector<vector<int>>(size_t, vector<pair<int, int>>)>;
    ObjectiveFunc objective_func;
    ConstraintFunc constraint_func;
    Problem* problem;
    vector<vector<int>> population;
    int population_size = 50;
    vector<float> fitness;
    vector<pair<int, int>> bounds;
    float mutation_rate = 0.6235;
    float crossover_rate = 0.5763;

    DifferentialEvolution(ObjectiveFunc objective_func, ConstraintFunc constraint_func, Problem* problem, vector<int> gurobi_solution);

    vector<int> Optimize(chrono::time_point<chrono::high_resolution_clock> start_time);

private:
    vector<pair<int, int>> CreateBounds(vector<Intervention> interventions);
    vector<vector<int>> GeneratePopulation(size_t interventions_size);
};

#endif