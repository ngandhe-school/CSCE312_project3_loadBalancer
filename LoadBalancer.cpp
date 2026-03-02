/**
 * @file LoadBalancer.cpp
 * @brief Implementation of the LoadBalancer class.
 */

#include "LoadBalancer.h"
#include <iostream>
#include <algorithm>

/** @brief ANSI reset color code for console logging. */
const std::string RESET = "\033[0m";
/** @brief ANSI green color code for informational events. */
const std::string GREEN = "\033[32m";
/** @brief ANSI yellow color code for scaling events. */
const std::string YELLOW = "\033[33m";
/** @brief ANSI cyan color code for status and summaries. */
const std::string CYAN = "\033[36m";

/**
 * @brief Constructs a specialized load balancer instance.
 * @param typeName Human-readable name used in logs ("Processing"/"Streaming").
 * @param initialServers Initial number of worker servers to create.
 * @param configValues Runtime configuration values.
 * @param sharedLogFile Optional shared log stream owned by the switch.
 */
LoadBalancer::LoadBalancer(std::string typeName, int initialServers, const Config& configValues, std::ofstream* sharedLogFile)
    : name(typeName),
      config(configValues),
      logFile(sharedLogFile),
      nextServerId(1),
      cooldownCyclesRemaining(0),
      startingQueueSize(0),
      totalCompleted(0),
      scaleUps(0),
      scaleDowns(0),
      maxQueueObserved(0) {
    
    for (int i = 0; i < initialServers; ++i) {
        servers.emplace_back(nextServerId++);
    }
}

/**
 * @brief Returns current server count.
 * @return Number of WebServer instances in this balancer.
 */
int LoadBalancer::getServerCount() const {
    return servers.size();
}

/**
 * @brief Writes an event to console and shared log (if available).
 * @param text Event message.
 * @param colorCode ANSI color code for console output.
 */
void LoadBalancer::logEvent(const std::string& text, const std::string& colorCode) const {
    std::cout << colorCode << "[" << name << "] " << text << RESET << "\n";

    if (logFile && logFile->is_open()) {
        *logFile << "[" << name << "] " << text << "\n";
    }
}

/**
 * @brief Adds a request to the balancer queue and records enqueue event.
 * @param req Request to enqueue.
 */
void LoadBalancer::addRequest(const Request& req) {
    requestQueue.push(req);
    maxQueueObserved = std::max(maxQueueObserved, requestQueue.size());
    logEvent("[Cycle " + std::to_string(req.createdCycle) + "] NEW REQUEST Queued. Task Time: " + std::to_string(req.processingTime));
}

/**
 * @brief Records queue state immediately after initial request generation.
 */
void LoadBalancer::recordStartingQueue() {
    startingQueueSize = requestQueue.size();
    maxQueueObserved = std::max(maxQueueObserved, startingQueueSize);
    logEvent("Initial servers: " + std::to_string(servers.size()), CYAN);
    logEvent("Initial queue size: " + std::to_string(startingQueueSize), CYAN);
}

/**
 * @brief Assigns queued requests to any idle servers.
 * @param currentCycle Current cycle used for assignment logs.
 */
void LoadBalancer::assignRequestsToIdleServers(long long currentCycle) {
    for (auto& server : servers) {
        if (requestQueue.empty()) break;

        if (!server.isBusy()) {
            Request nextReq = requestQueue.front();
            requestQueue.pop();
            server.assign(nextReq);
            logEvent("[Cycle " + std::to_string(currentCycle) + "] ASSIGNED Request to Server ID: " + std::to_string(server.getId()));
        }
    }
}

/**
 * @brief Advances all busy servers by one cycle and tracks completions.
 * @param currentCycle Current cycle used for completion logs.
 */
void LoadBalancer::tickServers(long long currentCycle) {
    for (auto& server : servers) {
        if (!server.isBusy()) continue;
        if (server.tick()) {
            totalCompleted++;
            logEvent("[Cycle " + std::to_string(currentCycle) + "] COMPLETED Request on Server ID: " + std::to_string(server.getId()));
        }
    }
}

