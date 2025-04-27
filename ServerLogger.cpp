// ServerLogger.cpp
#include "ServerLogger.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <mutex>

std::string ServerLogger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

ServerLogger::ServerLogger(const std::string& filename, bool outputToConsole)
    : consoleOutput(outputToConsole)
{
    logFile.open(filename, std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Warning: Could not open log file: " << filename << std::endl;
    }
}

ServerLogger::~ServerLogger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void ServerLogger::log(Level level, const std::string& message) {
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
        logFile.flush();
    }

    if (consoleOutput) {
        std::cout << formattedMsg << std::endl;
    }
}

void ServerLogger::info(const std::string& message) {
    log(Level::INFO, message);
}

void ServerLogger::warning(const std::string& message) {
    log(Level::WARNING, message);
}

void ServerLogger::error(const std::string& message) {
    log(Level::ERROR, message);
}

void ServerLogger::debug(const std::string& message) {
    log(Level::DEBUG, message);
}