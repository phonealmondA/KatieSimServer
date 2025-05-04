// GameServer.cpp
#include "GameServer.h"
#include "GameConstants.h"
#include <iostream>
#include <cmath>

GameServer::GameServer()
    : e(0), f(0.0f), j(0.1f) // Set validation threshold to 10% difference
{
    // Constructor implementation
}

GameServer::~GameServer()
{
    // Clean up players
    for (auto& player : b) {
        delete player.second;
    }
    b.clear();

    // Clean up planets
    for (auto& planet : a) {
        delete planet;
    }
    a.clear();
}

void GameServer::initialize()
{
    // Create main planet (sun)
    Planet* mainPlanet = new Planet(
        sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
        0, GameConstants::MAIN_PLANET_MASS, sf::Color::Yellow);
    mainPlanet->setVelocity(sf::Vector2f(1.f, -1.f));
    a.push_back(mainPlanet);

    // Create 9 orbiting planets with different orbital distances, sizes, and colors
    const float baseMass = GameConstants::SECONDARY_PLANET_MASS;
    const sf::Color planetColors[] = {
        sf::Color(150, 150, 150),   // Mercury (gray)
        sf::Color(255, 190, 120),   // Venus (light orange)
        sf::Color(0, 100, 255),     // Earth (blue)
        sf::Color(255, 100, 0),     // Mars (red)
        sf::Color(255, 200, 100),   // Jupiter (light orange)
        sf::Color(230, 180, 80),    // Saturn (tan)
        sf::Color(180, 230, 230),   // Uranus (light blue)
        sf::Color(100, 130, 255),   // Neptune (dark blue)
        sf::Color(230, 230, 230)    // Pluto (light gray)
    };

    // Distance and mass scaling factors
    const float distanceFactors[] = { 0.4f, 0.7f, 1.0f, 1.5f, 2.2f, 3.0f, 4.0f, 5.0f, 6.0f };
    const float massFactors[] = { 0.1f, 0.8f, 1.0f, 0.5f, 11.0f, 9.5f, 4.0f, 3.8f, 0.05f };

    // Create each planet
    for (int i = 0; i < 9; i++) {
        float orbitDistance = GameConstants::PLANET_ORBIT_DISTANCE * distanceFactors[i];
        float angle = (i * 40.0f) * (3.14159f / 180.0f); // Distribute planets around the sun

        // Calculate position based on orbit distance and angle
        float posX = mainPlanet->getPosition().x + orbitDistance * cos(angle);
        float posY = mainPlanet->getPosition().y + orbitDistance * sin(angle);

        // Calculate orbital velocity for a circular orbit
        float orbitalVelocity = std::sqrt(GameConstants::G * mainPlanet->getMass() / orbitDistance);

        // Velocity is perpendicular to position vector
        float velX = -sin(angle) * orbitalVelocity;
        float velY = cos(angle) * orbitalVelocity;

        // Create the planet with scaled mass
        Planet* newPlanet = new Planet(
            sf::Vector2f(posX, posY),
            0, baseMass * massFactors[i], planetColors[i]);

        newPlanet->setVelocity(sf::Vector2f(velX, velY));
        a.push_back(newPlanet);
    }

    // Setup gravity simulator
    c.setSimulatePlanetGravity(true);
    for (auto planet : a) {
        c.addPlanet(planet);
    }

    // Create a default host player (ID 0)
    sf::Vector2f spawnPos = a[0]->getPosition() +
        sf::Vector2f(0, -(a[0]->getRadius() + GameConstants::ROCKET_SIZE));
    addPlayer(0, spawnPos, sf::Color::White);
}

void GameServer::update(float deltaTime)
{
    // Update game time
    f += deltaTime;

    // Update simulator for server-owned objects
    c.update(deltaTime);

    // Update planets
    for (auto planet : a) {
        planet->update(deltaTime);
    }

    // Update players safely
    for (auto& pair : b) {
        if (pair.second) {
            pair.second->update(deltaTime);
        }
    }

    // Increment sequence number
    e++;

    // Periodically synchronize client and server states
    synchronizeState();
}

