#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "Request.h"

/**
 * @brief Simulates a single web server that processes requests over time.
 */
class WebServer {
public:
    explicit WebServer(int id);

    bool isBusy() const;
    int getId() const;

    // Assign a request if idle
    bool assign(const Request& request);

    // Advance one cycle. Returns true if request completed this cycle.
    bool tick();

    // Access current request info (valid only if isBusy() == true)
    const Request& currentRequest() const;

    int remainingTime() const;
    long long busyCycles() const;

private:
    int serverId;

    bool busy;
    Request activeRequest;  // meaningful only when busy==true
    int remainingCycles;

    long long totalBusyCycles;
};

#endif
