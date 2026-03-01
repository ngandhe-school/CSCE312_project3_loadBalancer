#include "Firewall.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

uint32_t Firewall::ipToUint(const std::string& ip) {
    std::stringstream ss(ip);
    std::string octetText;
    uint32_t octets[4];
    int octetIndex = 0;

    while (std::getline(ss, octetText, '.')) {
        if (octetIndex >= 4) throw std::runtime_error("Invalid IP: " + ip);

        const int octetValue = std::stoi(octetText);
        if (octetValue < 0 || octetValue > 255) {
            throw std::runtime_error("Invalid IP: " + ip);
        }

        octets[octetIndex++] = static_cast<uint32_t>(octetValue);
    }
    if (octetIndex != 4) throw std::runtime_error("Invalid IP: " + ip);

    return (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
}

void Firewall::addBlockedRange(const std::string& startIp, const std::string& endIp) {
    Range newRange;
    newRange.start = ipToUint(startIp);
    newRange.end = ipToUint(endIp);

    if (newRange.start > newRange.end) {
        std::swap(newRange.start, newRange.end);
    }

    blocked.push_back(newRange);
}

bool Firewall::isBlocked(const std::string& ip) const {
    const uint32_t ipValue = ipToUint(ip);

    for (const auto& range : blocked) {
        if (ipValue >= range.start && ipValue <= range.end) {
            return true;
        }
    }

    return false;
}
