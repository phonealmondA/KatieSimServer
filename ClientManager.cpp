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
    try {
        std::lock_guard<std::mutex> lock(clientsMutex);

        for (auto& pair : clients) {
            try {
                if (pair.second) {
                    delete pair.second;
                }
            }
            catch (const std::exception& e) {
                logger.error("Exception in ~ClientManager when deleting client: " + std::string(e.what()));
            }
        }
        clients.clear();
    }
    catch (const std::exception& e) {
        logger.error("Exception in ~ClientManager: " + std::string(e.what()));
    }
}

int ClientManager::addClient(sf::TcpSocket* socket)
{
    try {
        if (!socket) {
            logger.error("Attempted to add null socket to ClientManager");
            return -1;
        }

        std::lock_guard<std::mutex> lock(clientsMutex);

        int clientId = nextClientId++;
        ClientData* client = new ClientData(clientId, socket);
        clients[clientId] = client;

        sf::IpAddress address = sf::IpAddress::Any;
        if (socket->getRemoteAddress()) {
            address = *socket->getRemoteAddress();
        }

        std::stringstream ss;
        ss << "Client " << clientId << " connected from "
            << address.toString() << ":" << socket->getLocalPort();
        logger.info(ss.str());

        return clientId;
    }
    catch (const std::exception& e) {
        logger.error("Exception in addClient: " + std::string(e.what()));
        return -1;
    }
}

void ClientManager::removeClient(int clientId)
{
    try {
        std::lock_guard<std::mutex> lock(clientsMutex);

        auto it = clients.find(clientId);
        if (it != clients.end()) {
            std::stringstream ss;
            ss << "Client " << clientId << " disconnected";
            logger.info(ss.str());

            if (it->second) {
                delete it->second;
            }
            clients.erase(it);
        }
    }
    catch (const std::exception& e) {
        logger.error("Exception in removeClient: " + std::string(e.what()));
    }
}

void ClientManager::checkTimeouts()
{
    try {
        std::vector<int> timeoutClients;

        // Get list of timed out clients
        {
            std::lock_guard<std::mutex> lock(clientsMutex);

            for (auto& pair : clients) {
                if (pair.second && pair.second->isTimedOut(GameConstants::CLIENT_TIMEOUT)) {
                    timeoutClients.push_back(pair.first);
                }
            }
        }

        // Process timed out clients without the lock held
        for (int clientId : timeoutClients) {
            std::stringstream ss;
            ss << "Client " << clientId << " timed out";
            logger.warning(ss.str());
            removeClient(clientId);
        }
    }
    catch (const std::exception& e) {
        logger.error("Exception in checkTimeouts: " + std::string(e.what()));
    }
}

void ClientManager::sendToAll(sf::Packet& packet)
{
    try {
        std::lock_guard<std::mutex> lock(clientsMutex);

        for (auto& pair : clients) {
            if (!pair.second || !pair.second->socket) {
                continue;
            }

            if (pair.second->authenticated) {
                try {
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
                catch (const std::exception& e) {
                    logger.error("Exception sending to client " + std::to_string(pair.first) + ": " + std::string(e.what()));
                    pair.second->packetLoss++;
                }
            }
        }
    }
    catch (const std::exception& e) {
        logger.error("Exception in sendToAll: " + std::string(e.what()));
    }
}

void ClientManager::sendTo(int clientId, sf::Packet& packet)
{
    try {
        std::lock_guard<std::mutex> lock(clientsMutex);

        auto it = clients.find(clientId);
        if (it != clients.end() && it->second && it->second->socket) {
            try {
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
            catch (const std::exception& e) {
                logger.error("Exception sending to client " + std::to_string(clientId) + ": " + std::string(e.what()));
                it->second->packetLoss++;
            }
        }
    }
    catch (const std::exception& e) {
        logger.error("Exception in sendTo: " + std::string(e.what()));
    }
}

std::vector<int> ClientManager::getClientIds()
{
    try {
        std::lock_guard<std::mutex> lock(clientsMutex);

        std::vector<int> ids;
        for (auto& pair : clients) {
            ids.push_back(pair.first);
        }
        return ids;
    }
    catch (const std::exception& e) {
        logger.error("Exception in getClientIds: " + std::string(e.what()));
        return std::vector<int>();
    }
}

ClientData* ClientManager::getClient(int clientId)
{
    try {
        std::lock_guard<std::mutex> lock(clientsMutex);

        auto it = clients.find(clientId);
        if (it != clients.end()) {
            return it->second;
        }
        return nullptr;
    }
    catch (const std::exception& e) {
        logger.error("Exception in getClient: " + std::string(e.what()));
        return nullptr;
    }
}

int ClientManager::getClientCount()
{
    try {
        std::lock_guard<std::mutex> lock(clientsMutex);
        return static_cast<int>(clients.size());
    }
    catch (const std::exception& e) {
        logger.error("Exception in getClientCount: " + std::string(e.what()));
        return 0;
    }
}

void ClientManager::logClientInfo()
{
    try {
        std::lock_guard<std::mutex> lock(clientsMutex);

        std::stringstream ss;
        ss << "Connected clients: " << clients.size();
        logger.info(ss.str());

        for (auto& pair : clients) {
            if (!pair.second) {
                logger.warning("Null client found with ID " + std::to_string(pair.first));
                continue;
            }

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
    catch (const std::exception& e) {
        logger.error("Exception in logClientInfo: " + std::string(e.what()));
    }
}