void GameServer::handlePlayerInput(int playerId, const PlayerInput& input)
{
    auto it = b.find(playerId);
    if (it == b.end()) {
        // Player doesn't exist - could be a new connection, create player
        std::cout << "Unknown player ID: " << playerId << ", creating new player" << std::endl;
        sf::Vector2f initialPos = a[0]->getPosition() +
            sf::Vector2f(0, -(a[0]->getRadius() + GameConstants::ROCKET_SIZE));

        // Add the player with error handling
        VehicleManager* player = new VehicleManager(initialPos, a, playerId);
        if (player && player->getRocket()) {
            b[playerId] = player;
            c.addVehicleManager(player);

            // Initialize client simulation tracking
            g[playerId] = GameState();
            h[playerId] = f; // Current game time
            i[playerId] = true; // Initially valid
        }
        else {
            std::cerr << "Failed to create player for ID: " << playerId << std::endl;
            delete player;
        }
        return;
    }

    // Update client state tracking
    if (input.j > h[playerId]) {
        h[playerId] = input.j; // Update last client update time
    }

    // Get client's rocket state if provided
    if (input.k.j) { // If this is authoritative from client
        // Store the client's rocket state
        g[playerId].c.clear();
        g[playerId].c.push_back(input.k);

        // Mark client simulation as valid
        i[playerId] = true;
    }

    // Apply input to the player
    VehicleManager* player = it->second;
    if (!player) return; // Add null check

    // Apply the input
    if (input.b) {
        player->applyThrust(1.0f);
    }if (input.c) {
        player->applyThrust(-0.5f);
    }
    if (input.d) {
        player->rotate(-6.0f * input.h * 60.0f);
    }
    if (input.e) {
        player->rotate(6.0f * input.h * 60.0f);
    }
    if (input.f) {
        player->switchVehicle();
    }

    // Apply thrust level with safe null checking
    if (player->getActiveVehicleType() == VehicleType::ROCKET && player->getRocket()) {
        player->getRocket()->setThrustLevel(input.g);
    }
}

void GameServer::processClientSimulation(int playerId, const GameState& clientState)
{
    // Store the client's latest state
    g[playerId] = clientState;
    h[playerId] = clientState.b; // Update timestamp

    // Validate the client simulation
    GameState validatedState = validateClientSimulation(playerId, clientState);

    // If validation changed something, send back the corrected state
    if (!i[playerId]) {
        // Set the initialState flag to true to force client to accept this state
        validatedState.e = true;

        // TODO: Send validation state back to client
        // This would be added in NetworkManager with a new message type
    }
}

GameState GameServer::validateClientSimulation(int playerId, const GameState& clientState)
{
    GameState validatedState = clientState;
    bool isValid = true;

    // Get server's state for this player
    VehicleManager* player = getPlayer(playerId);
    if (!player || !player->getRocket()) {
        i[playerId] = false;
        return validatedState;
    }

    // Compare key values between client and server rocket states
    if (!clientState.c.empty()) {
        const RocketState& clientRocket = clientState.c[0];

        // Create current server rocket state
        RocketState serverRocket;
        player->createState(serverRocket);

        // Check position difference
        sf::Vector2f posDiff = clientRocket.b - serverRocket.b;
        float posDiffMag = std::sqrt(posDiff.x * posDiff.x + posDiff.y * posDiff.y);

        // Check velocity difference
        sf::Vector2f velDiff = clientRocket.c - serverRocket.c;
        float velDiffMag = std::sqrt(velDiff.x * velDiff.x + velDiff.y * velDiff.y);

        // If difference exceeds threshold, client simulation is invalid
        if (posDiffMag > j || velDiffMag > j * 10.0f) {
            isValid = false;

            // Update the client state with server state
            validatedState.c.clear();
            validatedState.c.push_back(serverRocket);
        }
    }

    // Update validation status
    i[playerId] = isValid;

    return validatedState;
}

void GameServer::synchronizeState()
{
    // For each player, check if their simulation is valid
    for (auto& playerPair : b) {
        int playerId = playerPair.first;

        // Skip validation for server player (ID 0)
        if (playerId == 0) continue;

        // Check if client simulation tracking exists for this player
        if (g.find(playerId) != g.end() && i.find(playerId) != i.end()) {
            // Check if client simulation is valid
            if (!i[playerId]) {
                // Client simulation invalid - next update will send correction
                continue;
            }

            // Check time since last update
            float timeSinceLastUpdate = f - h[playerId];
            if (timeSinceLastUpdate > 5.0f) {
                // Too long since last update, mark invalid
                i[playerId] = false;
            }
        }
    }
}

