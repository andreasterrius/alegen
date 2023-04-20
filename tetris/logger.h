#pragma once

#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void log(const std::string& message) {
        std::ofstream log_file("log.txt", std::ios_base::app);
        auto time_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        log_file << std::ctime(&time_now) << ": " << message << std::endl;
    }

private:
    Logger() = default;
};

#define LOG(message) Logger::instance().log(message)