#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <string>

class Logger {
public:
    explicit Logger(const std::string& filename);
    ~Logger();

    void header(const std::string& text);
    void line(const std::string& text);
    void flush();

private:
    std::ofstream out;
};

#endif