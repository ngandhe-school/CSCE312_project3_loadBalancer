/**
 * @file Switch.h
 * @brief Declares the top-level Switch that routes requests to specialized load balancers.
 */

#ifndef SWITCH_H
#define SWITCH_H

#include "LoadBalancer.h"
#include <string>
#include <random>
#include <cstdint>
#include <fstream>

/**
 * @class Switch
 * @brief Top-level request router that coordinates two specialized load balancers.
 *
 * @details
 * The Switch is responsible for generating random requests, applying firewall
 * filtering, routing by job type, and advancing both child balancers each cycle.
 */
class Switch {
public:
    /**
     * @brief Constructs the top-level switch controller.
     * @param initialServersTotal Total initial servers across both balancers.
     * @param simulationCycles Total number of cycles to execute.
     * @param configValues Shared configuration for request generation and scaling.
     */
    Switch(int initialServersTotal, int simulationCycles, const LoadBalancer::Config& configValues);

    /**
     * @brief Flushes and closes owned resources such as the shared log file.
     */
    ~Switch();

    /**
     * @brief Runs the full simulation loop and prints global/final summaries.
     */
    void run();

private:
    /**
     * @brief Generates initial request population and routes by job type.
     */
    void generateInitialQueue();

    /**
     * @brief Randomly generates a new request for the current cycle.
     */
    void maybeGenerateNewRequest();

    /**
     * @brief Creates a randomized request object.
     * @return Newly generated Request.
     */
    Request createRandomRequest();

    /**
     * @brief Creates a randomized IPv4 address string.
     * @return Random IP in dotted-decimal format.
     */
    std::string createRandomIp();

    /**
     * @brief Converts dotted-decimal IPv4 string to uint32 representation.
     * @param ip IPv4 string in A.B.C.D format.
     * @return 32-bit numeric IP value.
     */
    uint32_t ipToUint(const std::string& ip);

    /**
     * @brief Checks whether an IP falls inside the configured blocked range.
     * @param ip IPv4 string to evaluate.
     * @return True if blocked by firewall, false otherwise.
     */
    bool isBlocked(const std::string& ip);

    std::ofstream logFile;     ///< Shared simulation log stream.
    LoadBalancer processingLB; ///< Balancer dedicated to 'P' processing jobs.
    LoadBalancer streamingLB;  ///< Balancer dedicated to 'S' streaming jobs.

    int totalSimulationCycles;   ///< Total cycles to execute.
    long long currentCycle;      ///< Current cycle counter.
    LoadBalancer::Config config; ///< Shared simulation configuration.

    uint32_t firewallStartIp; ///< Inclusive firewall lower bound (numeric IP).
    uint32_t firewallEndIp;   ///< Inclusive firewall upper bound (numeric IP).
    long long totalRejected;  ///< Number of requests rejected by switch firewall.

    std::mt19937 rng; ///< Random number generator used by switch request creation.
};

#endif
