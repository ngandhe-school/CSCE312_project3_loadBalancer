#include "Logger.h"
#include <stdexcept>

Logger::Logger(const std::string& filename) {
    out.open(filename);
    if (!out.is_open()) throw std::runtime_error("Could not open log file: " + filename);
}

Logger::~Logger() {
    if (out.is_open()) out.close();
}

void Logger::header(const std::string& text) {
    out << "==== " << text << " ====\n";
}

void Logger::line(const std::string& text) {
    out << text << "\n";
}

void Logger::flush() {
    out.flush();
}