GameState GameServer::getGameState() const
{
    GameState state;
    state.a = e;
    state.b = f;
    state.e = false; // Not initial state by default

    try {
        // Add all player rockets
        for (const auto& playerPair : b) {
            int playerId = playerPair.first;
            const VehicleManager* player = playerPair.second;

            // Add null checks
            if (!player) continue;

            // Only add if it's a rocket
            if (player->getActiveVehicleType() == VehicleType::ROCKET) {
                const Rocket* rocket = player->getRocket();

                // Add null check for rocket
                if (!rocket) continue;

                RocketState rocketState;
                rocketState.a = playerId;  // playerId
                rocketState.b = rocket->getPosition();  // position
                rocketState.c = rocket->getVelocity();  // velocity
                rocketState.d = rocket->getRotation();  // rotation
                rocketState.e = 0.0f;  // angularVelocity - not tracked in current design
                rocketState.f = rocket->getThrustLevel();  // thrustLevel
                rocketState.g = rocket->getMass();  // mass
                rocketState.h = rocket->getColor();  // color
                rocketState.i = f;  // current server timestamp
                rocketState.j = true;  // Server state is authoritative

                state.c.push_back(rocketState);
            }
            // TODO: Add car state if needed
        }

        // Add all planets
        for (size_t i = 0; i < a.size(); ++i) {
            const Planet* planet = a[i];

            // Skip null planets
            if (!planet) continue;

            PlanetState planetState;
            planetState.a = static_cast<int>(i);  // planetId
            planetState.b = planet->getPosition();  // position
            planetState.c = planet->getVelocity();  // velocity
            planetState.d = planet->getMass();  // mass
            planetState.e = planet->getRadius();  // radius
            planetState.f = planet->getColor();  // color
            planetState.g = planet->getOwnerId();  // ownerId
            planetState.h = f;  // timestamp

            state.d.push_back(planetState);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in getGameState: " << ex.what() << std::endl;
        // Return a minimal valid state to avoid crashes
    }

    return state;
}

int GameServer::addPlayer(int playerId, sf::Vector2f initialPos, sf::Color color)
{
    // Check if player already exists
    if (b.find(playerId) != b.end()) {
        return playerId; // Player already exists
    }

    try {
        // Create a new vehicle manager for this player
        VehicleManager* player = new VehicleManager(initialPos, a, playerId);

        // Make sure rocket was initialized properly
        if (player && player->getRocket()) {
            player->getRocket()->setColor(color);

            // Add to simulator
            c.addVehicleManager(player);

            // Store in players map
            b[playerId] = player;

            // Initialize client simulation tracking
            g[playerId] = GameState();
            h[playerId] = f; // Current game time
            i[playerId] = true; // Initially valid

            std::cout << "Added player with ID: " << playerId << std::endl;
        }
        else {
            std::cerr << "Failed to initialize rocket for player ID: " << playerId << std::endl;
            delete player; // Clean up if rocket initialization failed
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception when adding player ID " << playerId << ": " << ex.what() << std::endl;
    }

    return playerId;
}

void GameServer::removePlayer(int playerId)
{
    auto playerIt = b.find(playerId);
    if (playerIt != b.end()) {
        VehicleManager* player = playerIt->second;

        // Remove from simulator
        c.removeVehicleManager(player);

        // Remove from map
        delete player;
        b.erase(playerIt);

        // Remove from client simulation tracking
        g.erase(playerId);
        h.erase(playerId);
        i.erase(playerId);

        std::cout << "Removed player " << playerId << std::endl;
    }
}

void GameServer::createSolarSystem()
{
    // Create main planet (sun)
    Planet* mainPlanet = new Planet(
        sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
        0, GameConstants::MAIN_PLANET_MASS, sf::Color::Yellow);
    mainPlanet->setVelocity(sf::Vector2f(1.f, -1.f));
    a.push_back(mainPlanet);

    // Create orbit planets with different orbital distances and sizes
    const float basePlanetMass = GameConstants::SECONDARY_PLANET_MASS;

    // Planet colors defined using sf::Color
    const sf::Color planetColors[] = {
        sf::Color(150, 150, 150),  // Mercury (gray)
        sf::Color(255, 190, 120),  // Venus (light orange)
        sf::Color(0, 100, 255),    // Earth (blue)
        sf::Color(255, 100, 0),    // Mars (red)
        sf::Color(255, 200, 100),  // Jupiter (light orange)
        sf::Color(230, 180, 80),   // Saturn (tan)
        sf::Color(180, 230, 230),  // Uranus (light blue)
        sf::Color(100, 130, 255),  // Neptune (dark blue)
        sf::Color(230, 230, 230)   // Pluto (light gray)
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
            planetColors[i]);

        planet->setVelocity(sf::Vector2f(velocityX, velocityY));
        a.push_back(planet);
    }
}