/**
 * @file LoadBalancer.h
 * @brief Defines the LoadBalancer class which manages WebServers and a queue of Requests.
 */

#ifndef LOADBALANCER_H
#define LOADBALANCER_H

#include "WebServer.h"
#include <queue>
#include <vector>
#include <string>
#include <fstream>
#include <random>
#include <cstdint>

/**
 * @class LoadBalancer
 * @brief Manages a dynamic pool of web servers and routes incoming requests.
 */
class LoadBalancer {
public:
    struct Config {
        int minTaskTime = 2;
        int maxTaskTime = 15;
        int cooldownTime = 25;
        std::string blockedIpStart = "192.168.0.0";
        std::string blockedIpEnd = "192.168.255.255";
    };

    /**
     * @brief Constructs a LoadBalancer with a set number of initial servers and cycles.
     */
    LoadBalancer(int initialServers, int simulationCycles, const Config& configValues);
    
    ~LoadBalancer();
    
    void run();

private:
    void generateInitialQueue();
    void maybeGenerateNewRequest();
    Request createRandomRequest();
    std::string createRandomIp();
    void assignRequestsToIdleServers();
    void tickServers();
    void maybeScale();
    
    // Firewall Logic
    uint32_t ipToUint(const std::string& ip);
    bool isBlocked(const std::string& ip);
    
    // Logging Logic
    void logEvent(const std::string& text, const std::string& colorCode = "");
    void logSnapshot();

    int totalSimulationCycles;
    long long currentCycle;
    Config config;

    std::queue<Request> requestQueue;
    std::vector<WebServer> servers;
    int nextServerId;
    int cooldownCyclesRemaining;

    const int LOWER_QUEUE_MULT = 50;
    const int UPPER_QUEUE_MULT = 80;
    const int INITIAL_MULTIPLIER = 100;
    const int NEW_REQUEST_CHANCE = 30;

    // Firewall Range
    uint32_t firewallStartIp;
    uint32_t firewallEndIp;

    std::ofstream logFile;
    size_t startingQueueSize;
    long long totalGenerated;
    long long totalRejected;
    long long totalCompleted;
    long long scaleUps;
    long long scaleDowns;
    size_t maxQueueObserved;

    std::mt19937 rng;
};

#endif