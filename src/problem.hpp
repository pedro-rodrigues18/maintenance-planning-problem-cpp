#ifndef PROBLEM_HPP
#define PROBLEM_HPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "../rapidjson/document.h"

using namespace std;

struct Resource {
    string name;
    vector<float> max;
    vector<float> min;
};

struct Intervention {
    string name;
    vector<int> delta;
    unordered_map<string,
        unordered_map<string,
        unordered_map<string, float>>> workload;
    unordered_map<string,
        unordered_map<string,
        vector<float>>> risk;
    int tmax;
};

struct Season {
    string name;
    vector<int> duration;
};

struct Exclusion {
    string name;
    vector<string> interventions;
    Season season;
};


class Problem {
public:
    string file_name;
    vector<Resource> resources;
    vector<Intervention> interventions;
    vector<Exclusion> exclusions;
    vector<int> scenarios;
    int time_steps;
    float quantile;
    float alpha;
    float computation_time;

    Problem(rapidjson::Document* doc, string file_name);

private:
    vector<Resource> GetResources(rapidjson::Document* doc);
    vector<Intervention> GetInterventions(rapidjson::Document* doc);
    unordered_map<string, unordered_map<string, unordered_map<string, float>>> GetWorkload(const rapidjson::Value& workload);
    unordered_map<string, unordered_map<string, vector<float>>>  GetRisk(const rapidjson::Value& risk);
    vector<Exclusion> GetExclusions(rapidjson::Document* doc);
    Season GetSeason(const rapidjson::Value& season, string season_name);
    vector<int> GetScenarios(rapidjson::Document* doc);
    int GetTimeSteps(rapidjson::Document* doc);
    float GetQuantile(rapidjson::Document* doc);
    float GetAlpha(rapidjson::Document* doc);
    float GetComputationTime(rapidjson::Document* doc);
};

#endif