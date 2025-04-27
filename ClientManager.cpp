// ClientManager.cpp
#include "ClientManager.h"
#include <sstream>
#include "GameConstants.h"

ClientManager::ClientManager(ServerLogger& logger, ServerConfig& config)
    : logger(logger), config(config), nextClientId(1)
{
}

ClientManager::~ClientManager()
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto& pair : clients) {
        delete pair.second;
    }
    clients.clear();
}

int ClientManager::addClient(sf::TcpSocket* socket)
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    int clientId = nextClientId++;
    ClientData* client = new ClientData(clientId, socket);
    clients[clientId] = client;

    sf::IpAddress address = sf::IpAddress::Any;
    if (socket && socket->getRemoteAddress()) {
        address = *socket->getRemoteAddress();
    }

    std::stringstream ss;
    ss << "Client " << clientId << " connected from "
        << address.toString() << ":" << socket->getLocalPort();
    logger.info(ss.str());

    return clientId;
}

void ClientManager::removeClient(int clientId)
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    auto it = clients.find(clientId);
    if (it != clients.end()) {
        std::stringstream ss;
        ss << "Client " << clientId << " disconnected";
        logger.info(ss.str());

        delete it->second;
        clients.erase(it);
    }
}

void ClientManager::checkTimeouts()
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    std::vector<int> timeoutClients;

    for (auto& pair : clients) {
        if (pair.second->isTimedOut(GameConstants::CLIENT_TIMEOUT)) {
            timeoutClients.push_back(pair.first);
        }
    }

    // Release lock before calling removeClient to avoid deadlock
    clientsMutex.unlock();

    for (int clientId : timeoutClients) {
        std::stringstream ss;
        ss << "Client " << clientId << " timed out";
        logger.warning(ss.str());
        removeClient(clientId);
    }

    // Reacquire lock
    clientsMutex.lock();
}

void ClientManager::sendToAll(sf::Packet& packet)
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto& pair : clients) {
        if (pair.second->socket && pair.second->authenticated) {
            sf::Socket::Status status = pair.second->socket->send(packet);
            if (status != sf::Socket::Status::Done) {
                pair.second->packetLoss++;

                if (config.isVerbose()) {
                    std::stringstream ss;
                    ss << "Failed to send packet to client " << pair.first;
                    logger.warning(ss.str());
                }

                if (status == sf::Socket::Status::Disconnected) {
                    pair.second->pendingDisconnect = true;
                }
            }
        }
    }
}

void ClientManager::sendTo(int clientId, sf::Packet& packet)
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    auto it = clients.find(clientId);
    if (it != clients.end() && it->second->socket) {
        sf::Socket::Status status = it->second->socket->send(packet);
        if (status != sf::Socket::Status::Done) {
            it->second->packetLoss++;

            if (config.isVerbose()) {
                std::stringstream ss;
                ss << "Failed to send packet to client " << clientId;
                logger.warning(ss.str());
            }

            if (status == sf::Socket::Status::Disconnected) {
                it->second->pendingDisconnect = true;
            }
        }
    }
}

std::vector<int> ClientManager::getClientIds()
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    std::vector<int> ids;
    for (auto& pair : clients) {
        ids.push_back(pair.first);
    }
    return ids;
}

ClientData* ClientManager::getClient(int clientId)
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    auto it = clients.find(clientId);
    if (it != clients.end()) {
        return it->second;
    }
    return nullptr;
}

int ClientManager::getClientCount()
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    return static_cast<int>(clients.size());
}

void ClientManager::logClientInfo()
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    std::stringstream ss;
    ss << "Connected clients: " << clients.size();
    logger.info(ss.str());

    for (auto& pair : clients) {
        ClientData* client = pair.second;
        std::stringstream clientSs;
        clientSs << "Client " << client->clientId
            << " [" << client->username << "] from ";

        if (client->socket && client->socket->getRemoteAddress()) {
            clientSs << client->socket->getRemoteAddress()->toString();
        }
        else {
            clientSs << "unknown";
        }

        clientSs << " - Ping: " << client->pingMs << "ms"
            << " - Packet Loss: " << client->packetLoss;
        logger.info(clientSs.str());
    }
}