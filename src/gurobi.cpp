#include "gurobi.hpp"

Gurobi::Gurobi(Problem* problem) {
    this->problem = problem;
}

vector<int> Gurobi::Optimize(chrono::time_point<chrono::high_resolution_clock> start_time) {
    utils::Log(this->problem->file_name, "\nStarting Gurobi optimization.\n");
    auto remaining_time = TIME_LIMIT - (chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - start_time).count());
    GRBEnv env = GRBEnv();
    GRBModel model = GRBModel(env);

    model.set(GRB_DoubleParam_TimeLimit, remaining_time);
    model.set(GRB_IntParam_OutputFlag, 0);
    model.set(GRB_DoubleParam_MIPGap, 0.00);
    model.set(GRB_IntParam_MIPFocus, 1);  // Prioritize finding feasible solutions
    model.set(GRB_IntParam_Presolve, 1);  // 1 - conservative, 0 - off, 2 - aggressive
    model.set(GRB_IntParam_PrePasses, 1);  // to not spend too much time in pre-solve
    model.set(GRB_IntParam_Method, 1);

    // Create variable
    map<int, map<int, GRBVar>> x;
    for (size_t i = 0; i < this->problem->interventions.size(); ++i) {
        map<int, GRBVar> temp_map;
        for (int t = 1; t <= this->problem->time_steps; ++t) {
            temp_map[t] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "x_" + to_string(i) + "_" + to_string(t));
        }
        x[i] = temp_map;
    }

    // Set objective
    GRBLinExpr obj = 0;
    for (int t = 1; t <= this->problem->time_steps; t++) {
        int scenariosCount = this->problem->scenarios[t - 1];
        for (int s = 0; s < scenariosCount; s++) {
            for (size_t i = 0; i < this->problem->interventions.size(); i++) {
                int tmax = this->problem->interventions[i].tmax;
                for (int st = 1; st <= tmax; st++) {
                    // Check if the risk data for time 't' exists
                    auto riskTimeIt = this->problem->interventions[i].risk.find(to_string(t));
                    if (riskTimeIt == this->problem->interventions[i].risk.end())
                        continue;

                    // Check if the risk data for start time 'st' exists
                    auto riskStIt = riskTimeIt->second.find(to_string(st));
                    if (riskStIt == riskTimeIt->second.end())
                        continue;

                    const vector<float>& riskVec = riskStIt->second;

                    // Check that the scenario index 's' is within bounds
                    if (s >= static_cast<int>(riskVec.size()))
                        continue;

                    double coeff = riskVec[s] / (scenariosCount * this->problem->time_steps);

                    obj += coeff * x[i][st];
                }
            }
        }
    }

    // Intervention constraint
    for (size_t i = 0; i < this->problem->interventions.size(); i++) {
        GRBLinExpr expr = 0;
        for (int t = 1; t <= this->problem->interventions[i].tmax; t++) {
            expr += x[i][t];
        }

        model.addConstr(expr == 1);
    }

    // Resource constraint
    for (size_t r = 0; r < this->problem->resources.size(); r++) {
        for (int t = 1; t <= this->problem->time_steps; t++) {
            GRBLinExpr expr = 0;
            for (size_t i = 0; i < this->problem->interventions.size(); i++) {
                for (int st = 1; st <= this->problem->interventions[i].tmax; st++) {
                    if (st <= t && t <= st + this->problem->interventions[i].delta[st - 1] - 1) {
                        expr += this->problem->interventions[i].workload[this->problem->resources[r].name][to_string(t)][to_string(st)] * x[i][st];
                    }
                }
            }
            model.addConstr(expr >= this->problem->resources[r].min[t - 1]);
            model.addConstr(expr <= this->problem->resources[r].max[t - 1]);
        }
    }

    // Exclusion constraint
    for (const auto& exclusion : this->problem->exclusions) {
        auto [i1, i2, season] = make_tuple(exclusion.interventions[0], exclusion.interventions[1], exclusion.season);
        auto it1 = find_if(this->problem->interventions.begin(), this->problem->interventions.end(), [i1](const Intervention& i) { return i.name == i1; });
        auto it2 = find_if(this->problem->interventions.begin(), this->problem->interventions.end(), [i2](const Intervention& i) { return i.name == i2; });

        for (size_t t = 0; t < season.duration.size(); t++) {
            GRBLinExpr expr = 0;
            for (size_t st = 1; st <= static_cast<size_t>(it1->tmax); st++) {
                if (st <= static_cast<size_t>(season.duration[t]) && static_cast<size_t>(season.duration[t]) <= st + it1->delta[st - 1] - 1) {
                    expr += x[distance(this->problem->interventions.begin(), it1)][st];
                }
            }

            for (size_t st = 1; st <= static_cast<size_t>(it2->tmax); st++) {
                if (st <= static_cast<size_t>(season.duration[t]) && static_cast<size_t>(season.duration[t]) <= st + it2->delta[st - 1] - 1) {
                    expr += x[distance(this->problem->interventions.begin(), it2)][st];
                }
            }

            model.addConstr(expr <= 1);
        }
    }

    model.setObjective(obj, GRB_MINIMIZE);
    model.optimize();

    vector<int> start_times;
    for (size_t i = 0; i < this->problem->interventions.size(); i++) {
        for (int t = 1; t <= this->problem->interventions[i].tmax; t++) {
            if (x[i][t].get(GRB_DoubleAttr_X) > 0.5) {
                start_times.push_back(t);
            }
        }
    }

    return start_times;
}