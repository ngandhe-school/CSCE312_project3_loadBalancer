#include "WebServer.h"
#include <stdexcept>

WebServer::WebServer(int id)
    : serverId(id),
      busy(false),
      activeRequest(),
      remainingCycles(0),
      totalBusyCycles(0) {}

bool WebServer::isBusy() const { return busy; }
int WebServer::getId() const { return serverId; }

bool WebServer::assign(const Request& request) {
    if (busy) return false;
    activeRequest = request;
    remainingCycles = request.processingTime;
    busy = true;
    return true;
}

bool WebServer::tick() {
    if (!busy) return false;

    remainingCycles--;
    totalBusyCycles++;

    if (remainingCycles <= 0) {
        busy = false;
        remainingCycles = 0;
        return true; // completed
    }
    return false;
}

const Request& WebServer::currentRequest() const {
    if (!busy) throw std::runtime_error("currentRequest() called while server is idle");
    return activeRequest;
}

int WebServer::remainingTime() const { return remainingCycles; }
long long WebServer::busyCycles() const { return totalBusyCycles; }
