#include "LoadBalancer.h"
#include <algorithm>
#include <sstream>

LoadBalancer::LoadBalancer(int initialServers,
                           int simulationCycles,
                           const Config& configValues,
                           const Firewall& firewallRules)
    : totalSimulationCycles(simulationCycles),
      currentCycle(0),
      config(configValues),
      nextServerId(1),
      cooldownCyclesRemaining(0),
      firewall(firewallRules),
      logger(config.logFile),
      startingQueueSize(0),
      totalGenerated(0),
      totalRejected(0),
      totalCompleted(0),
      scaleUps(0),
      scaleDowns(0),
      maxQueueObserved(0),
      rng(std::random_device{}()) {
    servers.reserve(initialServers + 50);
    for (int i = 0; i < initialServers; ++i) {
        servers.emplace_back(nextServerId++);
    }

    logger.header("Load Balancer Simulation Started");
}

std::string LoadBalancer::createRandomIp() {
    std::uniform_int_distribution<int> octetDistribution(1, 254);
    return std::to_string(octetDistribution(rng)) + "." +
           std::to_string(octetDistribution(rng)) + "." +
           std::to_string(octetDistribution(rng)) + "." +
           std::to_string(octetDistribution(rng));
}

Request LoadBalancer::createRandomRequest() {
    std::uniform_int_distribution<int> taskTimeDistribution(
        config.minTaskTime,
        config.maxTaskTime);
    std::uniform_int_distribution<int> jobTypeDistribution(0, 1);

    Request request;
    request.sourceIp = createRandomIp();
    request.destinationIp = createRandomIp();
    request.processingTime = taskTimeDistribution(rng);
    request.jobType = jobTypeDistribution(rng) == 0 ? 'S' : 'P';
    request.createdCycle = currentCycle;

    return request;
}

void LoadBalancer::generateInitialQueue() {
    const int initialQueueTarget =
        static_cast<int>(servers.size()) * config.initialQueueMultiplier;

    for (int i = 0; i < initialQueueTarget; ++i) {
        Request request = createRandomRequest();

        if (firewall.isBlocked(request.sourceIp)) {
            totalRejected++;
            continue;
        }

        requestQueue.push(request);
        totalGenerated++;
    }

    startingQueueSize = requestQueue.size();
    maxQueueObserved = std::max(maxQueueObserved, requestQueue.size());

    logger.line("Initial servers: " + std::to_string(servers.size()));
    logger.line("Initial queue size: " + std::to_string(startingQueueSize));
}

void LoadBalancer::maybeGenerateNewRequest() {
    std::uniform_int_distribution<int> chanceDistribution(1, 100);
    if (chanceDistribution(rng) > config.newRequestChancePercent) {
        return;
    }

    Request request = createRandomRequest();
    if (firewall.isBlocked(request.sourceIp)) {
        totalRejected++;
        logger.line("[C" + std::to_string(currentCycle) +
                    "] FIREWALL REJECT sourceIp=" + request.sourceIp);
        return;
    }

    requestQueue.push(request);
    totalGenerated++;
    maxQueueObserved = std::max(maxQueueObserved, requestQueue.size());

    logger.line("[C" + std::to_string(currentCycle) + "] NEW REQUEST queued time=" +
                std::to_string(request.processingTime) +
                " type=" + std::string(1, request.jobType));
}

void LoadBalancer::assignRequestsToIdleServers() {
    for (auto& server : servers) {
        if (requestQueue.empty()) {
            break;
        }

        if (!server.isBusy()) {
            const Request nextRequest = requestQueue.front();
            requestQueue.pop();
            server.assign(nextRequest);

            logger.line("[C" + std::to_string(currentCycle) + "] ASSIGN server=" +
                        std::to_string(server.getId()) +
                        " time=" + std::to_string(nextRequest.processingTime) +
                        " type=" + std::string(1, nextRequest.jobType));
        }
    }
}

