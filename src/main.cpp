#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include "../rapidjson/document.h"
#include "../rapidjson/filereadstream.h"
#include "problem.hpp"
#include "optimization.hpp"

int main() {
    //auto start_time = std::chrono::high_resolution_clock::now();

    string file_name = "B_01";

    // Load the problem
    FILE* fp = fopen(("input/" + file_name + ".json").c_str(), "r");

    if (!fp) {
        std::cerr << "Error: Could not open file." << std::endl;
        return 1;
    }

    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

    rapidjson::Document doc;
    doc.ParseStream(is);

    fclose(fp);

    if (doc.HasParseError()) {
        std::cerr << "Error parasing JSON: " << doc.GetParseError() << std::endl;
        return 1;
    }

    Problem problem = Problem(&doc);

    // Optimization Step
    Optimization optimization = Optimization(&problem);

    vector<pair<string, int>> solution = optimization.OptimizationStep();

    // Write solution to txt file
    ofstream output_file("output/" + file_name + ".txt");

    for (const auto& [name, start_time] : solution) {
        output_file << name << " " << start_time << endl;
    }

    output_file.close();

    return 0;
}