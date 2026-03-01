/**
 * @file LoadBalancer.cpp
 * @brief Implementation of the LoadBalancer class.
 */

#include "LoadBalancer.h"
#include <iostream>
#include <algorithm>
#include <sstream>

const std::string RESET = "\033[0m";
const std::string GREEN = "\033[32m";
const std::string RED = "\033[31m";
const std::string YELLOW = "\033[33m";
const std::string CYAN = "\033[36m";

LoadBalancer::LoadBalancer(int initialServers, int simulationCycles, const Config& configValues)
    : totalSimulationCycles(simulationCycles),
      currentCycle(0),
      config(configValues),
      nextServerId(1),
      cooldownCyclesRemaining(0),
      startingQueueSize(0),
      totalGenerated(0),
      totalRejected(0),
      totalCompleted(0),
      scaleUps(0),
      scaleDowns(0),
      maxQueueObserved(0),
      rng(std::random_device{}()) {
    
    logFile.open("log.txt");
    
    for (int i = 0; i < initialServers; ++i) {
        servers.emplace_back(nextServerId++);
    }

    // Set Firewall True Range Bounds
    firewallStartIp = ipToUint(config.blockedIpStart);
    firewallEndIp = ipToUint(config.blockedIpEnd);
    if (firewallStartIp > firewallEndIp) std::swap(firewallStartIp, firewallEndIp);

    logEvent("==== Load Balancer Simulation Started ====", CYAN);
}

LoadBalancer::~LoadBalancer() {
    if (logFile.is_open()) logFile.close();
}

void LoadBalancer::logEvent(const std::string& text, const std::string& colorCode) {
    if (logFile.is_open()) logFile << text << "\n";
    if (!colorCode.empty()) std::cout << colorCode << text << RESET << "\n";
    else std::cout << text << "\n";
}

