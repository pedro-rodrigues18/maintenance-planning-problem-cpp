#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include "../rapidjson/document.h"
#include "../rapidjson/filereadstream.h"
#include "problem.hpp"
#include "optimization.hpp"
#include "../utils/log.hpp"

int main() {

    string file_name = "A_08";

    auto start_time = std::chrono::high_resolution_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(start_time);

    std::ostringstream time_stream;
    time_stream << std::put_time(std::localtime(&now_c), "%Y-%m-%d %X");
    std::string formatted_time = time_stream.str();

    utils::Log(file_name, "Starting at " + formatted_time);

    // Load the problem
    FILE* fp = fopen(("input/" + file_name + ".json").c_str(), "r");

    if (!fp) {
        utils::Log(file_name, "Error: Could not open file.");
        return 1;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);

    fclose(fp);

    if (doc.HasParseError()) {
        utils::Log(file_name, "Error: Could not parse JSON.");
        utils::Log(file_name, "Error code: " + to_string(doc.GetParseError()));
        return 1;
    }

    Problem problem = Problem(&doc, file_name);

    auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start_time).count();

    utils::Log(file_name, "Problem loaded successfully!");
    utils::Log(file_name, "Elapsed time: " + to_string(elapsed_time) + "ms");

    // Optimization Step
    Optimization optimization = Optimization(&problem);

    vector<pair<string, int>> solution = optimization.OptimizationStep(start_time);

    utils::Log(file_name, "Optimization finished!");
    elapsed_time = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start_time).count();

    utils::Log(file_name, "Elapsed time: " + to_string(elapsed_time) + "ms\n");

    // Write solution to txt file
    ofstream output_file("output/" + file_name + ".txt");

    for (const auto& [name, start_time] : solution) {
        output_file << name << " " << start_time << endl;
    }

    output_file.close();

    return 0;
}