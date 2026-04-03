#pragma once
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include "MoveCommand.h"

inline std::vector<Command*> parseFile(const std::string& filename) {
    std::vector<Command*> commands;

    std::ifstream file(filename);
    std::string line;

    float x = 100.f;
    float y = 100.f;

    while (std::getline(file, line)) {
        std::stringstream ss(line);

        std::string cmd;
        std::string axis;
        float value;

        ss >> cmd >> axis >> value;

        if (cmd == "MOVE") {
            if (axis == "X") x = value;
            if (axis == "Y") y = value;

            commands.push_back(new MoveCommand(x, y));
        }
    }

    return commands;
}