#include <algorithm>
#include <vector>
#include <iterator>
#include <cmath>
#include <iostream>
#include <chrono>
#include "optimization.hpp"
#include "gurobi.hpp"
#include "de.hpp"

Optimization::Optimization(Problem* problem) {
    this->problem = problem;
}

vector<pair<string, int>> Optimization::OptimizationStep() {
    // ------ Gurobi ------
    Gurobi gb(this->problem);

    vector<int> gurobi_solution = gb.Optimize();

    cout << "Gurobi solution: ";
    for (const auto& i : gurobi_solution) {
        cout << i << " ";
    }
    cout << endl;

    // ------ Differential Evolution ------
    vector<pair<int, int>> bounds = CreateBounds(this->problem->interventions);

    vector<vector<int>> population = Optimization::GeneratePopulation(this->problem->interventions.size(), bounds);

    population[0] = gurobi_solution;

    vector<float> penalties;
    penalties.reserve(population.size());
    for (const auto& individual : population) {
        auto [violated, penalty] = this->ConstraintSatisfied(individual);
        penalties.push_back(penalty);
    }

    vector<float> fitness;
    fitness.reserve(population.size());
    for (const auto& individual : population) {
        auto [objective, mean_risk, expected_excess] = this->ObjectiveFunction(individual, penalties[&individual - &population[0]]);
        fitness.push_back(objective);
    }

    DifferentialEvolution de(
        [this](vector<int> start_times, float penalty) { return this->ObjectiveFunction(start_times, penalty); },
        [this](vector<int> start_times) { return this->ConstraintSatisfied(start_times); },
        [this](size_t interventions_size, vector<pair<int, int>> bounds) { return GeneratePopulation(interventions_size, bounds); },
        population,
        fitness,
        bounds
    );

    vector<int> best_solution = de.Optimize();

    vector<pair<string, int>> solution;
    for (size_t i = 0; i < best_solution.size(); i++) {
        solution.push_back(make_pair(this->problem->interventions[i].name, best_solution[i]));
    }

    return solution;
}

vector<vector<int>> Optimization::GeneratePopulation(size_t interventions_size, vector<pair<int, int>> bounds) {
    vector<vector<int>> population;

    for (int i = 0; i < this->population_size; i++) {
        vector<int> individual;
        for (size_t j = 0; j < interventions_size; j++) {
            individual.push_back(rand() % (bounds[j].second - bounds[j].first + 1) + bounds[j].first);
        }
        population.push_back(individual);
    }
    return population;
}

vector<pair<int, int>> Optimization::CreateBounds(vector<Intervention> interventions) {
    vector<pair<int, int>> bounds;

    for (const auto& intervention : interventions) {
        bounds.emplace_back(1, intervention.tmax);
    }

    return bounds;
}

void Optimization::PrintSolution(vector<pair<string, int>> solution) {
    for (const auto& [name, start_time] : solution) {
        cout << name << ": " << start_time << endl;
    }
}

tuple<float, float, float> Optimization::ObjectiveFunction(vector<int> start_times, float penalty) {
    float quantile = this->problem->quantile;
    float alpha = this->problem->alpha;
    float mean_risk = 0.0;
    float expected_excess = 0.0;

    for (int t = 1; t < this->problem->time_steps + 1; t++) {
        float risk_t = 0.0;
        vector<float> risk_by_scenario;

        for (long unsigned int i = 0; i < this->problem->interventions.size(); i++) {
            int start_time = start_times[i];
            if (start_time <= t && t <= start_time + this->problem->interventions[i].delta[start_time - 1] - 1) {
                for (long unsigned int s = 0; s < static_cast<size_t>(this->problem->scenarios[t - 1]); s++) {
                    float risk_value = 0.0;

                    auto t_it = this->problem->interventions[i].risk.find(to_string(t));
                    if (t_it != this->problem->interventions[i].risk.end()) {
                        auto start_it = t_it->second.find(to_string(start_time));
                        if (start_it != t_it->second.end() && s < start_it->second.size()) {
                            risk_value = start_it->second[s];
                        }
                    }

                    risk_t += risk_value;

                    if (s >= risk_by_scenario.size()) {
                        risk_by_scenario.push_back(risk_value);
                    }
                    else {
                        risk_by_scenario[s] += risk_value;
                    }
                }
            }
        }
        risk_t /= max(1, this->problem->scenarios[t - 1]);
        mean_risk += risk_t;

        sort(risk_by_scenario.begin(), risk_by_scenario.end());

        float excess_t = 0.0;
        if (!risk_by_scenario.empty()) {
            int quantile_index = int(ceil(quantile * risk_by_scenario.size())) - 1;
            excess_t = max(0.0f, risk_by_scenario[quantile_index] - risk_t);
        }

        expected_excess += excess_t;
    }

    mean_risk /= this->problem->time_steps;
    expected_excess /= this->problem->time_steps;
    float objective = alpha * mean_risk + (1 - alpha) * expected_excess;

    return make_tuple(objective + penalty, mean_risk, expected_excess);
}

