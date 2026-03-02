/**
 * @file Request.h
 * @brief Defines the Request struct, representing a job to be processed by a WebServer.
 */

#ifndef REQUEST_H
#define REQUEST_H

#include <string>

/**
 * @struct Request
 * @brief Encapsulates all data related to a single network request.
 */
struct Request {
    std::string sourceIp;      ///< The IP address the request originated from.
    std::string destinationIp; ///< The IP address the request is attempting to reach.
    int processingTime;        ///< How many clock cycles this request requires to finish.
    char jobType;              ///< 'P' for Processing, 'S' for Streaming.
    long long createdCycle;    ///< The clock cycle when this request was created.
};

#endif