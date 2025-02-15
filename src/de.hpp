#ifndef DIFFERENTIAL_EVOLUTION_HPP
#define DIFFERENTIAL_EVOLUTION_HPP

#include <vector>
#include <functional>

using namespace std;

class DifferentialEvolution {
public:
    using ObjectiveFunc = function<tuple<float, float, float>(vector<int>, float)>;
    using ConstraintFunc = function<tuple<bool, float>(vector<int>)>;
    using GeneratePopulationFunc = function<vector<vector<int>>(size_t, vector<pair<int, int>>)>;
    ObjectiveFunc objective_func;
    ConstraintFunc constraint_func;
    GeneratePopulationFunc generate_population_func;
    vector<vector<int>> population;
    vector<float> fitness;
    vector<pair<int, int>> bounds;
    float mutation_rate = 0.6235;
    float crossover_rate = 0.5763;
    //chrono::time_point<chrono::high_resolution_clock> start_time;

    DifferentialEvolution(
        ObjectiveFunc objective_func,
        ConstraintFunc constraint_func,
        GeneratePopulationFunc generate_population_func,
        vector<vector<int>> population,
        vector<float> fitness,
        vector<pair<int, int>> bounds);

    vector<int> Optimize();
};

#endif