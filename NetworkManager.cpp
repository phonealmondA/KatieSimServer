// NetworkManager.cpp
#include "NetworkManager.h"
#include <sstream>
#include <chrono>
#include <thread>

NetworkManager::NetworkManager(ClientManager& clientManager, ServerLogger& logger, ServerConfig& config)
    : clientManager(clientManager), logger(logger), config(config), running(false)
{
    // Initialize callback functions to nullptr to avoid calling uninitialized functions
    onPlayerInputReceived = nullptr;
    onClientAuthenticated = nullptr;
    onClientDisconnected = nullptr;
}

NetworkManager::~NetworkManager()
{
    stop();
}

bool NetworkManager::start()
{
    std::lock_guard<std::mutex> lock(networkMutex);

    if (running.load()) {
        logger.warning("Network manager already running");
        return false;
    }

    // Start listening for connections
    try {
        if (listener.listen(config.getPort()) != sf::Socket::Status::Done) {
            std::stringstream ss;
            ss << "Failed to bind to port " << config.getPort();
            logger.error(ss.str());
            return false;
        }

        listener.setBlocking(false);

        std::stringstream ss;
        ss << "Server started on port " << config.getPort();
        logger.info(ss.str());

        // Log local IP for convenience
        auto localIpOpt = sf::IpAddress::getLocalAddress();
        if (localIpOpt) {
            logger.info("Local IP address: " + localIpOpt->toString());
        }
        else {
            logger.warning("Could not determine local IP address");
        }

        try {
            auto publicIpOpt = sf::IpAddress::getPublicAddress(sf::seconds(2));
            if (publicIpOpt) {
                logger.info("Public IP address: " + publicIpOpt->toString());
            }
            else {
                logger.warning("Could not determine public IP address");
            }
        }
        catch (const std::exception& e) {
            logger.warning("Could not determine public IP address: " + std::string(e.what()));
        }
        catch (...) {
            logger.warning("Could not determine public IP address: unknown error");
        }

        running.store(true);

        // Start accept thread
        acceptThread = std::thread(&NetworkManager::acceptClientConnections, this);

        // Start receive thread
        receiveThread = std::thread(&NetworkManager::receiveClientMessages, this);

        return true;
    }
    catch (const std::exception& e) {
        logger.error("Exception in start: " + std::string(e.what()));
        return false;
    }
    catch (...) {
        logger.error("Unknown exception in start");
        return false;
    }
}

void NetworkManager::stop()
{
    {
        std::lock_guard<std::mutex> lock(networkMutex);

        if (!running.load()) {
            return;
        }

        running.store(false);

        try {
            listener.close();
        }
        catch (const std::exception& e) {
            logger.warning("Exception when closing listener: " + std::string(e.what()));
        }
    }

    // Wait for threads to finish
    try {
        if (acceptThread.joinable()) {
            acceptThread.join();
        }

        if (receiveThread.joinable()) {
            receiveThread.join();
        }
    }
    catch (const std::exception& e) {
        logger.warning("Exception when joining threads: " + std::string(e.what()));
    }

    logger.info("Network manager stopped");
}

bool NetworkManager::sendGameState(const GameState& state)
{
    if (!running.load()) {
        return false;
    }

    try {
        sf::Packet packet;
        packet << static_cast<uint32_t>(MessageType::GAME_STATE) << state;

        clientManager.sendToAll(packet);
        return true;
    }
    catch (const std::exception& e) {
        logger.error("Exception in sendGameState: " + std::string(e.what()));
        return false;
    }
}

bool NetworkManager::sendPlayerIdentity(int clientId)
{
    if (!running.load()) {
        return false;
    }

    try {
        sf::Packet packet;
        packet << static_cast<uint32_t>(MessageType::PLAYER_ID) << static_cast<uint32_t>(clientId);

        clientManager.sendTo(clientId, packet);
        return true;
    }
    catch (const std::exception& e) {
        logger.error("Exception in sendPlayerIdentity: " + std::string(e.what()));
        return false;
    }
}

void NetworkManager::setPlayerInputCallback(std::function<void(int clientId, const PlayerInput&)> callback)
{
    this->onPlayerInputReceived = callback;
}

void NetworkManager::setClientAuthenticatedCallback(std::function<void(int clientId, const std::string&)> callback)
{
    this->onClientAuthenticated = callback;
}

void NetworkManager::setClientDisconnectedCallback(std::function<void(int clientId)> callback)
{
    this->onClientDisconnected = callback;
}

