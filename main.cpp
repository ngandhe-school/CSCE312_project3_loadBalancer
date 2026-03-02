/**
 * @file main.cpp
 * @brief Driver program for the Load Balancer simulation.
 */

#include <iostream>
#include <fstream>
#include <string>
#include "LoadBalancer.h"
#include "Switch.h"

int main() {
    int initialServerCount = 0;
    int simulationCycles = 0;

    std::cout << "Enter total initial number of servers (e.g., 10): ";
    std::cin >> initialServerCount;

    std::cout << "Enter number of clock cycles to run (e.g., 10000): ";
    std::cin >> simulationCycles;

    if (initialServerCount < 1 || simulationCycles < 1) {
        std::cerr << "ERROR: Servers and cycles must be >= 1\n";
        return 1;
    }

    // Read Configuration File
    LoadBalancer::Config config;
    std::ifstream configFile("config.txt");
    if (configFile.is_open()) {
        std::string line;
        while (std::getline(configFile, line)) {
            size_t delim = line.find('=');
            if (delim != std::string::npos) {
                std::string key = line.substr(0, delim);
                std::string value = line.substr(delim + 1);
                
                if (key == "MIN_TASK_TIME") config.minTaskTime = std::stoi(value);
                else if (key == "MAX_TASK_TIME") config.maxTaskTime = std::stoi(value);
                else if (key == "COOLDOWN_TIME") config.cooldownTime = std::stoi(value);
                else if (key == "INITIAL_MULTIPLIER") config.initialMultiplier = std::stoi(value);
                else if (key == "NEW_REQUEST_CHANCE") config.newRequestChance = std::stoi(value);
                else if (key == "BLOCKED_IP_START") config.blockedIpStart = value;
                else if (key == "BLOCKED_IP_END") config.blockedIpEnd = value;
            }
        }
        std::cout << "Successfully loaded config.txt\n";
    } else {
        std::cout << "Warning: config.txt not found, using defaults.\n";
    }

    std::cout << "\nStarting Load Balancer Simulation...\n";
    
    Switch topLevelSwitch(initialServerCount, simulationCycles, config);
    topLevelSwitch.run();
    
    return 0;
}