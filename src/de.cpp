#include "de.hpp"
#include "optimization.hpp"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <random>
#include <omp.h>

DifferentialEvolution::DifferentialEvolution(
    ObjectiveFunc objective_func,
    ConstraintFunc constraint_func,
    Problem* problem,
    vector<int> gurobi_solution) :
    objective_func(objective_func), constraint_func(constraint_func), problem(problem) {

    this->problem = problem;
    this->bounds = CreateBounds(this->problem->interventions);
    this->population = GeneratePopulation(this->problem->interventions.size());

    this->population[0] = gurobi_solution;

    vector<float> penalties;
    penalties.reserve(this->population.size());
    for (const auto& individual : this->population) {
        auto [violated, penalty] = this->constraint_func(individual);
        penalties.push_back(penalty);
    }

    this->fitness.reserve(this->population.size());
    for (const auto& individual : this->population) {
        auto [objective, mean_risk, expected_excess] = this->objective_func(individual, penalties[&individual - &population[0]]);
        this->fitness.push_back(objective);
    }
}

vector<vector<int>> DifferentialEvolution::GeneratePopulation(size_t interventions_size) {
    vector<vector<int>> population;

    random_device rd;
    mt19937 gen(rd());

    for (int i = 0; i < this->population_size; i++) {
        vector<int> individual;
        for (size_t j = 0; j < interventions_size; j++) {
            uniform_int_distribution<int> dist(this->bounds[j].first, this->bounds[j].second);
            individual.push_back(dist(gen));
        }
        population.push_back(individual);
    }
    return population;
}

vector<pair<int, int>> DifferentialEvolution::CreateBounds(vector<Intervention> interventions) {
    vector<pair<int, int>> bounds;

    for (const auto& intervention : interventions) {
        bounds.emplace_back(1, intervention.tmax);
    }

    return bounds;
}

vector<int> DifferentialEvolution::Optimize(chrono::time_point<chrono::high_resolution_clock> start_time) {
    auto remaining_time = TIME_LIMIT - (chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - start_time).count());
    int iterations_without_improvement = 0;

    while (remaining_time > 0) {
        if (iterations_without_improvement > 100) {
            //cout << "Restarting population" << endl;
            vector<int> best_solution = this->population[distance(this->fitness.begin(), min_element(this->fitness.begin(), this->fitness.end()))];
            float best_fitness = *min_element(this->fitness.begin(), this->fitness.end());

            this->population = GeneratePopulation(best_solution.size());

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

        new_population.resize(this->population.size());
        new_fitness.resize(this->population.size());

#pragma omp parallel for
        for (size_t i = 0; i < this->population.size(); i++) {
            random_device rd;
            unsigned int seed = rd() ^ (static_cast<unsigned int>(omp_get_thread_num()));
            mt19937 rng(seed);
            uniform_int_distribution<size_t> dist_index(0, this->population.size() - 1);
            uniform_real_distribution<float> dist_real(0.0, 1.0);

            vector<int> target = this->population[i];

            size_t x1_index = dist_index(rng);
            size_t x2_index = dist_index(rng);

            while (x1_index == i || x2_index == i || x1_index == x2_index) {
                x1_index = dist_index(rng);
                x2_index = dist_index(rng);
            }

            vector<int> best = this->population[distance(this->fitness.begin(), min_element(this->fitness.begin(), this->fitness.end()))];
            vector<int> x1 = this->population[x1_index];
            vector<int> x2 = this->population[x2_index];

            // Mutation (/best/1)
            vector<int> mutant;
            for (size_t j = 0; j < target.size(); j++) {
                if (dist_real(rng) < this->mutation_rate) {
                    int chromosome = best[j] + this->mutation_rate * (x1[j] - x2[j]);
                    chromosome = max(this->bounds[j].first, min(this->bounds[j].second, chromosome));
                    mutant.push_back(chromosome);
                }
                else {
                    mutant.push_back(target[j]);
                }
            }

            // Exponential Crossover (/exp)
            vector<int> trial = target;
            size_t j = dist_index(rng) % target.size();
            size_t L = 0;
            do {
                trial[j] = mutant[j];
                j = (j + 1) % target.size();
                L++;
            } while (dist_index(rng) < this->crossover_rate && L < target.size());

            // Constraint satisfaction
            auto [violated, penalty] = this->constraint_func(trial);
            auto [objective, mean_risk, expected_excess] = this->objective_func(trial, penalty);

            if (objective < this->fitness[i]) {
                new_population[i] = trial;
                new_fitness[i] = objective;
            }
            else {
                new_population[i] = this->population[i];
                new_fitness[i] = this->fitness[i];
            }
        }

        float new_best_fitness = *min_element(new_fitness.begin(), new_fitness.end());
        cout << "Best fitness: " << setprecision(6) << new_best_fitness << fixed << "\r" << flush;

        if (new_best_fitness < *min_element(this->fitness.begin(), this->fitness.end())) {
            iterations_without_improvement = 0;
        }
        else {
            iterations_without_improvement++;
        }

        this->population = new_population;
        this->fitness = new_fitness;

        remaining_time = TIME_LIMIT - (chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - start_time).count());
    }

    vector<int> best_solution = this->population[distance(this->fitness.begin(), min_element(this->fitness.begin(), this->fitness.end()))];

    auto [violated, penalty] = this->constraint_func(best_solution);
    auto [objective, mean_risk, expected_excess] = this->objective_func(best_solution, penalty);

    ostringstream oss;
    oss << "[";
    for (size_t j = 0; j < best_solution.size(); ++j) {
        oss << best_solution[j];
        if (j < best_solution.size() - 1) oss << ", ";
    }
    oss << "]";

    utils::Log(this->problem->file_name, "DE solution: " + oss.str());
    utils::Log(this->problem->file_name, "Mean risk: " + to_string(mean_risk));
    utils::Log(this->problem->file_name, "Expected excess: " + to_string(expected_excess));
    utils::Log(this->problem->file_name, "Objective: " + to_string(objective));

    return best_solution;
}

