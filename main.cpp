/**
 * @file main.cpp
 * @brief Entry point for the switch-driven load balancer simulation.
 *
 * @mainpage Project 3: Load Balancer Simulation
 *
 * @section overview Overview
 * This program simulates web request load balancing over clock cycles.
 * A top-level Switch generates requests, applies firewall filtering, and routes
 * jobs by type:
 * - 'P' processing requests to the processing load balancer
 * - 'S' streaming requests to the streaming load balancer
 *
 * Each load balancer maintains:
 * - a FIFO request queue
 * - a dynamic WebServer pool
 * - autoscaling based on queue thresholds
 *
 * @section config Configuration
 * Runtime settings are read from `config.txt` using `KEY=VALUE` lines:
 * - MIN_TASK_TIME
 * - MAX_TASK_TIME
 * - COOLDOWN_TIME
 * - INITIAL_MULTIPLIER
 * - NEW_REQUEST_CHANCE
 * - BLOCKED_IP_START
 * - BLOCKED_IP_END
 *
 * @section output Output
 * Simulation events and final summaries are printed to console and written to
 * `log.txt` for submission logging requirements.
 */

#include <iostream>
#include <fstream>
#include <string>
#include "LoadBalancer.h"
#include "Switch.h"

/**
 * @brief Program entry point.
 * @return 0 on success, non-zero on invalid input.
 */
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