int NetworkManager::findNextAvailablePlayerId() {
    // Get current client IDs
    std::vector<int> currentIds = clientManager.getClientIds();

    // Start from player ID 1 (0 is reserved for host)
    for (int id = 1; id < 8; id++) {
        bool idTaken = false;
        for (int existingId : currentIds) {
            if (existingId == id) {
                idTaken = true;
                break;
            }
        }
        if (!idTaken) return id;
    }

    // If all IDs are taken, return a high number
    return 99;
}
void NetworkManager::acceptClientConnections()
{
    while (running.load()) {
        try {
            sf::TcpSocket* newClient = new sf::TcpSocket();

            sf::Socket::Status status = listener.accept(*newClient);

            if (status == sf::Socket::Status::Done) {
                newClient->setBlocking(false);

                // Get the next available player ID
                int clientId = findNextAvailablePlayerId();

                // Add client to manager - make sure to match the method signature!
                clientId = clientManager.addClient(newClient, clientId);

                // Send player ID to client - this lets them know which pre-created rocket to use
                sendPlayerIdentity(clientId);

                std::stringstream ss;
                ss << "New client connected with ID: " << clientId;
                logger.info(ss.str());

                // No need to create a new player - just use the pre-created one with this ID
                logger.info("Client " + std::to_string(clientId) + " assigned to pre-existing rocket");
            }
            else {
                // No new connection, sleep a bit to avoid CPU hogging
                delete newClient;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        catch (const std::exception& e) {
            logger.error("Exception in acceptClientConnections: " + std::string(e.what()));
            // Continue after error and sleep to avoid CPU hogging if in error state
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        catch (...) {
            logger.error("Unknown exception in acceptClientConnections");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
void NetworkManager::receiveClientMessages()
{
    while (running.load()) {
        try {
            // Process messages from all clients
            std::vector<int> clientIds;
            try {
                clientIds = clientManager.getClientIds();
            }
            catch (const std::exception& e) {
                logger.error("Exception getting client IDs: " + std::string(e.what()));
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            for (int clientId : clientIds) {
                ClientData* client = nullptr;
                try {
                    client = clientManager.getClient(clientId);
                }
                catch (const std::exception& e) {
                    logger.error("Exception getting client data: " + std::string(e.what()));
                    continue;
                }

                if (!client || !client->socket) {
                    continue;
                }

                sf::Packet packet;
                sf::Socket::Status status;
                try {
                    status = client->socket->receive(packet);
                }
                catch (const std::exception& e) {
                    logger.error("Exception receiving from client " + std::to_string(clientId) + ": " + std::string(e.what()));
                    continue;
                }

                if (status == sf::Socket::Status::Done) {
                    client->updateActivity();
                    try {
                        handleClientMessage(clientId, packet);
                    }
                    catch (const std::exception& e) {
                        logger.error("Exception handling message from client " + std::to_string(clientId) + ": " + std::string(e.what()));
                    }
                }
                else if (status == sf::Socket::Status::Disconnected) {
                    try {
                        if (onClientDisconnected) {
                            onClientDisconnected(clientId);
                        }
                        clientManager.removeClient(clientId);
                    }
                    catch (const std::exception& e) {
                        logger.error("Exception removing disconnected client " + std::to_string(clientId) + ": " + std::string(e.what()));
                    }
                }
            }

            // Check for client timeouts
            try {
                clientManager.checkTimeouts();
            }
            catch (const std::exception& e) {
                logger.error("Exception checking timeouts: " + std::string(e.what()));
            }

            // Send heartbeats periodically
            static auto lastHeartbeat = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();

            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeat).count() >= 1) {
                try {
                    sendHeartbeats();
                }
                catch (const std::exception& e) {
                    logger.error("Exception sending heartbeats: " + std::string(e.what()));
                }
                lastHeartbeat = now;
            }

            // Sleep a bit to avoid CPU hogging
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        catch (const std::exception& e) {
            logger.error("Exception in receiveClientMessages: " + std::string(e.what()));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        catch (...) {
            logger.error("Unknown exception in receiveClientMessages");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void NetworkManager::handleClientMessage(int clientId, sf::Packet& packet)
{
    if (packet.getDataSize() == 0) {
        logger.warning("Received empty packet from client " + std::to_string(clientId));
        return;
    }

    uint32_t msgTypeInt;
    if (!(packet >> msgTypeInt)) {
        logger.warning("Received malformed packet from client " + std::to_string(clientId));
        return;
    }

    MessageType msgType = static_cast<MessageType>(msgTypeInt);

    switch (msgType) {
    case MessageType::PLAYER_INPUT:
        handlePlayerInput(clientId, packet);
        break;

    case MessageType::AUTHENTICATION:
        handleAuthentication(clientId, packet);
        break;

    case MessageType::HEARTBEAT:
        // Just update client activity, already done in receiveClientMessages
        break;

    case MessageType::DISCONNECT:
        if (onClientDisconnected) {
            onClientDisconnected(clientId);
        }
        clientManager.removeClient(clientId);
        break;

    default:
        logger.warning("Received unknown message type from client " + std::to_string(clientId));
        break;
    }
}

void NetworkManager::handlePlayerInput(int clientId, sf::Packet& packet)
{
    PlayerInput input;
    if (!(packet >> input)) {
        logger.warning("Received malformed player input from client " + std::to_string(clientId));
        return;
    }

    // Override the player ID with the client ID for security
    input.playerId = clientId;

    if (onPlayerInputReceived) {
        onPlayerInputReceived(clientId, input);
    }
}

void NetworkManager::handleAuthentication(int clientId, sf::Packet& packet)
{
    std::string username;
    if (!(packet >> username)) {
        logger.warning("Received malformed authentication from client " + std::to_string(clientId));
        return;
    }

    ClientData* client = clientManager.getClient(clientId);
    if (client) {
        client->authenticated = true;
        client->username = username;

        std::stringstream ss;
        ss << "Client " << clientId << " authenticated as " << username;
        logger.info(ss.str());

        if (onClientAuthenticated) {
            onClientAuthenticated(clientId, username);
        }
    }
}

void NetworkManager::sendHeartbeats()
{
    sf::Packet packet;
    packet << static_cast<uint32_t>(MessageType::HEARTBEAT);

    clientManager.sendToAll(packet);
}