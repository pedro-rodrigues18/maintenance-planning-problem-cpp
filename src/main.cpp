#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <chrono>
#include "../rapidjson/document.h"
#include "../rapidjson/filereadstream.h"
#include "problem.hpp"
#include "optimization.hpp"
#include "../utils/log.hpp"

void MakeOptimization(std::string instance) {
    cout << "Running instance " << instance << endl;

    auto start_time = std::chrono::high_resolution_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(start_time);

    std::ostringstream time_stream;
    time_stream << std::put_time(std::localtime(&now_c), "%Y-%m-%d %X");
    std::string formatted_time = time_stream.str();

    utils::Log(instance, "Starting at " + formatted_time);

    // Load the problem
    FILE* fp = fopen(("input/" + instance + ".json").c_str(), "r");

    if (!fp) {
        utils::Log(instance, "Error: Could not open file.");
        exit(1);
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);

    fclose(fp);

    if (doc.HasParseError()) {
        utils::Log(instance, "Error: Could not parse JSON.");
        utils::Log(instance, "Error code: " + to_string(doc.GetParseError()));
        exit(1);
    }

    Problem problem = Problem(&doc, instance);

    auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start_time).count();

    utils::Log(instance, "Problem loaded successfully!");
    utils::Log(instance, "Elapsed time: " + to_string(elapsed_time) + "ms");

    // Optimization Step
    Optimization optimization = Optimization(&problem);

    vector<pair<string, int>> solution = optimization.OptimizationStep(start_time);

    utils::Log(instance, "Optimization finished!");
    elapsed_time = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start_time).count();

    utils::Log(instance, "Elapsed time: " + to_string(elapsed_time) + "ms\n");

    // Write solution to txt file
    ofstream output_file("output/" + instance + ".txt");

    for (const auto& [intervention, start_time_execution] : solution) {
        output_file << intervention << " " << start_time_execution << endl;
    }

    output_file.close();
}

void RunAllInstances(std::vector<std::string> instances) {
    sort(instances.begin(), instances.end());
    for (const auto& instance : instances) {
        MakeOptimization(instance);
    }
}

int main() {
    bool run_all = false;
    std::string input_path = "input/";
    std::string instance = "";
    std::vector<std::string> instances;

    for (const auto& entry : std::filesystem::directory_iterator(input_path)) {
        instance = entry.path().filename().string();
        instances.push_back(instance.substr(0, instance.find('.')));
    }

    if (run_all) {
        RunAllInstances(instances);
    }
    else {
        instance = "A_09";
        MakeOptimization(instance);
    }

    return 0;
}