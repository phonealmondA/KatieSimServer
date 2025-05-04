// NetworkManager.h
#pragma once
#include <SFML/Network.hpp>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <mutex>
#include <atomic>
#include "GameState.h"
#include "PlayerInput.h"

// Forward declarations
class GameServer;
class GameClient;

// Connection state enum
enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    AUTHENTICATING,
    CONNECTED
};

// Message types for network communication
enum class MessageType {
    GAME_STATE = 1,
    PLAYER_INPUT = 2,
    PLAYER_ID = 3,
    HEARTBEAT = 4,
    DISCONNECT = 5,
    CLIENT_SIMULATION = 6,   // New message type for client simulation state
    SERVER_VALIDATION = 7    // New message type for server validation
};

class NetworkManager {
private:
    bool a; // isHost
    std::vector<sf::TcpSocket*> b; // clients
    sf::TcpSocket c; // serverConnection
    sf::TcpListener d; // listener
    unsigned short e; // port
    bool f; // connected

    // References to game components
    GameServer* g; // gameServer
    GameClient* h; // gameClient

    // Network diagnostics
    sf::Clock i; // lastPacketTime
    int j; // packetLossCounter
    int k; // pingMs

    // Connection state tracking
    ConnectionState l; // connectionState

    // New variables for distributed simulation
    float m; // syncInterval - how often to sync with server
    sf::Clock n; // syncClock - tracks time since last sync
    std::map<int, float> o; // clientLastSyncTimes - when each client last sent their simulation

public:
    NetworkManager();
    ~NetworkManager();

    // Set references to game components
    void setGameServer(GameServer* server);
    void setGameClient(GameClient* client);

    bool hostGame(unsigned short port);
    bool joinGame(const sf::IpAddress& address, unsigned short port);
    void disconnect();
    void update();
    bool sendGameState(const GameState& state);   // Host only
    bool sendPlayerInput(const PlayerInput& input); // Client only

    // New methods for distributed simulation
    bool sendClientSimulation(const GameState& clientState);  // Client sending its simulation
    bool sendServerValidation(const GameState& validatedState, int clientId);  // Server validation

    // Network robustness improvements
    void enableRobustNetworking();
    float getPing() const;
    int getPacketLoss() const;

    // Sync interval setter/getter
    void setSyncInterval(float interval) { m = interval; }
    float getSyncInterval() const { return m; }

    bool isConnected() const { return f; }
    bool getIsHost() const { return a; }
    bool isFullyConnected() const { return f && l == ConnectionState::CONNECTED; }

    // Callbacks to be set by the game
    std::function<void(int clientId, const PlayerInput&)> onPlayerInputReceived;
    std::function<void(const GameState&)> onGameStateReceived;
    std::function<void(int clientId, const GameState&)> onClientSimulationReceived;  // New callback
    std::function<void(const GameState&)> onServerValidationReceived;  // New callback
};