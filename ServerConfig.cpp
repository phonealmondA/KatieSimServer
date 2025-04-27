// ServerConfig.cpp
#include "ServerConfig.h"

// The implementation is already in the header file
// This file exists to satisfy the build system

ServerConfig::ServerConfig()
    : port(GameConstants::DEFAULT_PORT),
    maxClients(GameConstants::MAX_CLIENTS),
    updateRate(GameConstants::SERVER_UPDATE_RATE),
    verbose(true),
    logFile("server_log.txt")
{
}

unsigned short ServerConfig::getPort() const {
    return port;
}

int ServerConfig::getMaxClients() const {
    return maxClients;
}

float ServerConfig::getUpdateRate() const {
    return updateRate;
}

bool ServerConfig::isVerbose() const {
    return verbose;
}

const std::string& ServerConfig::getLogFile() const {
    return logFile;
}

void ServerConfig::setPort(unsigned short value) {
    port = value;
}

void ServerConfig::setMaxClients(int value) {
    maxClients = value;
}

void ServerConfig::setUpdateRate(float value) {
    updateRate = value;
}

void ServerConfig::setVerbose(bool value) {
    verbose = value;
}

void ServerConfig::setLogFile(const std::string& value) {
    logFile = value;
}