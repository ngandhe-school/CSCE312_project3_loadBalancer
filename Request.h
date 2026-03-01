#ifndef REQUEST_H
#define REQUEST_H

#include <string>

struct Request {
    std::string sourceIp;
    std::string destinationIp;
    int processingTime;     
    char jobType;           
    long long createdCycle; 
};

#endif
