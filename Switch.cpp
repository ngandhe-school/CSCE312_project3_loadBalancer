/**
 * @file Switch.cpp
 * @brief Implementation of the Switch class.
 */

#include "Switch.h"
#include <iostream>
#include <sstream>

/** @brief ANSI red color code for blocked-request messages. */
const std::string RED = "\033[31m";
/** @brief ANSI cyan color code for headers and status messages. */
const std::string CYAN = "\033[36m";
/** @brief ANSI reset color code for console logging. */
const std::string RESET = "\033[0m";

/**
 * @brief Constructs the top-level switch and child load balancers.
 * @param initialServersTotal Total initial server count across both balancers.
 * @param simulationCycles Total number of simulation cycles to execute.
 * @param configValues Shared simulation configuration.
 *
 * @details
 * The constructor opens the shared log file, splits total servers between
 * processing and streaming balancers, and precomputes the firewall range.
 */
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

/**
 * @brief Closes shared log resources.
 */
Switch::~Switch() {
    if (logFile.is_open()) {
        logFile.flush();
        logFile.close();
    }
}

/**
 * @brief Converts dotted IPv4 text to 32-bit numeric format.
 * @param ip IP string in A.B.C.D form.
 * @return Numeric IPv4 representation, or 0 for parse failure.
 */
uint32_t Switch::ipToUint(const std::string& ip) {
    uint32_t a, b, c, d;
    if (sscanf(ip.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return 0;
    return (a << 24) | (b << 16) | (c << 8) | d;
}

/**
 * @brief Checks whether an IP falls within the blocked firewall range.
 * @param ip Source IP address.
 * @return True if blocked, false otherwise.
 */
bool Switch::isBlocked(const std::string& ip) {
    uint32_t ipValue = ipToUint(ip);
    return (ipValue >= firewallStartIp && ipValue <= firewallEndIp);
}

/**
 * @brief Creates a randomized source/destination IP component.
 * @return Random IPv4 string.
 *
 * @details
 * A small percentage of generated IPs are intentionally placed in the
 * blocked private range to exercise firewall behavior in logs/statistics.
 */
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

/**
 * @brief Creates a randomized request for the current cycle.
 * @return Fully populated Request object.
 */
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

/**
 * @brief Builds the initial request population and routes by job type.
 *
 * @details
 * Initial target is:
 * totalServersAcrossBothBalancers * config.initialMultiplier.
 * Blocked requests are counted as rejected and not queued.
 */
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

/**
 * @brief Possibly generates one new request during this cycle.
 *
 * @details
 * Request generation is probabilistic using config.newRequestChance.
 * Requests are firewall-filtered at switch level before routing to a child LB.
 */
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

/**
 * @brief Executes the full switch-driven simulation loop.
 *
 * @details
 * Each cycle:
 * - maybe generate/request-route one new request
 * - tick both child load balancers (processing and streaming)
 * Final output includes global firewall stats and per-balancer summaries.
 */
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
