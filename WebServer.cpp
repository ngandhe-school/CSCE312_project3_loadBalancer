/**
 * @file WebServer.cpp
 * @brief Implements the WebServer class behavior.
 */

#include "WebServer.h"
#include <stdexcept>

/**
 * @brief Constructs a WebServer with default idle state.
 * @param id Unique server identifier.
 */
WebServer::WebServer(int id)
    : serverId(id),
      busy(false),
      activeRequest(),
      remainingCycles(0),
      totalBusyCycles(0) {}

/**
 * @brief Indicates whether the server is currently processing a request.
 * @return True if busy; otherwise false.
 */
bool WebServer::isBusy() const { return busy; }

/**
 * @brief Gets this server's identifier.
 * @return Server ID.
 */
int WebServer::getId() const { return serverId; }

/**
 * @brief Assigns a request to the server if idle.
 * @param request Request to assign.
 * @return True on success, false if server is already busy.
 */
bool WebServer::assign(const Request& request) {
    if(busy){
        return false;
    }
    
    activeRequest = request;
    remainingCycles = request.processingTime;
    busy = true;
    return true;
}

/**
 * @brief Advances processing by one cycle.
 * @return True if the active request completed during this tick, else false.
 */
bool WebServer::tick() {
    if(!busy){
        return false;
    }

    remainingCycles--;
    totalBusyCycles++;

    if (remainingCycles <= 0) {
        busy = false;
        remainingCycles = 0;
        return true; 
    }
    return false;
}

/**
 * @brief Gets the currently active request.
 * @return Const reference to active request.
 * @throws std::runtime_error if server is idle.
 */
const Request& WebServer::currentRequest() const {
    if(!busy){
        throw std::runtime_error("currentRequest() called while server is idle");
    }
    return activeRequest;
}

/**
 * @brief Returns remaining cycles for active request.
 * @return Remaining cycles; 0 if idle.
 */
int WebServer::remainingTime() const { return remainingCycles; }

/**
 * @brief Returns accumulated busy-cycle count.
 * @return Total cycles spent processing requests.
 */
long long WebServer::busyCycles() const { return totalBusyCycles; }
