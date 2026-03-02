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

class LoadBalancer {
public:
    struct Config {
        int minTaskTime = 2;
        int maxTaskTime = 15;
        int cooldownTime = 25;
        int initialMultiplier = 100;
        int newRequestChance = 30;
        std::string blockedIpStart = "192.168.0.0";
        std::string blockedIpEnd = "192.168.255.255";
    };

    LoadBalancer(std::string typeName, int initialServers, const Config& configValues, std::ofstream* sharedLogFile = nullptr);
    
    void addRequest(const Request& req);
    void recordStartingQueue();
    void tickCycle(long long currentCycle);
    void printSummary() const;
    int getServerCount() const;

private:
    void assignRequestsToIdleServers(long long currentCycle);
    void tickServers(long long currentCycle);
    void maybeScale(long long currentCycle);
    void logEvent(const std::string& text, const std::string& colorCode = "") const;

    std::string name;
    Config config;
    std::ofstream* logFile; 

    std::queue<Request> requestQueue;
    std::vector<WebServer> servers;
    int nextServerId;
    int cooldownCyclesRemaining;

    size_t startingQueueSize;
    long long totalCompleted;
    long long scaleUps;
    long long scaleDowns;
    size_t maxQueueObserved;
    
    const int LOWER_QUEUE_MULT = 50;
    const int UPPER_QUEUE_MULT = 80;
};

#endif