tuple<bool, float> Optimization::InterventionConstraint(vector<int> start_times) {
    float penalty = 0.0;

    for (long unsigned int i = 0; i < start_times.size(); i++) {
        int start_time = start_times[i];

        if (start_time < 0 || start_time > this->problem->interventions[i].tmax) {
            return make_tuple(true, 1.0);
        }

        if (start_time > 0 && start_time <= long(size(this->problem->interventions[i].delta))) {
            int duration = this->problem->interventions[i].delta[start_time - 1];
            int end_time = start_time + duration - 1;

            if (end_time > this->problem->time_steps) {
                penalty += float(end_time - this->problem->time_steps);
            }
        }
    }

    return make_tuple(penalty > 0, penalty);
}

tuple<bool, float> Optimization::ResourceConstraint(vector<int> start_times) {
    float penalty = 0.0;
    float eps = 1e-6;

    for (int t = 1; t <= this->problem->time_steps; t++) {
        for (const auto& resource : this->problem->resources) {
            float total_resource_usage = 0.0;
            for (size_t i = 0; i < this->problem->interventions.size(); i++) {
                const auto& intervention = this->problem->interventions[i];
                int start_time = start_times[i];
                if (start_time <= t && t <= start_time + intervention.delta[start_time - 1] - 1) {
                    // Access workload safely
                    auto resource_it = intervention.workload.find(resource.name);
                    if (resource_it != intervention.workload.end()) {
                        auto t_it = resource_it->second.find(to_string(t));
                        if (t_it != resource_it->second.end()) {
                            auto start_it = t_it->second.find(to_string(start_time));
                            if (start_it != t_it->second.end()) {
                                total_resource_usage += start_it->second;
                            }
                        }
                    }
                }
            }

            if (total_resource_usage < resource.min[t - 1] - eps) {
                penalty += (resource.min[t - 1] - total_resource_usage);
            }
            else if (total_resource_usage > resource.max[t - 1] + eps) {
                penalty += (total_resource_usage - resource.max[t - 1]);
            }
        }
    }
    return make_tuple(penalty > 0, penalty);
}

tuple<bool, float> Optimization::ExclusionConstraint(vector<int> start_times) {
    float penalty = 0.0;

    for (const auto& exclusion : this->problem->exclusions) {
        auto [i1, i2, season] = make_tuple(exclusion.interventions[0], exclusion.interventions[1], exclusion.season);
        auto it1 = find_if(this->problem->interventions.begin(), this->problem->interventions.end(), [i1](const Intervention& i) { return i.name == i1; });
        auto it2 = find_if(this->problem->interventions.begin(), this->problem->interventions.end(), [i2](const Intervention& i) { return i.name == i2; });

        int start_1 = start_times[distance(this->problem->interventions.begin(), it1)];
        int end_1 = start_1 + it1->delta[start_1 - 1] - 1;

        int start_2 = start_times[distance(this->problem->interventions.begin(), it2)];
        int end_2 = start_2 + it2->delta[start_2 - 1] - 1;

        int t_start = max(start_1, start_2);
        int t_end = min(end_1, end_2);

        if (t_start <= t_end) {
            for (int t = t_start; t <= t_end; t++) {
                if (find(season.duration.begin(), season.duration.end(), t) != season.duration.end()) {
                    penalty += 1.0;
                }
            }
        }
    }

    return make_tuple(penalty > 0, penalty);
}

tuple<bool, float> Optimization::ConstraintSatisfied(vector<int> start_times) {
    float penalty = 0.0;

    // cout << "Start times: ";
    // for (const auto& i : start_times) {
    //     cout << i << " ";
    // }
    // cout << endl;

    auto [violated, pen] = InterventionConstraint(start_times);

    if (violated) {
        //cout << "Intervention constraint violated." << endl;
        penalty += pen * 1e6;
    }

    auto [violated2, pen2] = ResourceConstraint(start_times);

    if (violated2) {
        //cout << "Resource constraint violated." << endl;
        penalty += pen2 * 1e6;
    }

    auto [violated3, pen3] = ExclusionConstraint(start_times);

    if (violated3) {
        //cout << "Exclusion constraint violated." << endl;
        penalty += pen3 * 1e6;
    }

    return make_tuple(penalty == 0.0, penalty);

}