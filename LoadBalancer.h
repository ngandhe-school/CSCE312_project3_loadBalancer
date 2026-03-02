/**
 * @file LoadBalancer.h
 * @brief Declares the LoadBalancer class used for request queueing and server scaling.
 */

#ifndef LOADBALANCER_H
#define LOADBALANCER_H

#include "WebServer.h"
#include <queue>
#include <vector>
#include <string>
#include <fstream>

/**
 * @class LoadBalancer
 * @brief Manages a request queue and scalable server pool for one job category.
 *
 * @details
 * The Switch owns two instances of this class:
 * - Processing LB for job type 'P'
 * - Streaming LB for job type 'S'
 *
 * Each instance independently assigns requests, advances server work, scales
 * up/down, and tracks summary statistics.
 */
class LoadBalancer {
public:
    /**
     * @struct Config
     * @brief Tunable simulation parameters consumed by each load balancer.
     */
    struct Config {
        int minTaskTime = 2;                    ///< Minimum request processing cycles.
        int maxTaskTime = 15;                   ///< Maximum request processing cycles.
        int cooldownTime = 25;                  ///< Cycles to wait between scaling actions.
        int initialMultiplier = 100;            ///< Initial queue target: totalServers * multiplier.
        int newRequestChance = 30;              ///< Percent chance [0,100] to generate request each cycle.
        std::string blockedIpStart = "192.168.0.0";   ///< Inclusive firewall lower bound.
        std::string blockedIpEnd = "192.168.255.255"; ///< Inclusive firewall upper bound.
    };

    /**
     * @brief Constructs a typed load balancer instance.
     * @param typeName Human-readable balancer name used in logs.
     * @param initialServers Number of initial WebServer instances.
     * @param configValues Simulation configuration values.
     * @param sharedLogFile Optional shared log stream owned externally (Switch).
     */
    LoadBalancer(std::string typeName, int initialServers, const Config& configValues, std::ofstream* sharedLogFile = nullptr);

    /**
     * @brief Adds a request into this balancer's queue.
     * @param req Request to enqueue.
     */
    void addRequest(const Request& req);

    /**
     * @brief Captures and logs the starting queue size after initialization.
     */
    void recordStartingQueue();

    /**
     * @brief Executes one simulation cycle for this load balancer.
     * @param currentCycle Current global cycle index.
     */
    void tickCycle(long long currentCycle);

    /**
     * @brief Prints final summary statistics to console and log.
     */
    void printSummary() const;

    /**
     * @brief Gets number of currently active server objects in this balancer.
     * @return Current server count.
     */
    int getServerCount() const;

private:
    /**
     * @brief Assigns queued requests to idle servers.
     * @param currentCycle Current cycle used for logging.
     */
    void assignRequestsToIdleServers(long long currentCycle);

    /**
     * @brief Advances all busy servers by one cycle and records completions.
     * @param currentCycle Current cycle used for logging.
     */
    void tickServers(long long currentCycle);

    /**
     * @brief Applies autoscaling policy based on queue-to-server thresholds.
     * @param currentCycle Current cycle used for logging.
     */
    void maybeScale(long long currentCycle);

    /**
     * @brief Emits a tagged event line to console and optional shared log.
     * @param text Message content.
     * @param colorCode ANSI color prefix for console output.
     */
    void logEvent(const std::string& text, const std::string& colorCode = "") const;

    std::string name;         ///< Balancer label ("Processing" or "Streaming").
    Config config;            ///< Per-balancer runtime configuration.
    std::ofstream* logFile;   ///< Shared log file stream owned by Switch.

    std::queue<Request> requestQueue; ///< FIFO queue of waiting requests.
    std::vector<WebServer> servers;   ///< Dynamic worker pool.
    int nextServerId;                 ///< Next ID assigned when adding a server.
    int cooldownCyclesRemaining;      ///< Remaining cooldown before next scale action.

    size_t startingQueueSize; ///< Queue size after initial request generation.
    long long totalCompleted; ///< Total requests completed by this balancer.
    long long scaleUps;       ///< Number of scale-up events.
    long long scaleDowns;     ///< Number of scale-down events.
    size_t maxQueueObserved;  ///< Peak queue size observed during run.

    const int LOWER_QUEUE_MULT = 50; ///< Lower queue threshold per server.
    const int UPPER_QUEUE_MULT = 80; ///< Upper queue threshold per server.
};

#endif
