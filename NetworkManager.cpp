// NetworkManager.cpp
#include "NetworkManager.h"
#include "GameServer.h" // Include directly instead of forward declaration
#include "GameClient.h" // Include directly instead of forward declaration
#include "ClientManager.h"
#include "ServerLogger.h"
#include "ServerConfig.h"
#include "GameConstants.h"
#include <sstream>
#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm>

NetworkManager::NetworkManager(ClientManager& clientManager, ServerLogger& logger, ServerConfig& config)
    : a(true), // Always true for server application
    e(0), f(false), g(nullptr), h(nullptr),
    j(0), k(0), l(ConnectionState::DISCONNECTED), m(0.1f),
    p(clientManager), q(logger), r(config)
{
    // Initialize clocks and maps
    i.restart();
    n.restart();

    // Initialize callbacks to nullptr
    onPlayerInputReceived = nullptr;
    onGameStateReceived = nullptr;
    onClientSimulationReceived = nullptr;
    onServerValidationReceived = nullptr;
    s = nullptr;
    t = nullptr;
    u = nullptr;
}

NetworkManager::~NetworkManager()
{
    try {
        disconnect();
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in NetworkManager destructor: " << ex.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception in NetworkManager destructor" << std::endl;
    }
}

void NetworkManager::setGameServer(GameServer* server)
{
    g = server;
}

void NetworkManager::setGameClient(GameClient* client)
{
    h = client;
}

bool NetworkManager::hostGame(unsigned short port)
{
    try {
        e = port;
        a = true;
        l = ConnectionState::CONNECTING;

        // Start listening for connections
        if (d.listen(port) != sf::Socket::Status::Done) {
            std::cerr << "Failed to bind to port " << port << std::endl;
            l = ConnectionState::DISCONNECTED;
            return false;
        }

        std::cout << "Server started on port " << port << std::endl;

        // Log IP addresses
        auto localIp = sf::IpAddress::getLocalAddress();
        if (localIp) {
            std::cout << "Local IP address: " << localIp->toString() << std::endl;
        }
        else {
            std::cout << "Could not determine local IP address" << std::endl;
        }

        try {
            auto publicIp = sf::IpAddress::getPublicAddress(sf::seconds(2));
            if (publicIp) {
                std::cout << "Public IP address: " << publicIp->toString() << std::endl;
            }
            else {
                std::cout << "Could not determine public IP address" << std::endl;
            }
        }
        catch (const std::exception& ex) {
            std::cerr << "Error getting public IP: " << ex.what() << std::endl;
        }

        d.setBlocking(false);
        f = true;
        l = ConnectionState::CONNECTED;
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in hostGame: " << ex.what() << std::endl;
        f = false;
        l = ConnectionState::DISCONNECTED;
        return false;
    }
}

bool NetworkManager::joinGame(const sf::IpAddress& address, unsigned short port)
{
    try {
        a = false;
        l = ConnectionState::CONNECTING;

        std::cout << "Connecting to " << address.toString() << ":" << port << "..." << std::endl;

        // Set timeout for connection attempts
        c.setBlocking(true);
        sf::Socket::Status status = c.connect(address, port, sf::seconds(5));
        c.setBlocking(false);

        if (status != sf::Socket::Status::Done) {
            std::cerr << "Failed to connect to " << address.toString() << ":" << port << std::endl;
            l = ConnectionState::DISCONNECTED;
            return false;
        }

        std::cout << "Successfully connected to server!" << std::endl;
        f = true;
        l = ConnectionState::AUTHENTICATING; // Move to authenticating until we get player ID
        i.restart();
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in joinGame: " << ex.what() << std::endl;
        f = false;
        l = ConnectionState::DISCONNECTED;
        return false;
    }
}

bool NetworkManager::sendClientSimulation(const GameState& clientState)
{
    if (a || !f) return false;

    try {
        sf::Packet packet;
        packet << static_cast<uint32_t>(static_cast<int>(MessageType::CLIENT_SIMULATION)) << clientState;

        sf::Socket::Status status = c.send(packet);
        if (status != sf::Socket::Status::Done) {
            j++;
            return false;
        }

        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in sendClientSimulation: " << ex.what() << std::endl;
        return false;
    }
}