/**
 * @brief Applies autoscaling rules based on queue pressure and cooldown.
 * @param currentCycle Current cycle used for scaling logs.
 *
 * @details
 * Rules:
 * - Scale up if queue size exceeds upper threshold (80 * servers).
 * - Scale down if queue size drops below lower threshold (50 * servers),
 *   removing an idle server when possible.
 * - A cooldown delay is enforced between scaling actions.
 * - If server count is zero and queue has work, bootstrap one server.
 */
void LoadBalancer::maybeScale(long long currentCycle) {
    if (cooldownCyclesRemaining > 0) {
        cooldownCyclesRemaining--;
        return;
    }

    int numServers = servers.size();
    int qSize = requestQueue.size();

    // Recovery path: if this balancer has queued work but zero servers, bootstrap one.
    if (numServers == 0) {
        if (qSize > 0) {
            servers.emplace_back(nextServerId++);
            scaleUps++;
            cooldownCyclesRemaining = config.cooldownTime;
            logEvent("[Cycle " + std::to_string(currentCycle) + "] SCALING UP. Total servers: " + std::to_string(servers.size()), YELLOW);
        }
        return;
    }

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

/**
 * @brief Executes one complete cycle for this load balancer.
 * @param currentCycle Global cycle index.
 */
void LoadBalancer::tickCycle(long long currentCycle) {
    assignRequestsToIdleServers(currentCycle);
    tickServers(currentCycle);
    maybeScale(currentCycle);

    if (currentCycle % 1000 == 0) {
        logEvent("[Cycle " + std::to_string(currentCycle) + "] STATUS: Queue=" + std::to_string(requestQueue.size()) + 
                 " Servers=" + std::to_string(servers.size()), CYAN);
    }
}

/**
 * @brief Prints and logs final summary metrics for this balancer.
 */
void LoadBalancer::printSummary() const {
    int activeCount = 0;
    int inactiveCount = 0;
    for (const auto& server : servers) {
        if (server.isBusy()) activeCount++;
        else inactiveCount++;
    }

    std::cout << CYAN << "--- " << name << " Load Balancer Status ---" << RESET << "\n";
    std::cout << "Starting queue size: " << startingQueueSize << "\n";
    std::cout << "Ending queue size: " << requestQueue.size() << "\n";
    std::cout << "Task time range: " << config.minTaskTime << " to " << config.maxTaskTime << " cycles\n";
    std::cout << "Max queue observed: " << maxQueueObserved << "\n";
    std::cout << "Total completed: " << totalCompleted << "\n";
    std::cout << "Server scale ups: " << scaleUps << "\n";
    std::cout << "Server scale downs: " << scaleDowns << "\n";
    std::cout << "Active servers (busy): " << activeCount << "\n";
    std::cout << "Inactive servers (idle): " << inactiveCount << "\n";
    std::cout << "Total remaining servers: " << servers.size() << "\n\n";

    if (logFile && logFile->is_open()) {
        *logFile << "--- " << name << " Load Balancer Status ---\n";
        *logFile << "Starting queue size: " << startingQueueSize << "\n";
        *logFile << "Ending queue size: " << requestQueue.size() << "\n";
        *logFile << "Task time range: " << config.minTaskTime << " to " << config.maxTaskTime << " cycles\n";
        *logFile << "Max queue observed: " << maxQueueObserved << "\n";
        *logFile << "Total completed: " << totalCompleted << "\n";
        *logFile << "Server scale ups: " << scaleUps << "\n";
        *logFile << "Server scale downs: " << scaleDowns << "\n";
        *logFile << "Active servers (busy): " << activeCount << "\n";
        *logFile << "Inactive servers (idle): " << inactiveCount << "\n";
        *logFile << "Total remaining servers: " << servers.size() << "\n\n";
    }
}
