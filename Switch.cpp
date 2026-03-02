/**
 * @file Switch.cpp
 * @brief Implementation of the Switch class.
 */

#include "Switch.h"
#include <iostream>
#include <sstream>

const std::string RED = "\033[31m";
const std::string CYAN = "\033[36m";
const std::string RESET = "\033[0m";

// Splits the total servers evenly between the two LBs
Switch::Switch(int initialServersTotal, int simulationCycles, const LoadBalancer::Config& configValues)
    : logFile("log.txt"),
      processingLB("Processing", initialServersTotal / 2, configValues, &logFile),
      streamingLB("Streaming", initialServersTotal - (initialServersTotal / 2), configValues, &logFile),
      totalSimulationCycles(simulationCycles),
      currentCycle(0),
      config(configValues),
      totalRejected(0),
      rng(std::random_device{}()) {
    
    if (!logFile.is_open()) {
        std::cerr << "WARNING: Could not open log.txt for writing.\n";
    }

    firewallStartIp = ipToUint(config.blockedIpStart);
    firewallEndIp = ipToUint(config.blockedIpEnd);
    if (firewallStartIp > firewallEndIp) std::swap(firewallStartIp, firewallEndIp);
}

Switch::~Switch() {
    if (logFile.is_open()) {
        logFile.flush();
        logFile.close();
    }
}

uint32_t Switch::ipToUint(const std::string& ip) {
    uint32_t a, b, c, d;
    if (sscanf(ip.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return 0;
    return (a << 24) | (b << 16) | (c << 8) | d;
}

bool Switch::isBlocked(const std::string& ip) {
    uint32_t ipValue = ipToUint(ip);
    return (ipValue >= firewallStartIp && ipValue <= firewallEndIp);
}

std::string Switch::createRandomIp() {
    std::uniform_int_distribution<int> octetDist(1, 254);
    std::uniform_int_distribution<int> evilDist(1, 100);
    
    if (evilDist(rng) == 1) {
        return "192.168." + std::to_string(octetDist(rng)) + "." + std::to_string(octetDist(rng));
    }

    return std::to_string(octetDist(rng)) + "." +
           std::to_string(octetDist(rng)) + "." +
           std::to_string(octetDist(rng)) + "." +
           std::to_string(octetDist(rng));
}

Request Switch::createRandomRequest() {
    std::uniform_int_distribution<int> timeDist(config.minTaskTime, config.maxTaskTime);
    std::uniform_int_distribution<int> typeDist(0, 1);

    Request req;
    req.sourceIp = createRandomIp();
    req.destinationIp = createRandomIp();
    req.processingTime = timeDist(rng);
    req.jobType = typeDist(rng) == 0 ? 'S' : 'P';
    req.createdCycle = currentCycle;
    return req;
}

void Switch::generateInitialQueue() {
    int totalServers = processingLB.getServerCount() + streamingLB.getServerCount();
    int target = totalServers * config.initialMultiplier; 

    for (int i = 0; i < target; ++i) {
        Request req = createRandomRequest();
        if (isBlocked(req.sourceIp)) {
            totalRejected++;
            continue;
        }
        
        if (req.jobType == 'P') processingLB.addRequest(req);
        else streamingLB.addRequest(req);
    }

    processingLB.recordStartingQueue();
    streamingLB.recordStartingQueue();
}

void Switch::maybeGenerateNewRequest() {
    std::uniform_int_distribution<int> chanceDist(1, 100);
    if (chanceDist(rng) > config.newRequestChance) return;

    Request req = createRandomRequest();
    if (isBlocked(req.sourceIp)) {
        totalRejected++;
        std::cout << RED << "[Cycle " << currentCycle << "] SWITCH BLOCKED IP: " << req.sourceIp << RESET << "\n";
        if (logFile.is_open()) {
            logFile << "[Cycle " << currentCycle << "] SWITCH BLOCKED IP: " << req.sourceIp << "\n";
        }
        return;
    }

    if (req.jobType == 'P') processingLB.addRequest(req);
    else streamingLB.addRequest(req);
}

void Switch::run() {
    std::cout << CYAN << "==== Switch Routing Simulation Started ====" << RESET << "\n";
    if (logFile.is_open()) logFile << "==== Switch Routing Simulation Started ====\n";
    generateInitialQueue();

    for (currentCycle = 1; currentCycle <= totalSimulationCycles; ++currentCycle) {
        maybeGenerateNewRequest();
        
        processingLB.tickCycle(currentCycle);
        streamingLB.tickCycle(currentCycle);
    }

    std::cout << CYAN << "\n==== GLOBAL SWITCH SUMMARY ====" << RESET << "\n";
    std::cout << "Total Malicious Requests Blocked: " << totalRejected << "\n\n";

    if (logFile.is_open()) {
        logFile << "\n==== GLOBAL SWITCH SUMMARY ====\n";
        logFile << "Total Malicious Requests Blocked: " << totalRejected << "\n\n";
    }
    
    processingLB.printSummary();
    streamingLB.printSummary();
}