void LoadBalancer::tickServers() {
    for (auto& server : servers) {
        if (!server.isBusy()) {
            continue;
        }

        if (server.tick()) {
            totalCompleted++;
            logger.line("[C" + std::to_string(currentCycle) + "] COMPLETE server=" +
                        std::to_string(server.getId()));
        }
    }
}

void LoadBalancer::maybeScale() {
    if (cooldownCyclesRemaining > 0) {
        cooldownCyclesRemaining--;
        return;
    }

    const int serverCount = static_cast<int>(servers.size());
    if (serverCount <= 0) {
        return;
    }

    const int upperQueueThreshold = config.upperQueuePerServer * serverCount;
    const int lowerQueueThreshold = config.lowerQueuePerServer * serverCount;
    const int queueSize = static_cast<int>(requestQueue.size());

    if (queueSize > upperQueueThreshold) {
        servers.emplace_back(nextServerId++);
        scaleUps++;
        cooldownCyclesRemaining = config.cooldownCycles;

        logger.line("[C" + std::to_string(currentCycle) + "] SCALE UP -> servers=" +
                    std::to_string(servers.size()) +
                    " cooldown=" + std::to_string(cooldownCyclesRemaining));
        return;
    }

    if (queueSize >= lowerQueueThreshold || servers.size() <= 1) {
        return;
    }

    int indexToRemove = -1;
    for (int i = static_cast<int>(servers.size()) - 1; i >= 0; --i) {
        if (!servers[i].isBusy()) {
            indexToRemove = i;
            break;
        }
    }

    if (indexToRemove == -1) {
        indexToRemove = static_cast<int>(servers.size()) - 1;
    }

    const int removedServerId = servers[indexToRemove].getId();
    servers.erase(servers.begin() + indexToRemove);

    scaleDowns++;
    cooldownCyclesRemaining = config.cooldownCycles;

    logger.line("[C" + std::to_string(currentCycle) + "] SCALE DOWN removed=" +
                std::to_string(removedServerId) +
                " -> servers=" + std::to_string(servers.size()) +
                " cooldown=" + std::to_string(cooldownCyclesRemaining));
}

void LoadBalancer::logSnapshot() {
    if (config.snapshotEvery <= 0 || currentCycle % config.snapshotEvery != 0) {
        return;
    }

    int busyServerCount = 0;
    for (const auto& server : servers) {
        if (server.isBusy()) {
            busyServerCount++;
        }
    }

    std::ostringstream snapshotLine;
    snapshotLine << "[C" << currentCycle
                 << "] SNAPSHOT queue=" << requestQueue.size()
                 << " servers=" << servers.size()
                 << " busy=" << busyServerCount
                 << " cooldownLeft=" << cooldownCyclesRemaining
                 << " completed=" << totalCompleted
                 << " rejected=" << totalRejected;

    logger.line(snapshotLine.str());
}

void LoadBalancer::run() {
    generateInitialQueue();

    for (currentCycle = 1; currentCycle <= totalSimulationCycles; ++currentCycle) {
        maybeGenerateNewRequest();
        assignRequestsToIdleServers();
        tickServers();
        maybeScale();

        maxQueueObserved = std::max(maxQueueObserved, requestQueue.size());
        logSnapshot();
    }

    logger.header("Simulation Summary");
    logger.line("Starting queue size: " + std::to_string(startingQueueSize));
    logger.line("Ending queue size: " + std::to_string(requestQueue.size()));
    logger.line("Task time range: " + std::to_string(config.minTaskTime) +
                " to " + std::to_string(config.maxTaskTime));
    logger.line("Total generated (accepted into queue): " + std::to_string(totalGenerated));
    logger.line("Total completed: " + std::to_string(totalCompleted));
    logger.line("Total rejected (firewall): " + std::to_string(totalRejected));
    logger.line("Scale ups: " + std::to_string(scaleUps));
    logger.line("Scale downs: " + std::to_string(scaleDowns));
    logger.line("Max queue observed: " + std::to_string(maxQueueObserved));
    logger.line("Final active servers: " + std::to_string(servers.size()));

    logger.flush();
}