uint32_t LoadBalancer::ipToUint(const std::string& ip) {
    uint32_t a, b, c, d;
    // Simple sscanf format to grab octets
    if (sscanf(ip.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return 0;
    return (a << 24) | (b << 16) | (c << 8) | d;
}

bool LoadBalancer::isBlocked(const std::string& ip) {
    uint32_t ipValue = ipToUint(ip);
    return (ipValue >= firewallStartIp && ipValue <= firewallEndIp);
}

std::string LoadBalancer::createRandomIp() {
    std::uniform_int_distribution<int> octetDist(1, 254);
    std::uniform_int_distribution<int> evilDist(1, 10);
    
    // Artificially inject some blocked IPs for testing logging
    if (evilDist(rng) == 1) {
        return "192.168." + std::to_string(octetDist(rng)) + "." + std::to_string(octetDist(rng));
    }

    return std::to_string(octetDist(rng)) + "." +
           std::to_string(octetDist(rng)) + "." +
           std::to_string(octetDist(rng)) + "." +
           std::to_string(octetDist(rng));
}

Request LoadBalancer::createRandomRequest() {
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

void LoadBalancer::generateInitialQueue() {
    int target = servers.size() * INITIAL_MULTIPLIER;
    for (int i = 0; i < target; ++i) {
        Request req = createRandomRequest();
        if (isBlocked(req.sourceIp)) {
            totalRejected++;
            continue;
        }
        requestQueue.push(req);
        totalGenerated++;
    }

    startingQueueSize = requestQueue.size();
    maxQueueObserved = std::max(maxQueueObserved, requestQueue.size());

    logEvent("Initial servers: " + std::to_string(servers.size()));
    logEvent("Initial queue size: " + std::to_string(startingQueueSize));
}

void LoadBalancer::maybeGenerateNewRequest() {
    std::uniform_int_distribution<int> chanceDist(1, 100);
    if (chanceDist(rng) > NEW_REQUEST_CHANCE) return;

    Request req = createRandomRequest();
    if (isBlocked(req.sourceIp)) {
        totalRejected++;
        logEvent("[Cycle " + std::to_string(currentCycle) + "] FIREWALL BLOCKED IP: " + req.sourceIp, RED);
        return;
    }

    requestQueue.push(req);
    totalGenerated++;
    maxQueueObserved = std::max(maxQueueObserved, requestQueue.size());
    logEvent("[Cycle " + std::to_string(currentCycle) + "] NEW REQUEST added. Time: " + std::to_string(req.processingTime), GREEN);
}

void LoadBalancer::assignRequestsToIdleServers() {
    for (auto& server : servers) {
        if (requestQueue.empty()) break;

        if (!server.isBusy()) {
            Request nextReq = requestQueue.front();
            requestQueue.pop();
            server.assign(nextReq);
        }
    }
}

void LoadBalancer::tickServers() {
    for (auto& server : servers) {
        if (!server.isBusy()) continue;
        if (server.tick()) totalCompleted++;
    }
}

void LoadBalancer::maybeScale() {
    if (cooldownCyclesRemaining > 0) {
        cooldownCyclesRemaining--;
        return;
    }

    int numServers = servers.size();
    if (numServers <= 0) return;
    int qSize = requestQueue.size();

    if (qSize > (UPPER_QUEUE_MULT * numServers)) {
        servers.emplace_back(nextServerId++);
        scaleUps++;
        cooldownCyclesRemaining = config.cooldownTime;
        logEvent("[Cycle " + std::to_string(currentCycle) + "] SCALING UP. Total servers: " + std::to_string(servers.size()), YELLOW);
        return;
    }

    if (qSize < (LOWER_QUEUE_MULT * numServers) && numServers > 1) {
        for (auto it = servers.begin(); it != servers.end(); ++it) {
            if (!it->isBusy()) {
                logEvent("[Cycle " + std::to_string(currentCycle) + "] SCALING DOWN. Removed Server ID: " + std::to_string(it->getId()), YELLOW);
                servers.erase(it);
                scaleDowns++;
                cooldownCyclesRemaining = config.cooldownTime;
                return;
            }
        }
    }
}

void LoadBalancer::logSnapshot() {
    if (currentCycle % 500 == 0) {
        logEvent("[Cycle " + std::to_string(currentCycle) + "] STATUS: Queue=" + std::to_string(requestQueue.size()) + 
                 " Servers=" + std::to_string(servers.size()), CYAN);
    }
}

void LoadBalancer::run() {
    generateInitialQueue();

    for (currentCycle = 1; currentCycle <= totalSimulationCycles; ++currentCycle) {
        maybeGenerateNewRequest();
        assignRequestsToIdleServers();
        tickServers();
        maybeScale();
        logSnapshot();
    }

    // Calculate Active vs Inactive servers for the final rubric requirement
    int activeCount = 0;
    int inactiveCount = 0;
    for (const auto& server : servers) {
        if (server.isBusy()) activeCount++;
        else inactiveCount++;
    }

    logEvent("\n==== Final Simulation Summary ====", CYAN);
    logEvent("Starting queue size: " + std::to_string(startingQueueSize));
    logEvent("Ending queue size: " + std::to_string(requestQueue.size()));
    logEvent("Task time range: " + std::to_string(config.minTaskTime) + " to " + std::to_string(config.maxTaskTime) + " cycles");
    logEvent("Total generated: " + std::to_string(totalGenerated));
    logEvent("Total completed: " + std::to_string(totalCompleted));
    logEvent("Total rejected by firewall: " + std::to_string(totalRejected));
    logEvent("Server scale ups: " + std::to_string(scaleUps));
    logEvent("Server scale downs: " + std::to_string(scaleDowns));
    logEvent("Active servers (busy): " + std::to_string(activeCount));
    logEvent("Inactive servers (idle): " + std::to_string(inactiveCount));
    logEvent("Total remaining servers: " + std::to_string(servers.size()));
}