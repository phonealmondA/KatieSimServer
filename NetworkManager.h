// NetworkManager.h
#pragma once
#include <SFML/Network.hpp>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>  // Add this for std::function
#include "ClientManager.h"
#include "ServerLogger.h"
#include "ServerConfig.h"
#include "GameState.h"
#include "PlayerInput.h"

enum class MessageType {
    GAME_STATE = 1,
    PLAYER_INPUT = 2,
    PLAYER_ID = 3,
    HEARTBEAT = 4,
    DISCONNECT = 5,
    AUTHENTICATION = 6
};

class NetworkManager {
private:
    sf::TcpListener listener;
    std::thread acceptThread;
    std::thread receiveThread;
    std::atomic<bool> running;
    ClientManager& clientManager;
    ServerLogger& logger;
    ServerConfig& config;
    std::mutex networkMutex;

    // Callback functions
    std::function<void(int clientId, const PlayerInput&)> onPlayerInputReceived;
    std::function<void(int clientId, const std::string&)> onClientAuthenticated;
    std::function<void(int clientId)> onClientDisconnected;

public:
    NetworkManager(ClientManager& clientManager, ServerLogger& logger, ServerConfig& config);
    ~NetworkManager();

    bool start();
    void stop();
    bool sendGameState(const GameState& state);
    bool sendPlayerIdentity(int clientId);

    void setPlayerInputCallback(std::function<void(int clientId, const PlayerInput&)> callback);
    void setClientAuthenticatedCallback(std::function<void(int clientId, const std::string&)> callback);
    void setClientDisconnectedCallback(std::function<void(int clientId)> callback);

private:
    void acceptClientConnections();
    void receiveClientMessages();
    void handleClientMessage(int clientId, sf::Packet& packet);
    void handlePlayerInput(int clientId, sf::Packet& packet);
    void handleAuthentication(int clientId, sf::Packet& packet);
    void sendHeartbeats();
};