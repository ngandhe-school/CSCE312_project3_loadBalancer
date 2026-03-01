#ifndef FIREWALL_H
#define FIREWALL_H

#include <string>
#include <vector>
#include <cstdint>

class Firewall {
public:
    void addBlockedRange(const std::string& startIp, const std::string& endIp);
    bool isBlocked(const std::string& ip) const;

private:
    struct Range {
        uint32_t start;
        uint32_t end;
    };

    static uint32_t ipToUint(const std::string& ip);
    std::vector<Range> blocked;
};

#endif