bool NetworkManager::sendServerValidation(const GameState& validatedState, int clientId)
{
    if (!a || !f) return false;

    try {
        sf::Packet packet;
        packet << static_cast<uint32_t>(static_cast<int>(MessageType::SERVER_VALIDATION)) << validatedState;

        // Find the client socket matching the ID
        if (clientId <= 0 || clientId > static_cast<int>(b.size())) {
            std::cerr << "Invalid client ID in sendServerValidation: " << clientId << std::endl;
            return false;
        }

        sf::TcpSocket* clientSocket = b[clientId - 1]; // Client IDs are 1-based, array is 0-based
        if (!clientSocket) {
            std::cerr << "Null client socket for ID: " << clientId << std::endl;
            return false;
        }

        sf::Socket::Status status = clientSocket->send(packet);
        if (status != sf::Socket::Status::Done) {
            j++;
            return false;
        }

        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in sendServerValidation: " << ex.what() << std::endl;
        return false;
    }
}

void NetworkManager::update()
{
    try {
        if (!f) {
            // Return early if we're not connected
            return;
        }

        // Check for timeouts (5 seconds without data)
        if (i.getElapsedTime().asSeconds() > 5.0f) {
            std::cerr << "Connection timed out - no data received for 5 seconds" << std::endl;
            disconnect();
            return;
        }

        // Send heartbeat every second
        static sf::Clock heartbeatClock;
        if (heartbeatClock.getElapsedTime().asSeconds() > 1.0f) {
            try {
                sf::Packet heartbeatPacket;
                heartbeatPacket << static_cast<uint32_t>(static_cast<int>(MessageType::HEARTBEAT));

                if (a) {
                    for (auto client : b) {
                        if (client) {
                            sf::Socket::Status status = client->send(heartbeatPacket);
                            if (status != sf::Socket::Status::Done) {
                                j++;
                            }
                        }
                    }
                }
                else {
                    sf::Socket::Status status = c.send(heartbeatPacket);
                    if (status != sf::Socket::Status::Done) {
                        j++;
                    }
                }
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception sending heartbeat: " << ex.what() << std::endl;
            }

            heartbeatClock.restart();
        }

        if (a) {
            // Server mode: accept new connections
            try {
                sf::TcpSocket* newClient = new sf::TcpSocket();
                sf::Socket::Status status = d.accept(*newClient);

                if (status == sf::Socket::Status::Done) {
                    newClient->setBlocking(false);

                    // Log connection info
                    if (auto remoteAddress = newClient->getRemoteAddress()) {
                        std::cout << "New client connecting from: " << remoteAddress->toString() << std::endl;
                    }
                    else {
                        std::cout << "New client connecting from: unknown address" << std::endl;
                    }

                    b.push_back(newClient);

                    // Create a unique ID for the client (use client index + 1 to avoid ID 0)
                    int clientId = static_cast<int>(b.size()); // This will be 1 for the first client

                    // Send player ID to the client
                    sf::Packet idPacket;
                    idPacket << static_cast<uint32_t>(static_cast<int>(MessageType::PLAYER_ID)) << static_cast<uint32_t>(clientId);
                    sf::Socket::Status sendStatus = newClient->send(idPacket);

                    if (sendStatus != sf::Socket::Status::Done) {
                        std::cerr << "Failed to send player ID to client" << std::endl;
                    }

                    // Create a new player for this client if gameServer exists
                    if (g) {
                        const auto& planets = g->getPlanets();
                        if (!planets.empty() && planets[0]) {
                            sf::Vector2f spawnPos = planets[0]->getPosition() +
                                sf::Vector2f(0, -(planets[0]->getRadius() + GameConstants::ROCKET_SIZE + 30.0f));
                            g->addPlayer(clientId, spawnPos, sf::Color::Red);
                        }
                        else {
                            g->addPlayer(clientId, sf::Vector2f(400.f, 100.f), sf::Color::Red);
                        }
                    }

                    std::cout << "New client connected with ID: " << clientId << std::endl;

                    // Call the authentication callback
                    if (u) {
                        u(clientId, "Player_" + std::to_string(clientId));
                    }
                }
                else {
                    // No new connection, clean up allocated socket
                    delete newClient;
                }
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception accepting new connection: " << ex.what() << std::endl;
            }

            // Check for messages from clients
            for (size_t i = 0; i < b.size(); i++) {
                sf::TcpSocket* client = b[i];

                if (!client) {
                    continue;
                }

                try {
                    sf::Packet packet;
                    sf::Socket::Status status = client->receive(packet);

                    if (status == sf::Socket::Status::Done) {
                        if (packet.getDataSize() > 0) {
                            uint32_t msgType;
                            if (packet >> msgType) {
                                // Client ID is index+1
                                int clientId = static_cast<int>(i + 1);

                                switch (static_cast<MessageType>(msgType)) {
                                case MessageType::PLAYER_INPUT:
                                {
                                    PlayerInput input;
                                    if (packet >> input) {
                                        // Override the player ID with the client ID for security
                                        input.a = clientId;

                                        if (onPlayerInputReceived) {
                                            onPlayerInputReceived(clientId, input);
                                        }

                                        // Call the callback
                                        if (s) {
                                            s(clientId, input);
                                        }
                                    }
                                    break;
                                }
                                case MessageType::CLIENT_SIMULATION:
                                {
                                    GameState clientState;
                                    if (packet >> clientState) {
                                        if (onClientSimulationReceived) {
                                            onClientSimulationReceived(clientId, clientState);
                                        }
                                    }
                                    break;
                                }
                                case MessageType::DISCONNECT:
                                    std::cout << "Client " << clientId << " requested disconnect" << std::endl;
                                    // Handle client disconnect - clean up client socket and game resources
                                    client->disconnect();
                                    delete client;
                                    b[i] = nullptr;

                                    if (g) {
                                        g->removePlayer(clientId);
                                    }

                                    // Call the disconnection callback
                                    if (t) {
                                        t(clientId);
                                    }
                                    break;

                                default:
                                    std::cerr << "Received unknown message type from client: " << msgType << std::endl;
                                    break;
                                }
                            }
                        }
                    }
                    else if (status == sf::Socket::Status::Disconnected) {
                        int clientId = static_cast<int>(i + 1);
                        std::cout << "Client " << clientId << " disconnected" << std::endl;

                        // Clean up client socket and game resources
                        delete client;
                        b[i] = nullptr;

                        if (g) {
                            g->removePlayer(clientId);
                        }

                        // Call the disconnection callback
                        if (t) {
                            t(clientId);
                        }
                    }
                }
                catch (const std::exception& ex) {
                    std::cerr << "Exception processing client message: " << ex.what() << std::endl;
                }
            }

            // Remove null client pointers from the vector
            b.erase(
                std::remove_if(b.begin(), b.end(),
                    [](sf::TcpSocket* client) { return client == nullptr; }),
                b.end()
            );
        }
        else {
            // Client mode - improved error handling
            sf::Packet packet;
            sf::Socket::Status status = c.receive(packet);

            if (status == sf::Socket::Status::Done) {
                i.restart();

                // Ensure packet is not empty before trying to read from it
                if (packet.getDataSize() > 0) {
                    uint32_t msgType;
                    if (packet >> msgType) {
                        switch (static_cast<MessageType>(msgType)) {
                        case MessageType::PLAYER_ID:
                        {
                            uint32_t playerId;
                            if (packet >> playerId) {
                                if (h) {
                                    std::cout << "Received player ID from server: " << playerId << std::endl;

                                    // Set the player ID and update connection state
                                    h->setLocalPlayerId(static_cast<int>(playerId));

                                    // Explicitly transition to waiting for state
                                    l = ConnectionState::CONNECTED;
                                    std::cout << "Connection state updated to waiting for game state" << std::endl;
                                }
                                else {
                                    std::cerr << "Error: Received player ID but gameClient is null" << std::endl;
                                }
                            }
                        }
                        break;
                        case MessageType::GAME_STATE:
                        {
                            // Measure ping
                            static sf::Clock pingClock;
                            k = pingClock.restart().asMilliseconds();

                            // Handle game state with additional safety
                            GameState state;
                            try {
                                if (packet >> state) {
                                    if (onGameStateReceived && h) {
                                        onGameStateReceived(state);
                                    }
                                }
                                else {
                                    std::cerr << "Failed to parse game state packet" << std::endl;
                                }
                            }
                            catch (const std::exception& ex) {
                                std::cerr << "Exception parsing game state: " << ex.what() << std::endl;
                            }
                        }
                        break;
                        case MessageType::SERVER_VALIDATION:
                        {
                            GameState validatedState;
                            try {
                                if (packet >> validatedState) {
                                    if (onServerValidationReceived && h) {
                                        onServerValidationReceived(validatedState);
                                    }
                                }
                                else {
                                    std::cerr << "Failed to parse server validation packet" << std::endl;
                                }
                            }
                            catch (const std::exception& ex) {
                                std::cerr << "Exception parsing server validation: " << ex.what() << std::endl;
                            }
                        }
                        break;
                        case MessageType::HEARTBEAT:
                            // Just a keep-alive, no action needed
                            break;
                        case MessageType::DISCONNECT:
                            std::cout << "Disconnected from server" << std::endl;
                            f = false;
                            l = ConnectionState::DISCONNECTED;
                            c.disconnect();
                            break;
                        default:
                            std::cerr << "Received unknown message type: " << msgType << std::endl;
                            break;
                        }
                    }
                    else {
                        std::cerr << "Failed to read message type from packet" << std::endl;
                    }
                }
            }
            else if (status == sf::Socket::Status::Disconnected) {
                std::cout << "Lost connection to server" << std::endl;
                f = false;
                l = ConnectionState::DISCONNECTED;
            }
        }

        // Check if it's time to sync with server for client simulation
        if (!a && h && f && l == ConnectionState::CONNECTED) {
            if (n.getElapsedTime().asSeconds() >= m) {
                if (h->isPendingValidation()) {
                    GameState clientSim = h->getLocalSimulation();
                    sendClientSimulation(clientSim);
                    n.restart();
                }
            }
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in update: " << ex.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception in update" << std::endl;
    }
}

void NetworkManager::disconnect()
{
    try {
        if (f) {
            // Send disconnect message
            if (!a) {
                try {
                    sf::Packet disconnectPacket;
                    disconnectPacket << static_cast<uint32_t>(static_cast<int>(MessageType::DISCONNECT));
                    c.send(disconnectPacket);
                }
                catch (...) {
                    // Ignore errors when trying to send disconnect message
                }
            }
        }

        if (a) {
            try {
                d.close();
            }
            catch (...) {
                // Ignore errors when closing listener
            }

            // Disconnect all clients
            for (auto client : b) {
                if (client) {
                    try {
                        // Send disconnect message to clients
                        sf::Packet disconnectPacket;
                        disconnectPacket << static_cast<uint32_t>(static_cast<int>(MessageType::DISCONNECT));
                        client->send(disconnectPacket);

                        client->disconnect();
                        delete client;
                    }
                    catch (...) {
                        // Ignore errors and continue cleanup
                    }
                }
            }
            b.clear();
        }
        else {
            try {
                c.disconnect();
            }
            catch (...) {
                // Ignore errors when disconnecting
            }
        }

        f = false;
        l = ConnectionState::DISCONNECTED;
        std::cout << "Disconnected from network" << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in disconnect: " << ex.what() << std::endl;
        // Force disconnect state even if there was an error
        f = false;
        l = ConnectionState::DISCONNECTED;
    }
    catch (...) {
        std::cerr << "Unknown exception in disconnect" << std::endl;
        // Force disconnect state even if there was an error
        f = false;
        l = ConnectionState::DISCONNECTED;
    }
}

void NetworkManager::enableRobustNetworking()
{
    try {
        // Set non-blocking sockets with timeouts
        if (a) {
            for (auto* client : b) {
                if (client) {
                    client->setBlocking(false);
                }
            }
        }
        else {
            c.setBlocking(false);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in enableRobustNetworking: " << ex.what() << std::endl;
    }
}

bool NetworkManager::sendGameState(const GameState& state)
{
    if (!a || !f) return false;

    try {
        sf::Packet packet;
        packet << static_cast<uint32_t>(static_cast<int>(MessageType::GAME_STATE)) << state;

        bool allSucceeded = true;

        for (auto client : b) {
            if (!client) continue;

            sf::Socket::Status status = client->send(packet);
            if (status != sf::Socket::Status::Done) {
                allSucceeded = false;
                j++;
            }
        }

        return allSucceeded;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in sendGameState: " << ex.what() << std::endl;
        return false;
    }
}

bool NetworkManager::sendPlayerInput(const PlayerInput& input)
{
    if (a || !f) return false;

    try {
        sf::Packet packet;
        packet << static_cast<uint32_t>(static_cast<int>(MessageType::PLAYER_INPUT)) << input;

        sf::Socket::Status status = c.send(packet);
        if (status != sf::Socket::Status::Done) {
            j++;
            return false;
        }

        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in sendPlayerInput: " << ex.what() << std::endl;
        return false;
    }
}

float NetworkManager::getPing() const
{
    return static_cast<float>(k);
}

int NetworkManager::getPacketLoss() const
{
    return j;
}
bool NetworkManager::start() {
    try {
        // Initialize the network components
        if (a) {
            // Host mode - start server
            if (!hostGame(r.getPort())) {
                q.error("Failed to start server on port " + std::to_string(r.getPort()));
                return false;
            }
        }
        else {
            // Client mode not implemented here
            q.error("Client mode not implemented in start()");
            return false;
        }

        return true;
    }
    catch (const std::exception& ex) {
        q.error("Exception in start(): " + std::string(ex.what()));
        return false;
    }
}

void NetworkManager::stop() {
    try {
        disconnect();
        q.info("Network manager stopped");
    }
    catch (const std::exception& ex) {
        q.error("Exception in stop(): " + std::string(ex.what()));
    }
}