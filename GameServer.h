// GameServer.h
#pragma once
#include "GravitySimulator.h"
#include "VehicleManager.h"
#include "GameState.h"
#include "PlayerInput.h"
#include <vector>
#include <map>
#include <mutex>
#include <SFML/Graphics.hpp>
class GameServer {
private:
    std::vector<Planet*> a; // planets
    std::map<int, VehicleManager*> b; // players
    GravitySimulator c; // simulator
    std::mutex d; // gameStateMutex

    unsigned long e; // sequenceNumber
    float f; // gameTime

    // New members for distributed simulation
    std::map<int, GameState> g; // clientSimulations - stores each client's simulation state
    std::map<int, float> h; // lastClientUpdateTime - when each client last sent their simulation
    std::map<int, bool> i; // clientSimulationValid - whether each client's simulation is valid
    float j; // validationThreshold - how much difference is allowed before correcting client

public:
    GameServer();
    ~GameServer();

    void initialize();
    void update(float deltaTime);

    // Handle input from clients
    void handlePlayerInput(int playerId, const PlayerInput& input);

    // New method to process client simulation states
    void processClientSimulation(int playerId, const GameState& clientState);

    // Validate client simulation against server state
    GameState validateClientSimulation(int playerId, const GameState& clientState);

    // Get the current game state to send to clients
    GameState getGameState() const;

    // Add/remove players
    int addPlayer(int playerId, sf::Vector2f initialPos = sf::Vector2f(0, 0), sf::Color color = sf::Color::White);
    void removePlayer(int playerId);

    // Synchronize states between server and clients
    void synchronizeState();

    // Getters
    const std::vector<Planet*>& getPlanets() const { return a; }
    const std::map<int, VehicleManager*>& getPlayers() const { return b; }
    VehicleManager* getPlayer(int playerId) {
        auto it = b.find(playerId);
        return (it != b.end()) ? it->second : nullptr;
    }

    // Set validation threshold
    void setValidationThreshold(float threshold) { j = threshold; }
    float getValidationThreshold() const { return j; }

private:
    // Create the initial solar system
    void createSolarSystem();
};