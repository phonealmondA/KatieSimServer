// GameClient.h
#pragma once
#include "GravitySimulator.h"
#include "VehicleManager.h"
#include "GameState.h"
#include "PlayerInput.h"
#include <vector>
#include <map>
#include <SFML/Graphics.hpp>
// Forward declaration for connection state
enum class ClientConnectionState {
    DISCONNECTED,
    CONNECTING,
    WAITING_FOR_ID,
    WAITING_FOR_STATE,
    CONNECTED
};

// Struct to store interpolation data for remote players
struct RemotePlayerState {
    sf::Vector2f a; // startPos
    sf::Vector2f b; // startVel
    sf::Vector2f c; // targetPos
    sf::Vector2f d; // targetVel
    float e; // rotation
    float f; // timestamp
};

class GameClient {
private:
    GravitySimulator a; // simulator
    std::vector<Planet*> b; // planets
    std::map<int, VehicleManager*> c; // remotePlayers
    VehicleManager* d; // localPlayer
    int e; // localPlayerId

    // Last received state for interpolation
    GameState f; // lastState
    float g; // stateTimestamp

    // Interpolation data for remote players
    std::map<int, RemotePlayerState> h; // remotePlayerStates
    float i; // latencyCompensation - time window for interpolation

    // Connection state tracking
    ClientConnectionState j; // connectionState
    bool k; // hasReceivedInitialState

    // New members for distributed simulation
    GameState l; // localSimulation - client's own simulation state
    sf::Clock m; // simulationClock - time tracking for local simulation
    float n; // simulationTime - current time in local simulation
    bool o; // simulationPaused - whether local simulation is paused waiting for server
    float p; // lastServerSyncTime - when server last validated our simulation
    float q; // syncInterval - how often to send simulation to server
    bool r; // pendingValidation - waiting for server validation

public:
    GameClient();
    ~GameClient();

    void initializeLocalSimulation();
    void initialize();
    void update(float deltaTime);
    void processGameState(const GameState& state);
    PlayerInput getLocalPlayerInput(float deltaTime) const;

    // Apply input locally for responsive control
    void applyLocalInput(const PlayerInput& input);

    // Interpolate remote players between received states
    void interpolateRemotePlayers(float currentTime);

    // New methods for distributed simulation
    void runLocalSimulation(float deltaTime);
    void pauseSimulation() { o = true; }
    void resumeSimulation() { o = false; }
    GameState getLocalSimulation() const { return l; }
    void processServerValidation(const GameState& validatedState);
    void setSyncInterval(float interval) { q = interval; }

    // Set latency compensation window
    void setLatencyCompensation(float value);
    void setLocalPlayerId(int id);
    int getLocalPlayerId() const { return e; }

    VehicleManager* getLocalPlayer() { return d; }
    const std::vector<Planet*>& getPlanets() const { return b; }
    const std::map<int, VehicleManager*>& getRemotePlayers() const { return c; }

    // Connection state methods
    bool isConnected() const { return j == ClientConnectionState::CONNECTED && k; }
    bool isWaitingForState() const { return j == ClientConnectionState::WAITING_FOR_STATE; }
    bool isPendingValidation() const { return r; }
};