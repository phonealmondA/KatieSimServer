// NetworkManager.cpp
#include "NetworkManager.h"
#include <sstream>
#include <chrono>
#include <thread>

NetworkManager::NetworkManager(ClientManager& clientManager, ServerLogger& logger, ServerConfig& config)
    : clientManager(clientManager), logger(logger), config(config), running(false)
{
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
    if (listener.listen(config.getPort()) != sf::Socket::Done) {
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
    sf::IpAddress localIp = sf::IpAddress::getLocalAddress();
    logger.info("Local IP address: " + localIp.toString());

    try {
        sf::IpAddress publicIp = sf::IpAddress::getPublicAddress(sf::seconds(2));
        logger.info("Public IP address: " + publicIp.toString());
    }
    catch (...) {
        logger.warning("Could not determine public IP address");
    }

    running.store(true);

    // Start accept thread
    acceptThread = std::thread(&NetworkManager::acceptClientConnections, this);

    // Start receive thread
    receiveThread = std::thread(&NetworkManager::receiveClientMessages, this);

    return true;
}

void NetworkManager::stop()
{
    {
        std::lock_guard<std::mutex> lock(networkMutex);

        if (!running.load()) {
            return;
        }

        running.store(false);
        listener.close();
    }

    // Wait for threads to finish
    if (acceptThread.joinable()) {
        acceptThread.join();
    }

    if (receiveThread.joinable()) {
        receiveThread.join();
    }

    logger.info("Network manager stopped");
}

bool NetworkManager::sendGameState(const GameState& state)
{
    if (!running.load()) {
        return false;
    }

    sf::Packet packet;
    packet << static_cast<uint32_t>(MessageType::GAME_STATE) << state;

    clientManager.sendToAll(packet);
    return true;
}

bool NetworkManager::sendPlayerIdentity(int clientId)
{
    if (!running.load()) {
        return false;
    }

    sf::Packet packet;
    packet << static_cast<uint32_t>(MessageType::PLAYER_ID) << static_cast<uint32_t>(clientId);

    clientManager.sendTo(clientId, packet);
    return true;
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

void NetworkManager::acceptClientConnections()
{
    while (running.load()) {
        sf::TcpSocket* newClient = new sf::TcpSocket();

        if (listener.accept(*newClient) == sf::Socket::Done) {
            newClient->setBlocking(false);

            // Add client to manager
            int clientId = clientManager.addClient(newClient);

            // Send player ID to client
            sendPlayerIdentity(clientId);

            std::stringstream ss;
            ss << "New client connected with ID: " << clientId;
            logger.info(ss.str());
        }
        else {
            // No new connection, sleep a bit to avoid CPU hogging
            delete newClient;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void NetworkManager::receiveClientMessages()
{
    while (running.load()) {
        // Process messages from all clients
        std::vector<int> clientIds = clientManager.getClientIds();

        for (int clientId : clientIds) {
            ClientData* client = clientManager.getClient(clientId);

            if (!client || !client->socket) {
                continue;
            }

            sf::Packet packet;
            sf::Socket::Status status = client->socket->receive(packet);

            if (status == sf::Socket::Done) {
                client->updateActivity();
                handleClientMessage(clientId, packet);
            }
            else if (status == sf::Socket::Disconnected) {
                if (onClientDisconnected) {
                    onClientDisconnected(clientId);
                }

                clientManager.removeClient(clientId);
            }
        }

        // Check for client timeouts
        clientManager.checkTimeouts();

        // Send heartbeats periodically
        static auto lastHeartbeat = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeat).count() >= 1) {
            sendHeartbeats();
            lastHeartbeat = now;
        }

        // Sleep a bit to avoid CPU hogging
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void NetworkManager::handleClientMessage(int clientId, sf::Packet& packet)
{
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