#ifndef LOADBALANCER_H
#define LOADBALANCER_H

#include "WebServer.h"
#include "Logger.h"
#include "Firewall.h"
#include <queue>
#include <random>
#include <string>
#include <vector>

/**
 * @brief Manages a queue of requests and a dynamic pool of web servers.
 */
class LoadBalancer {
public:
    struct Config {
        int initialQueueMultiplier = 100;
        int minTaskTime = 2;
        int maxTaskTime = 15;
        int cooldownCycles = 25;
        int newRequestChancePercent = 30;

        int lowerQueuePerServer = 50;
        int upperQueuePerServer = 80;

        int snapshotEvery = 50;
        std::string logFile = "loadbalancer.log";
    };

    LoadBalancer(int initialServers,
                 int simulationCycles,
                 const Config& configValues,
                 const Firewall& firewallRules);
    
    void run();

private:
    // simulation helpers
    void generateInitialQueue();
    void maybeGenerateNewRequest();
    Request createRandomRequest();
    std::string createRandomIp();
    void assignRequestsToIdleServers();
    void tickServers();
    void maybeScale();
    void logSnapshot();

    // config & state
    int totalSimulationCycles;
    long long currentCycle;

    Config config;

    std::queue<Request> requestQueue;
    std::vector<WebServer> servers;
    int nextServerId;

    int cooldownCyclesRemaining;

    Firewall firewall;
    Logger logger;

    // stats
    size_t startingQueueSize;
    long long totalGenerated;
    long long totalRejected;
    long long totalCompleted;
    long long scaleUps;
    long long scaleDowns;
    size_t maxQueueObserved;

    // rng
    std::mt19937 rng;
};

#endif
