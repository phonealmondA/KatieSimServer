// GameServer.cpp
#include "GameServer.h"
#include "GameConstants.h"
#include <sstream>
#include <cmath>

GameServer::GameServer(ServerLogger& logger, ServerConfig& config)
    : sequenceNumber(0), gameTime(0.0f), logger(logger), config(config)
{
}

GameServer::~GameServer()
{
    // Clean up rockets
    for (auto& pair : rockets) {
        delete pair.second;
    }
    rockets.clear();

    // Clean up planets
    for (auto& planet : planets) {
        delete planet;
    }
    planets.clear();
}

void GameServer::initialize()
{
    // Create solar system
    createSolarSystem();

    // Initialize simulator
    simulator.setSimulatePlanetGravity(true);

    for (auto& planet : planets) {
        simulator.addPlanet(planet);
    }

    logger.info("Game server initialized with " + std::to_string(planets.size()) + " planets");
}

void GameServer::update(float deltaTime)
{
    std::lock_guard<std::mutex> lock(gameStateMutex);

    // Update game time
    gameTime += deltaTime;

    // Update simulator
    simulator.update(deltaTime);

    // Increment sequence number
    sequenceNumber++;
}

void GameServer::handlePlayerInput(int playerId, const PlayerInput& input)
{
    std::lock_guard<std::mutex> lock(gameStateMutex);

    auto it = rockets.find(playerId);

    if (it == rockets.end()) {
        // Player not found - create a new rocket for this player
        addPlayer(playerId);
        it = rockets.find(playerId);

        if (it == rockets.end()) {
            logger.warning("Failed to create rocket for player " + std::to_string(playerId));
            return;
        }
    }

    Rocket* rocket = it->second;

    // Apply input to the rocket
    if (input.thrustForward) {
        rocket->applyThrust(input.thrustLevel);
    }

    if (input.thrustBackward) {
        rocket->applyThrust(-0.5f);
    }

    if (input.rotateLeft) {
        rocket->rotate(-6.0f * input.deltaTime * 60.0f);
    }

    if (input.rotateRight) {
        rocket->rotate(6.0f * input.deltaTime * 60.0f);
    }

    // Apply thrust level
    rocket->setThrustLevel(input.thrustLevel);
}

void GameServer::handlePlayerDisconnect(int playerId)
{
    removePlayer(playerId);
}

GameState GameServer::getGameState()
{
    std::lock_guard<std::mutex> lock(gameStateMutex);

    GameState state;
    state.sequenceNumber = sequenceNumber;
    state.timestamp = gameTime;

    // Add rockets
    for (auto& pair : rockets) {
        int playerId = pair.first;
        Rocket* rocket = pair.second;

        RocketState rocketState;
        rocketState.playerId = playerId;
        rocketState.position = rocket->getPosition();
        rocketState.velocity = rocket->getVelocity();
        rocketState.rotation = rocket->getRotation();
        rocketState.angularVelocity = 0.0f; // Not tracked in this implementation
        rocketState.thrustLevel = rocket->getThrustLevel();
        rocketState.mass = rocket->getMass();

        // Get rocket color
        uint8_t r, g, b;
        rocket->getColor(r, g, b);
        rocketState.colorR = r;
        rocketState.colorG = g;
        rocketState.colorB = b;

        state.rockets.push_back(rocketState);
    }

    // Add planets
    for (size_t i = 0; i < planets.size(); ++i) {
        Planet* planet = planets[i];

        PlanetState planetState;
        planetState.planetId = static_cast<int>(i);
        planetState.position = planet->getPosition();
        planetState.velocity = planet->getVelocity();
        planetState.mass = planet->getMass();
        planetState.radius = planet->getRadius();

        // Get planet color
        uint8_t r, g, b;
        planet->getColor(r, g, b);
        planetState.colorR = r;
        planetState.colorG = g;
        planetState.colorB = b;

        state.planets.push_back(planetState);
    }

    return state;
}

