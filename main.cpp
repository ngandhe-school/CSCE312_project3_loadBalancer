#include <iostream>
#include <fstream>
#include <string>

#include "LoadBalancer.h"
#include "Firewall.h"

static std::string trim(const std::string& text) {
    const size_t start = text.find_first_not_of(" \t\r\n");
    if(start == std::string::npos){
        return "";
    }
    const size_t end = text.find_last_not_of(" \t\r\n");

    return text.substr(start, end - start + 1);
}

static LoadBalancer::Config readConfig(const std::string& path, Firewall& firewall) {
    LoadBalancer::Config config;

    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << "WARNING: Could not open config file: " << path << ". Using defaults.\n";
        return config;
    }

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        const size_t equalsIndex = line.find('=');
        if (equalsIndex == std::string::npos) continue;

        const std::string key = trim(line.substr(0, equalsIndex));
        const std::string value = trim(line.substr(equalsIndex + 1));

        if (key == "initialQueueMultiplier") config.initialQueueMultiplier = std::stoi(value);
        else if (key == "minTaskTime") config.minTaskTime = std::stoi(value);
        else if (key == "maxTaskTime") config.maxTaskTime = std::stoi(value);
        else if (key == "cooldownCycles") config.cooldownCycles = std::stoi(value);
        else if (key == "newRequestChancePercent") config.newRequestChancePercent = std::stoi(value);
        else if (key == "lowerQueuePerServer") config.lowerQueuePerServer = std::stoi(value);
        else if (key == "upperQueuePerServer") config.upperQueuePerServer = std::stoi(value);
        else if (key == "snapshotEvery") config.snapshotEvery = std::stoi(value);
        else if (key == "logFile") config.logFile = value;
        else if (key == "blockRange") {
            const size_t dash = value.find('-');
            if (dash != std::string::npos) {
                const std::string startIp = trim(value.substr(0, dash));
                const std::string endIp = trim(value.substr(dash + 1));
                firewall.addBlockedRange(startIp, endIp);
            }
        }
    }

    return config;
}

int main() {
    int initialServerCount = 0;
    int simulationCycles = 0;

    std::cout << "Enter initial number of servers: ";
    std::cin >> initialServerCount;

    std::cout << "Enter number of clock cycles to run: ";
    std::cin >> simulationCycles;

    if (initialServerCount < 1) {
        std::cerr << "ERROR: servers must be >= 1\n";
        return 1;
    }
    if (simulationCycles < 1) {
        std::cerr << "ERROR: cycles must be >= 1\n";
        return 1;
    }

    Firewall firewall;
    const LoadBalancer::Config config = readConfig("config.txt", firewall);

    std::cout << "\nLoaded config. Starting simulation...\n";
    std::cout << "Log file: " << config.logFile << "\n";

    LoadBalancer loadBalancer(initialServerCount, simulationCycles, config, firewall);
    loadBalancer.run();

    std::cout << "Done. Log written to " << config.logFile << "\n";
    return 0;
}
