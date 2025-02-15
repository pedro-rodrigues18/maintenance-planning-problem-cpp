#include <iostream>
#include <unordered_map>
#include "problem.hpp"
#include "../rapidjson/document.h"

using namespace std;

Problem::Problem(rapidjson::Document* doc) {
    this->resources = GetResources(doc);
    this->interventions = GetInterventions(doc);
    this->exclusions = GetExclusions(doc);
    this->scenarios = GetScenarios(doc);
    this->time_steps = GetTimeSteps(doc);
    this->quantile = GetQuantile(doc);
    this->alpha = GetAlpha(doc);
    this->computation_time = GetComputationTime(doc);
}

vector<Resource> Problem::GetResources(rapidjson::Document* doc) {
    vector<Resource> resources;

    if (!doc->HasMember("Resources") || !(*doc)["Resources"].IsObject()) {
        cerr << "Invalid or missing 'Resources' key in JSON." << endl;
        return resources;
    }

    const rapidjson::Value& resources_data = (*doc)["Resources"];
    for (auto itr = resources_data.MemberBegin(); itr != resources_data.MemberEnd(); ++itr) {
        if (!itr->value.IsObject() ||
            !itr->value.HasMember("max") || !itr->value["max"].IsArray() ||
            !itr->value.HasMember("min") || !itr->value["min"].IsArray()) {
            cerr << "Invalid resource format for: " << itr->name.GetString() << endl;
            continue;
        }

        Resource resource;
        resource.name = itr->name.GetString();

        for (const auto& v : itr->value["max"].GetArray()) {
            if (v.IsNumber()) {
                resource.max.push_back(v.GetFloat());
            }
            else {
                cerr << "Invalid max value for resource: " << itr->name.GetString() << endl;
            }
        }

        for (const auto& v : itr->value["min"].GetArray()) {
            if (v.IsNumber()) {
                resource.min.push_back(v.GetFloat());
            }
            else {
                cerr << "Invalid min value for resource: " << itr->name.GetString() << endl;
            }
        }

        resources.push_back(resource);
    }

    return resources;
}

vector<Intervention> Problem::GetInterventions(rapidjson::Document* doc) {
    vector<Intervention> interventions;

    if (!doc->HasMember("Interventions") || !(*doc)["Interventions"].IsObject()) {
        cerr << "Invalid or missing 'Resources' key in JSON." << endl;
        return interventions;
    }

    const rapidjson::Value& interventions_data = (*doc)["Interventions"];
    for (auto itr = interventions_data.MemberBegin(); itr != interventions_data.MemberEnd(); ++itr) {
        if (!itr->value.IsObject() ||
            !itr->value.HasMember("tmax") || !itr->value["tmax"].IsString() ||
            !itr->value.HasMember("Delta") || !itr->value["Delta"].IsArray() ||
            !itr->value.HasMember("workload") || !itr->value["workload"].IsObject() ||
            !itr->value.HasMember("risk") || !itr->value["risk"].IsObject()) {
            cerr << "Invalid intervention format for: " << itr->name.GetString() << endl;
            continue;
        }

        Intervention intervention;
        intervention.name = itr->name.GetString();
        intervention.tmax = stoi(itr->value["tmax"].GetString());

        for (const auto& v : itr->value["Delta"].GetArray()) {
            if (v.IsNumber()) {
                intervention.delta.push_back(v.GetFloat());
            }
            else {
                cerr << "Invalid Delta value for: " << itr->name.GetString() << endl;
            }
        }

        intervention.workload = Problem::GetWorkload(itr->value["workload"]);
        intervention.risk = Problem::GetRisk(itr->value["risk"]);

        interventions.push_back(intervention);
    }

    return interventions;
}

unordered_map<string, unordered_map<string, unordered_map<string, float>>> Problem::GetWorkload(const rapidjson::Value& workload) {

    unordered_map<string,
        unordered_map<string,
        unordered_map<string, float>>> resource_workload;

    for (auto itr = workload.MemberBegin(); itr != workload.MemberEnd(); ++itr) {
        if (!itr->value.IsObject()) {
            cerr << "Invalid workload format for: " << itr->name.GetString() << endl;
            continue;
        }

        for (auto itr2 = itr->value.MemberBegin(); itr2 != itr->value.MemberEnd(); ++itr2) {
            if (!itr2->value.IsObject()) {
                cerr << "Invalid workload format for: " << itr->name.GetString() << endl;
                continue;
            }

            for (auto itr3 = itr2->value.MemberBegin(); itr3 != itr2->value.MemberEnd(); ++itr3) {
                if (!itr3->value.IsNumber()) {
                    cerr << "Invalid workload format for: " << itr->name.GetString() << endl;
                    continue;
                }

                resource_workload[itr->name.GetString()][itr2->name.GetString()][itr3->name.GetString()] = itr3->value.GetFloat();
            }
        }
    }

    return resource_workload;
}

