#ifndef REQUEST_H
#define REQUEST_H

#include <string>

/**
 * @brief Represents a web request entering the load balancer simulation.
 */
struct Request {
    std::string sourceIp;
    std::string destinationIp;
    int processingTime;      // total processing time
    char jobType;            // 'P' (processing) or 'S' (streaming)
    long long createdCycle;  // cycle when request was created
};

#endif