void GameServer::addPlayer(int playerId)
{
    std::lock_guard<std::mutex> lock(gameStateMutex);

    // Check if player already exists
    if (rockets.find(playerId) != rockets.end()) {
        return;
    }

    // Determine a good spawn position (above the main planet)
    sf::Vector2f spawnPos;
    if (!planets.empty()) {
        spawnPos = planets[0]->getPosition() +
            sf::Vector2f(0, -(planets[0]->getRadius() + GameConstants::ROCKET_SIZE + 30.0f));
    }
    else {
        spawnPos = sf::Vector2f(400.f, 100.f);
    }

    // Generate a unique color based on player ID
    uint8_t r = 100 + (playerId * 50) % 155;
    uint8_t g = 100 + (playerId * 30) % 155;
    uint8_t b = 100 + (playerId * 70) % 155;

    // Create a new rocket
    Rocket* rocket = new Rocket(spawnPos, sf::Vector2f(0, 0), playerId, 1.0f, r, g, b);

    // Add to the simulator
    simulator.addRocket(rocket);

    // Store in rockets map
    rockets[playerId] = rocket;

    std::stringstream ss;
    ss << "Added player " << playerId << " at position ("
        << spawnPos.x << ", " << spawnPos.y << ")";
    logger.info(ss.str());
}

void GameServer::removePlayer(int playerId)
{
    std::lock_guard<std::mutex> lock(gameStateMutex);

    auto it = rockets.find(playerId);
    if (it != rockets.end()) {
        Rocket* rocket = it->second;

        // Remove from simulator
        simulator.removeRocket(rocket);

        // Remove from map
        delete rocket;
        rockets.erase(it);

        logger.info("Removed player " + std::to_string(playerId));
    }
}

void GameServer::createSolarSystem()
{
    // Create main planet (sun)
    Planet* mainPlanet = new Planet(
        sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
        0, GameConstants::MAIN_PLANET_MASS, 255, 255, 0);  // Yellow
    mainPlanet->setVelocity(sf::Vector2f(0.f, 0.f));
    planets.push_back(mainPlanet);

    // Create orbit planets with different orbital distances and sizes
    const float basePlanetMass = GameConstants::SECONDARY_PLANET_MASS;

    // Planet colors
    const uint8_t planetColors[][3] = {
        {150, 150, 150},  // Mercury (gray)
        {255, 190, 120},  // Venus (light orange)
        {0, 100, 255},    // Earth (blue)
        {255, 100, 0},    // Mars (red)
        {255, 200, 100},  // Jupiter (light orange)
        {230, 180, 80},   // Saturn (tan)
        {180, 230, 230},  // Uranus (light blue)
        {100, 130, 255},  // Neptune (dark blue)
        {230, 230, 230}   // Pluto (light gray)
    };

    // Distance and mass scaling factors
    const float distanceScalings[] = { 0.4f, 0.7f, 1.0f, 1.5f, 2.2f, 3.0f, 4.0f, 5.0f, 6.0f };
    const float massScalings[] = { 0.1f, 0.8f, 1.0f, 0.5f, 11.0f, 9.5f, 4.0f, 3.8f, 0.05f };

    // Create each planet
    for (int i = 0; i < 9; i++) {
        float orbitDistance = GameConstants::PLANET_ORBIT_DISTANCE * distanceScalings[i];
        float angle = (i * 40.0f) * (3.14159f / 180.0f); // Distribute planets around the sun

        // Calculate position based on orbit distance and angle
        float planetX = mainPlanet->getPosition().x + orbitDistance * cos(angle);
        float planetY = mainPlanet->getPosition().y + orbitDistance * sin(angle);

        // Calculate orbital velocity for a circular orbit
        float orbitalVelocity = std::sqrt(GameConstants::G * mainPlanet->getMass() / orbitDistance);

        // Velocity is perpendicular to position vector
        float velocityX = -sin(angle) * orbitalVelocity;
        float velocityY = cos(angle) * orbitalVelocity;

        // Create the planet with scaled mass
        Planet* planet = new Planet(
            sf::Vector2f(planetX, planetY),
            0, basePlanetMass * massScalings[i],
            planetColors[i][0], planetColors[i][1], planetColors[i][2]);

        planet->setVelocity(sf::Vector2f(velocityX, velocityY));
        planets.push_back(planet);
    }
}