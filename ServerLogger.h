// ServerLogger.h
#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <mutex>

class ServerLogger {
private:
    std::ofstream logFile;
    bool consoleOutput;
    std::mutex logMutex;

    std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

public:
    enum class Level {
        INFO,
        WARNING,
        ERROR,
        DEBUG
    };

    ServerLogger(const std::string& filename, bool outputToConsole = true)
        : consoleOutput(outputToConsole)
    {
        logFile.open(filename, std::ios::out | std::ios::app);
    }

    ~ServerLogger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    void log(Level level, const std::string& message) {
        std::string levelStr;
        switch (level) {
        case Level::INFO:    levelStr = "INFO"; break;
        case Level::WARNING: levelStr = "WARNING"; break;
        case Level::ERROR:   levelStr = "ERROR"; break;
        case Level::DEBUG:   levelStr = "DEBUG"; break;
        }

        std::string formattedMsg = getTimestamp() + " [" + levelStr + "] " + message;

        std::lock_guard<std::mutex> lock(logMutex);

        if (logFile.is_open()) {
            logFile << formattedMsg << std::endl;
        }

        if (consoleOutput) {
            std::cout << formattedMsg << std::endl;
        }
    }

    void info(const std::string& message) {
        log(Level::INFO, message);
    }

    void warning(const std::string& message) {
        log(Level::WARNING, message);
    }

    void error(const std::string& message) {
        log(Level::ERROR, message);
    }

    void debug(const std::string& message) {
        log(Level::DEBUG, message);
    }
};