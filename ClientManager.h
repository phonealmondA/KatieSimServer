// ClientManager.h
#pragma once
#include <SFML/Network.hpp>
#include <vector>
#include <map>
#include <mutex>
#include "ClientData.h"
#include "ServerLogger.h"
#include "ServerConfig.h"

class ClientManager {
private:
    std::map<int, ClientData*> clients;
    std::mutex clientsMutex;
    ServerLogger& logger;
    ServerConfig& config;
    int nextClientId;

public:
    ClientManager(ServerLogger& logger, ServerConfig& config);
    ~ClientManager();

    int addClient(sf::TcpSocket* socket);
    void removeClient(int clientId);
    void checkTimeouts();
    void sendToAll(sf::Packet& packet);
    void sendTo(int clientId, sf::Packet& packet);
    std::vector<int> getClientIds();
    ClientData* getClient(int clientId);
    int getClientCount();

    void logClientInfo();
};