/**
 * @file Switch.h
 * @brief Top-level router that generates traffic and distributes it to specialized LoadBalancers.
 */

#ifndef SWITCH_H
#define SWITCH_H

#include "LoadBalancer.h"
#include <string>
#include <random>
#include <cstdint>
#include <fstream>

class Switch {
public:
    Switch(int initialServersTotal, int simulationCycles, const LoadBalancer::Config& configValues);
    ~Switch();
    void run();

private:
    void generateInitialQueue();
    void maybeGenerateNewRequest();
    Request createRandomRequest();
    std::string createRandomIp();
    
    uint32_t ipToUint(const std::string& ip);
    bool isBlocked(const std::string& ip);

    std::ofstream logFile;
    LoadBalancer processingLB;
    LoadBalancer streamingLB;

    int totalSimulationCycles;
    long long currentCycle;
    LoadBalancer::Config config;

    uint32_t firewallStartIp;
    uint32_t firewallEndIp;
    long long totalRejected;

    std::mt19937 rng;
};

#endif