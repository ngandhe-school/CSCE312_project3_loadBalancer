/**
 * @file WebServer.h
 * @brief Defines the WebServer class, acting as a worker node to process Requests.
 */

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "Request.h"

/**
 * @class WebServer
 * @brief Represents a single server instance capable of processing one Request at a time.
 */
class WebServer {
public:
    /**
     * @brief Constructs a new WebServer with a unique ID.
     * @param id The unique identifier for this server.
     */
    explicit WebServer(int id);

    /** @brief Checks if the server is currently processing a request. */
    bool isBusy() const;
    
    /** @brief Gets the server's unique ID. */
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

    /** @brief Returns the request currently being processed. */
    const Request& currentRequest() const;

    /** @brief Returns the number of cycles remaining for the current request. */
    int remainingTime() const;
    
    /** @brief Returns the total number of cycles this server has spent working. */
    long long busyCycles() const;

private:
    int serverId;
    bool busy;
    Request activeRequest;  
    int remainingCycles;
    long long totalBusyCycles;
};

#endif