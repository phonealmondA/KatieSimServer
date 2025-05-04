// ServerConfig.h
#pragma once
#include <string>
#include "GameConstants.h"
#include <SFML/Graphics.hpp>
class ServerConfig {
private:
    unsigned short port;
    int maxClients;
    float updateRate;
    bool verbose;
    std::string logFile;

public:
    ServerConfig()
        : port(GameConstants::DEFAULT_PORT),
        maxClients(GameConstants::MAX_CLIENTS),
        updateRate(GameConstants::SERVER_UPDATE_RATE),
        verbose(true),
        logFile("server_log.txt")
    {
    }

    unsigned short getPort() const { return port; }
    int getMaxClients() const { return maxClients; }
    float getUpdateRate() const { return updateRate; }
    bool isVerbose() const { return verbose; }
    const std::string& getLogFile() const { return logFile; }

    void setPort(unsigned short value) { port = value; }
    void setMaxClients(int value) { maxClients = value; }
    void setUpdateRate(float value) { updateRate = value; }
    void setVerbose(bool value) { verbose = value; }
    void setLogFile(const std::string& value) { logFile = value; }
};