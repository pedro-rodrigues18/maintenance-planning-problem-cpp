#include "de.hpp"
#include "optimization.hpp"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <random>

DifferentialEvolution::DifferentialEvolution(
    ObjectiveFunc objective_func,
    ConstraintFunc constraint_func,
    GeneratePopulationFunc generate_population_func,
    vector<vector<int>> population,
    vector<float> fitness,
    vector<pair<int, int>> bounds)
    : objective_func(objective_func), constraint_func(constraint_func), generate_population_func(generate_population_func) {
    this->population = population;
    this->fitness = fitness;
    this->bounds = bounds;
}

vector<int> DifferentialEvolution::Optimize() {
    chrono::time_point<chrono::high_resolution_clock> start_time = chrono::high_resolution_clock::now();
    int iterations_without_improvement = 0;

    while (true) {
        if (chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - start_time).count() > TIME_LIMIT) {
            break;
        }

        if (iterations_without_improvement > 500) {
            // Get best individual in population
            vector<int> best_solution = this->population[distance(this->fitness.begin(), min_element(this->fitness.begin(), this->fitness.end()))];
            float best_fitness = *min_element(this->fitness.begin(), this->fitness.end());

            this->population = this->generate_population_func(best_solution.size(), this->bounds);

            vector<float> penalties;
            penalties.reserve(this->population.size());
            for (const auto& individual : this->population) {
                auto [violated, penalty] = this->constraint_func(individual);
                penalties.push_back(penalty);
            }

            vector<float> fitness;
            fitness.reserve(this->population.size());
            for (const auto& individual : this->population) {
                auto [objective, mean_risk, expected_excess] = this->objective_func(individual, penalties[&individual - &this->population[0]]);
                fitness.push_back(objective);
            }
            this->fitness = fitness;

            // Add the best solution to the population
            this->population[0] = best_solution;
            this->fitness[0] = best_fitness;

            iterations_without_improvement = 0;
        }

        vector<vector<int>> new_population;
        vector<float> new_fitness;

        for (size_t i = 0; i < this->population.size(); i++) {
            vector<int> target = this->population[i];

            size_t x1_index = rand() % this->population.size();
            size_t x2_index = rand() % this->population.size();

            // Ensure that indces are different and not equal to the target
            while (x1_index == i || x2_index == i || x1_index == x2_index) {
                x1_index = rand() % this->population.size();
                x2_index = rand() % this->population.size();
            }

            vector<int> best = this->population[distance(this->fitness.begin(), min_element(this->fitness.begin(), this->fitness.end()))];
            vector<int> x1 = this->population[x1_index];
            vector<int> x2 = this->population[x2_index];

            // Mutation best/1/bin
            vector<int> mutant;
            for (size_t j = 0; j < target.size(); j++) {
                int chromosome = best[j] + this->mutation_rate * (x1[j] - x2[j]);
                chromosome = max(this->bounds[j].first, min(this->bounds[j].second, chromosome));
                mutant.push_back(chromosome);
            }

            // Exponential Crossover
            vector<int> trial = target;
            size_t j = rand() % target.size();
            size_t L = 0;
            do {
                trial[j] = mutant[j];
                j = (j + 1) % target.size();
                L++;
            } while (rand() / static_cast<float>(RAND_MAX) < this->crossover_rate && L < target.size());

            // Constraint satisfaction
            auto [violated, penalty] = this->constraint_func(trial);
            auto [objective, mean_risk, expected_excess] = this->objective_func(trial, penalty);

            if (objective < this->fitness[i]) {
                cout << "Objective: " << objective + penalty << endl;
                new_population.push_back(trial);
                new_fitness.push_back(objective + penalty);

                iterations_without_improvement = 0;
            }
            else {
                new_population.push_back(this->population[i]);
                new_fitness.push_back(this->fitness[i]);

                iterations_without_improvement++;
            }
        }

        this->population = new_population;
        this->fitness = new_fitness;
    }

    vector<int> best_solution = this->population[distance(this->fitness.begin(), min_element(this->fitness.begin(), this->fitness.end()))];

    auto [violated, penalty] = this->constraint_func(best_solution);
    auto [objective, mean_risk, expected_excess] = this->objective_func(best_solution, penalty);

    cout << "Best solution found: ";
    for (const auto& i : best_solution) {
        std::cout << i << " ";
    }
    std::cout << endl;

    cout << "Objective: " << objective << endl;
    cout << "Mean risk: " << mean_risk << endl;
    cout << "Expected excess: " << expected_excess << endl;

    return best_solution;
}