#include <iostream>
#include <vector>
#include <fstream>
#include "../rapidjson/document.h"
#include "../rapidjson/filereadstream.h"
#include "problem.hpp"
#include "optimization.hpp"

int main() {
    // Load the problem
    FILE* fp = fopen("input/A_01.json", "r");

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

    return 0;
}