unordered_map<string, unordered_map<string, vector<float>>> Problem::GetRisk(const rapidjson::Value& risk) {
    unordered_map<string,
        unordered_map<string,
        vector<float>>> intervention_risk;

    for (auto itr = risk.MemberBegin(); itr != risk.MemberEnd(); ++itr) {
        for (auto itr2 = itr->value.MemberBegin(); itr2 != itr->value.MemberEnd(); ++itr2) {
            if (!itr2->value.IsArray()) {
                cerr << "Invalid risk format for: " << itr->name.GetString() << endl;
                continue;
            }

            for (const auto& v : itr2->value.GetArray()) {
                if (v.IsNumber()) {
                    intervention_risk[itr->name.GetString()][itr2->name.GetString()].push_back(v.GetFloat());
                }
                else {
                    cerr << "Invalid risk value for: " << itr->name.GetString() << endl;
                }
            }
        }
    }

    return intervention_risk;
}


vector<Exclusion> Problem::GetExclusions(rapidjson::Document* doc) {
    vector<Exclusion> exclusions;

    if (!doc->HasMember("Exclusions") || !(*doc)["Exclusions"].IsObject()) {
        cerr << "Invalid or missing 'Exclusions' key in JSON." << endl;
        return exclusions;
    }

    if (!doc->HasMember("Seasons") || !(*doc)["Seasons"].IsObject()) {
        cerr << "Invalid or missing 'Seasons' key in JSON." << endl;
        return exclusions;
    }

    const rapidjson::Value& exclusions_data = (*doc)["Exclusions"];

    for (auto itr = exclusions_data.MemberBegin(); itr != exclusions_data.MemberEnd(); ++itr) {
        if (!itr->value.IsArray()) {
            cerr << "Invalid exclusion format for: " << itr->name.GetString() << endl;
            continue;
        }

        Exclusion exclusion;
        exclusion.name = itr->name.GetString();
        exclusion.interventions.push_back(itr->value.GetArray()[0].GetString());
        exclusion.interventions.push_back(itr->value.GetArray()[1].GetString());
        exclusion.season = Problem::GetSeason((*doc)["Seasons"], itr->value.GetArray()[2].GetString());

        exclusions.push_back(exclusion);
    }

    return exclusions;
}

Season Problem::GetSeason(const rapidjson::Value& season, string season_name) {
    Season s;
    s.name = season_name;

    // cout << "Getting season: " << season_name << endl;
    // cout << season.HasMember(season_name.c_str()) << endl;
    // cout << season[season_name.c_str()].IsArray() << endl;

    if (!season.HasMember(season_name.c_str()) || !season[season_name.c_str()].IsArray()) {
        cerr << "Invalid season format for: " << season_name << endl;
        return s;
    }

    for (const auto& v : season[season_name.c_str()].GetArray()) {
        if (v.IsString()) {
            s.duration.push_back(stoi(v.GetString()));
        }
        else {
            cerr << "Invalid duration value for season: " << season_name << endl;
        }
    }

    return s;
}


vector<int> Problem::GetScenarios(rapidjson::Document* doc) {
    vector<int> scenarios;

    if (!(*doc).HasMember("Scenarios_number") || !(*doc)["Scenarios_number"].IsArray()) {
        cerr << "Invalid or missing 'Scenarios' key in JSON." << endl;
        return scenarios;
    }

    for (const auto& v : (*doc)["Scenarios_number"].GetArray()) {
        if (v.IsNumber()) {
            scenarios.push_back(v.GetInt());
        }
        else {
            cerr << "Invalid scenario value." << endl;
        }
    }

    return scenarios;
}

int Problem::GetTimeSteps(rapidjson::Document* doc) {
    if (!(*doc).HasMember("T") || !(*doc)["T"].IsInt()) {
        cerr << "Invalid or missing 'TimeSteps (T)' key in JSON." << endl;
        return -1;
    }

    return (*doc)["T"].GetInt();
}

float Problem::GetQuantile(rapidjson::Document* doc) {
    if (!(*doc).HasMember("Quantile") || !(*doc)["Quantile"].IsNumber()) {
        cerr << "Invalid or missing 'Quantile' key in JSON." << endl;
        return -1;
    }

    return (*doc)["Quantile"].GetFloat();
}

float Problem::GetAlpha(rapidjson::Document* doc) {
    if (!(*doc).HasMember("Alpha") || !(*doc)["Alpha"].IsNumber()) {
        cerr << "Invalid or missing 'Alpha' key in JSON." << endl;
        return -1;
    }

    return (*doc)["Alpha"].GetFloat();
}

float Problem::GetComputationTime(rapidjson::Document* doc) {
    if (!(*doc).HasMember("ComputationTime") || !(*doc)["ComputationTime"].IsNumber()) {
        cerr << "Invalid or missing 'ComputationTime' key in JSON." << endl;
        return -1;
    }

    return (*doc)["ComputationTime"].GetFloat();
}