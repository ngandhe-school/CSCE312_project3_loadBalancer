/**
 * @file WebServer.h
 * @brief Declares the WebServer class used by the load balancer simulation.
 */

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "Request.h"

/**
 * @class WebServer
 * @brief Represents a single worker server that processes one request at a time.
 *
 * @details
 * Each WebServer can hold at most one active Request. The simulation calls
 * tick() once per clock cycle to advance processing. When the request's
 * remaining cycles reach zero, the server becomes idle again.
 */
class WebServer {
public:
    /**
     * @brief Constructs a new WebServer with a unique ID.
     * @param id The unique identifier for this server.
     */
    explicit WebServer(int id);

    /**
     * @brief Checks whether this server currently has an active request.
     * @return True if busy, false if idle.
     */
    bool isBusy() const;

    /**
     * @brief Gets this server's unique identifier.
     * @return Server ID assigned at construction.
     */
    int getId() const;

    /**
     * @brief Assigns a new request to the server.
     * @param request The Request object to process.
     * @return True if assigned successfully, false if the server is already busy.
     */
    bool assign(const Request& request);

    /**
     * @brief Advances the processing state by one clock cycle.
     * @return True if the request was completed during this tick, false otherwise.
     */
    bool tick();

    /**
     * @brief Returns the active request currently being processed.
     * @return Const reference to the active Request.
     * @throws std::runtime_error if called while the server is idle.
     */
    const Request& currentRequest() const;

    /**
     * @brief Gets remaining processing cycles for the active request.
     * @return Remaining cycles; returns 0 if idle.
     */
    int remainingTime() const;

    /**
     * @brief Gets lifetime busy-cycle count for this server.
     * @return Number of cycles this server has been busy.
     */
    long long busyCycles() const;

private:
    int serverId;            ///< Unique server ID.
    bool busy;               ///< Busy/idle state flag.
    Request activeRequest;   ///< Request currently assigned to this server.
    int remainingCycles;     ///< Remaining cycles for activeRequest.
    long long totalBusyCycles; ///< Total cycles spent in busy state.
};

#endif
