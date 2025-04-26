// GameServer.h
#pragma once
#include <vector>
#include <map>
#include <mutex>
#include "GravitySimulator.h"
#include "Planet.h"
#include "Rocket.h"
#include "GameState.h"
#include "PlayerInput.h"
#include "ServerLogger.h"
#include "ServerConfig.h"

class GameServer {
private:
    std::vector<Planet*> planets;
    std::map<int, Rocket*> rockets;
    GravitySimulator simulator;
    std::mutex gameStateMutex;

    unsigned long sequenceNumber;
    float gameTime;

    ServerLogger& logger;
    ServerConfig& config;

public:
    GameServer(ServerLogger& logger, ServerConfig& config);
    ~GameServer();

    void initialize();
    void update(float deltaTime);
    void handlePlayerInput(int playerId, const PlayerInput& input);
    void handlePlayerDisconnect(int playerId);
    GameState getGameState();

    void addPlayer(int playerId);
    void removePlayer(int playerId);

private:
    void createSolarSystem();
};