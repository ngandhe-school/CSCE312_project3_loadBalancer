#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "Request.h"

class WebServer {
public:
    explicit WebServer(int id);

    bool isBusy() const;
    int getId() const;

    bool assign(const Request& request);

    bool tick();

    const Request& currentRequest() const;

    int remainingTime() const;
    long long busyCycles() const;

private:
    int serverId;

    bool busy;
    Request activeRequest;  
    int remainingCycles;

    long long totalBusyCycles;
};

#endif
