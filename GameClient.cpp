// GameClient.cpp
#include "GameClient.h"
#include "GameConstants.h"
#include "VectorHelper.h"
#include <iostream>
#include <ctime>

GameClient::GameClient()
    : a(), // simulator
    b(), // planets
    c(), // remotePlayers
    d(nullptr), // localPlayer
    e(0), // localPlayerId
    f(), // lastState
    g(0.0f), // stateTimestamp
    h(), // remotePlayerStates
    i(0.05f), // latencyCompensation
    j(ClientConnectionState::DISCONNECTED), // connectionState
    k(false), // hasReceivedInitialState
    l(), // localSimulation
    m(), // simulationClock
    n(0.0f), // simulationTime
    o(false), // simulationPaused
    p(0.0f), // lastServerSyncTime
    q(0.1f), // syncInterval
    r(false) // pendingValidation
{
}

GameClient::~GameClient()
{
    // Clean up remote players
    for (auto& player : c) {
        delete player.second;
    }
    c.clear();

    // Clean up local player
    delete d;
    d = nullptr;

    // Clean up planets
    for (auto& planet : b) {
        delete planet;
    }
    b.clear();
}

void GameClient::initialize()
{
    try {
        // Mark waiting for initial state
        k = false;
        j = ClientConnectionState::CONNECTING;

        // Create placeholder planets until we get state from server
        Planet* mainPlanet = new Planet(
            sf::Vector2f(GameConstants::MAIN_PLANET_X, GameConstants::MAIN_PLANET_Y),
            0, GameConstants::MAIN_PLANET_MASS, sf::Color::Yellow);
        mainPlanet->setVelocity(sf::Vector2f(1.f, -1.f));
        b.push_back(mainPlanet);

        // Create secondary planet
        Planet* secondPlanet = new Planet(
            sf::Vector2f(GameConstants::SECONDARY_PLANET_X, GameConstants::SECONDARY_PLANET_Y),
            0, GameConstants::SECONDARY_PLANET_MASS, sf::Color::Green);
        secondPlanet->setVelocity(sf::Vector2f(0.f, GameConstants::SECONDARY_PLANET_ORBITAL_VELOCITY));
        b.push_back(secondPlanet);

        // Set up simulator with ID of local player
        a.setOwnerId(e);

        // Add planets to simulator
        for (auto planet : b) {
            a.addPlanet(planet);
        }

        // Create local player (placeholder until assigned ID)
        sf::Vector2f initialPos = b[0]->getPosition() +
            sf::Vector2f(0, -(b[0]->getRadius() + GameConstants::ROCKET_SIZE));

        // Create with null check
        if (d) {
            delete d;
            d = nullptr;
        }

        try {
            d = new VehicleManager(initialPos, b, e);
            if (!d || !d->getRocket()) {
                std::cerr << "Failed to create VehicleManager for local player" << std::endl;
                delete d;
                d = nullptr;
                throw std::runtime_error("Local player creation failed");
            }
        }
        catch (const std::exception& ex) {
            std::cerr << "Exception creating local player: " << ex.what() << std::endl;
            if (d) {
                delete d;
                d = nullptr;
            }
            throw;
        }

        // Add to simulator
        a.addVehicleManager(d);

        // Initialize local simulation
        initializeLocalSimulation();

        std::cout << "GameClient initialized successfully" << std::endl;
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in GameClient::initialize: " << ex.what() << std::endl;

        // Clean up resources
        for (auto& planet : b) {
            delete planet;
        }
        b.clear();

        delete d;
        d = nullptr;

        throw; // Re-throw to inform caller
    }
}

void GameClient::initializeLocalSimulation()
{
    // Start with an empty simulation
    l = GameState();
    l.a = 0; // Sequence number
    l.b = 0.0f; // Timestamp

    // Add local rocket if available
    if (d && d->getRocket()) {
        RocketState rocketState;
        d->createState(rocketState);
        l.c.push_back(rocketState);
    }

    // Add planets
    for (size_t i = 0; i < b.size(); ++i) {
        Planet* planet = b[i];
        if (!planet) continue;

        PlanetState planetState;
        planetState.a = static_cast<int>(i); // planetId
        planetState.b = planet->getPosition(); // position
        planetState.c = planet->getVelocity(); // velocity
        planetState.d = planet->getMass(); // mass
        planetState.e = planet->getRadius(); // radius
        planetState.f = planet->getColor(); // color
        planetState.g = planet->getOwnerId(); // ownerId
        planetState.h = 0.0f; // timestamp

        l.d.push_back(planetState);
    }

    // Start simulation clock
    m.restart();
    n = 0.0f;
    o = false;
}

void GameClient::update(float deltaTime)
{
    try {
        // Skip updates if not fully connected
        if (j != ClientConnectionState::CONNECTED && !k) {
            return;
        }

        // Update the simulation time
        if (!o) {
            n += deltaTime;
        }

        // Run local simulation if not paused
        if (!o) {
            runLocalSimulation(deltaTime);
        }

        // Update simulator with null checking
        a.update(deltaTime);

        // Update planets with null checking
        for (auto planet : b) {
            if (planet) {
                planet->update(deltaTime);
            }
        }

        // Update local player with null checking
        if (d) {
            d->update(deltaTime);
        }

        // Update remote players with null checking
        for (auto& player : c) {
            if (player.second) {
                player.second->update(deltaTime);
            }
        }

        // Interpolate remote players for smooth movement
        interpolateRemotePlayers(n);

        // Check if it's time to sync with server
        if (m.getElapsedTime().asSeconds() >= q && !o && !r) {
            // Update local simulation with current state
            if (d && d->getRocket()) {
                RocketState rocketState;
                d->createState(rocketState);

                // Update or add our rocket in the local simulation
                bool found = false;
                for (auto& rocket : l.c) {
                    if (rocket.a == e) {
                        rocket = rocketState;
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    l.c.push_back(rocketState);
                }

                // Update the timestamp
                l.b = n;

                // Set flag for sending to server
                r = true;
            }
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in GameClient::update: " << ex.what() << std::endl;
    }
}

void GameClient::runLocalSimulation(float deltaTime)
{
    // Skip if not ready
    if (j != ClientConnectionState::CONNECTED || !k) {
        return;
    }

    // Make local changes to the simulation
    if (d && d->getRocket()) {
        // Update the rocket state in local simulation
        RocketState rocketState;
        d->createState(rocketState);

        // Update or add this rocket in the simulation
        bool found = false;
        for (auto& rocket : l.c) {
            if (rocket.a == e) {
                rocket = rocketState;
                found = true;
                break;
            }
        }

        if (!found) {
            l.c.push_back(rocketState);
        }
    }

    // Update planets in local simulation
    for (size_t i = 0; i < b.size() && i < l.d.size(); ++i) {
        Planet* planet = b[i];
        if (!planet) continue;

        l.d[i].b = planet->getPosition();
        l.d[i].c = planet->getVelocity();
        l.d[i].d = planet->getMass();
    }

    // Update timestamp
    l.b = n;
}

void GameClient::processServerValidation(const GameState& validatedState)
{
    // Reset the validation pending flag
    r = false;

    // Update last server sync time
    p = n;

    // Check if server rejected our simulation
    if (validatedState.e) { // isInitialState flag is used to indicate server override
        // Server is overriding our state - pause simulation
        o = true;

        // Apply the server's state
        processGameState(validatedState);

        // Resume simulation
        o = false;

        // Reset simulation time to server time
        n = validatedState.b;

        // Reset the local simulation to match server state
        l = validatedState;
    }

    // Otherwise, server accepted our simulation - continue normally
}

void GameClient::processGameState(const GameState& state)
{
    try {
        // Don't process empty states
        if (state.d.empty()) {
            std::cerr << "Received empty game state, ignoring" << std::endl;
            return;
        }

        // Update last state
        f = state;
        g = state.b;

        // Update connection state if this is our first state
        if (!k) {
            std::cout << "Received initial game state with " << state.d.size() << " planets and "
                << state.c.size() << " rockets" << std::endl;

            j = ClientConnectionState::CONNECTED;
            k = true;
            std::cout << "Client now fully connected and ready for gameplay" << std::endl;

            // Initialize local simulation with this state
            l = state;
            n = state.b;
            m.restart();
        }

        // Process planets - ensure we have the right number of planets
        for (const auto& planetState : state.d) {
            if (planetState.a < 0) {
                std::cerr << "Invalid planet ID: " << planetState.a << std::endl;
                continue;
            }

            // Make sure we have enough planets
            while (planetState.a >= static_cast<int>(b.size())) {
                try {
                    Planet* planet = new Planet(sf::Vector2f(0, 0), 0, 1.0f);
                    b.push_back(planet);
                    a.addPlanet(planet);
                }
                catch (const std::exception& ex) {
                    std::cerr << "Exception creating new planet: " << ex.what() << std::endl;
                    break;
                }
            }

            // Update planet state
            if (planetState.a < static_cast<int>(b.size())) {
                Planet* planet = b[planetState.a];
                if (planet) {
                    planet->setPosition(planetState.b);
                    planet->setVelocity(planetState.c);
                    planet->setMass(planetState.d);
                    planet->setOwnerId(planetState.g); // Set owner ID
                }
            }
        }

        // Process rockets
        for (const auto& rocketState : state.c) {
            // First ensure we have a valid local player initialized
            if (!d && !b.empty()) {
                try {
                    sf::Vector2f initialPos = b[0]->getPosition() +
                        sf::Vector2f(0, -(b[0]->getRadius() + GameConstants::ROCKET_SIZE));
                    d = new VehicleManager(initialPos, b, e);
                    a.addVehicleManager(d);
                    std::cout << "Created local player (was null)" << std::endl;
                }
                catch (const std::exception& ex) {
                    std::cerr << "Exception creating local player: " << ex.what() << std::endl;
                    continue;
                }
            }

            // Process rocket based on player ID
            if (rocketState.a == e) {
                // This is our local player - only update if server state is authoritative
                if (rocketState.j) {
                    if (!d) {
                        // Create local player if it doesn't exist
                        try {
                            d = new VehicleManager(rocketState.b, b, e);
                            a.addVehicleManager(d);
                            std::cout << "Created local player with ID: " << e << std::endl;

                            // Verify the rocket was actually created
                            if (!d->getRocket()) {
                                std::cerr << "ERROR: Local player created but rocket is null!" << std::endl;
                                delete d;
                                d = nullptr;
                                continue;
                            }
                            else {
                                std::cout << "Successfully created rocket for local player" << std::endl;
                            }
                        }
                        catch (const std::exception& ex) {
                            std::cerr << "Exception creating local player: " << ex.what() << std::endl;
                            continue;
                        }
                    }

                    // Apply the server state to our local rocket
                    if (d && d->getRocket()) {
                        d->applyState(rocketState);
                        std::cout << "Updated local rocket position: " << rocketState.b.x << ", " << rocketState.b.y << std::endl;
                    }
                    else {
                        std::cerr << "ERROR: Local player exists but rocket is null after state update" << std::endl;
                    }
                }
            }
            else {
                // This is a remote player
                VehicleManager* remotePlayer = nullptr;
                auto it = c.find(rocketState.a);

                if (it == c.end()) {
                    // Create a new remote player
                    try {
                        remotePlayer = new VehicleManager(rocketState.b, b, rocketState.a);
                        if (remotePlayer && remotePlayer->getRocket()) {
                            c[rocketState.a] = remotePlayer;
                            a.addVehicleManager(remotePlayer);

                            // Set color based on player ID
                            remotePlayer->getRocket()->setColor(rocketState.h);

                            std::cout << "Added remote player with ID: " << rocketState.a << std::endl;
                        }
                        else {
                            std::cerr << "Failed to create valid VehicleManager for remote player" << std::endl;
                            delete remotePlayer; // Clean up if rocket initialization failed
                        }
                    }
                    catch (const std::exception& ex) {
                        std::cerr << "Exception creating remote player: " << ex.what() << std::endl;
                        delete remotePlayer; // Clean up on exception
                        continue;
                    }
                }
                else {
                    remotePlayer = it->second;
                }

                // Update rocket state with interpolation if manager exists
                if (remotePlayer && remotePlayer->getRocket()) {
                    Rocket* rocket = remotePlayer->getRocket();

                    // Store previous position for interpolation
                    sf::Vector2f prevPos = rocket->getPosition();
                    sf::Vector2f prevVel = rocket->getVelocity();

                    // Update with server values
                    rocket->setPosition(rocketState.b);
                    rocket->setVelocity(rocketState.c);
                    rocket->setRotation(rocketState.d);
                    rocket->setThrustLevel(rocketState.f);

                    // Store for interpolation
                    h[rocketState.a] = {
                        prevPos, prevVel,
                        rocketState.b, rocketState.c,
                        rocketState.d,
                        state.b
                    };
                }
            }
        }

        // Remove any players that weren't in the update
        std::vector<int> playersToRemove;
        for (const auto& player : c) {
            bool found = false;
            for (const auto& rocketState : state.c) {
                if (rocketState.a == player.first) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                playersToRemove.push_back(player.first);
            }
        }

        for (int playerId : playersToRemove) {
            std::cout << "Remote player " << playerId << " disconnected" << std::endl;
            if (c[playerId]) {
                a.removeVehicleManager(c[playerId]);
                delete c[playerId];
            }
            c.erase(playerId);
            h.erase(playerId);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in processGameState: " << ex.what() << std::endl;
    }
}

void GameClient::setLatencyCompensation(float value)
{
    i = value;
}

void GameClient::setLocalPlayerId(int id)
{
    e = id;
    j = ClientConnectionState::WAITING_FOR_STATE;
    std::cout << "Local player ID set to: " << id << ", waiting for initial game state..." << std::endl;

    // Also set the owner ID for local objects
    if (d) {
        d->setOwnerId(id);
        if (d->getRocket()) {
            d->getRocket()->setOwnerId(id);
        }
    }

    // Set simulator owner ID
    a.setOwnerId(id);
}

PlayerInput GameClient::getLocalPlayerInput(float deltaTime) const
{
    PlayerInput input;
    input.a = e; // playerId
    input.h = deltaTime; // deltaTime
    input.i = n; // clientTimestamp
    input.j = g; // lastServerStateTimestamp

    // Skip input collection if not fully connected
    if (j != ClientConnectionState::CONNECTED || !k || !d) {
        return input;
    }

    // Get keyboard state
    input.b = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W); // thrustForward
    input.c = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S); // thrustBackward
    input.d = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A); // rotateLeft
    input.e = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D); // rotateRight
    input.f = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L); // switchVehicle

    // Get thrust level
    if (d && d->getActiveVehicleType() == VehicleType::ROCKET && d->getRocket()) {
        input.g = d->getRocket()->getThrustLevel(); // thrustLevel

        // Include current rocket state
        if (r) { // If pending validation
            RocketState rocketState;
            d->createState(rocketState);
            input.k = rocketState; // clientRocketState
        }
    }

    return input;
}

void GameClient::applyLocalInput(const PlayerInput& input)
{
    // Skip if not fully connected or no local player
    if (!k || j != ClientConnectionState::CONNECTED || !d) {
        return;
    }

    try {
        // Apply input to local player immediately for responsive feel
        if (input.b) {
            d->applyThrust(1.0f);
        }
        if (input.c) {
            d->applyThrust(-0.5f);
        }
        if (input.d) {
            d->rotate(-6.0f * input.h * 60.0f);
        }
        if (input.e) {
            d->rotate(6.0f * input.h * 60.0f);
        }
        if (input.f) {
            d->switchVehicle();
        }

        // Apply thrust level
        if (d->getActiveVehicleType() == VehicleType::ROCKET && d->getRocket()) {
            d->getRocket()->setThrustLevel(input.g);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in applyLocalInput: " << ex.what() << std::endl;
    }
}

void GameClient::interpolateRemotePlayers(float currentTime)
{
    // Skip if not fully connected
    if (!k || j != ClientConnectionState::CONNECTED) {
        return;
    }

    try {
        for (auto it = h.begin(); it != h.end();) {
            int playerId = it->first;
            RemotePlayerState& playerState = it->second;

            auto playerIt = c.find(playerId);
            if (playerIt == c.end() || !playerIt->second || !playerIt->second->getRocket()) {
                // Remove stale state if player no longer exists
                it = h.erase(it);
                continue;
            }

            VehicleManager* player = playerIt->second;
            Rocket* rocket = player->getRocket();
            if (!rocket) {
                ++it;
                continue;
            }

            // Calculate interpolation factor
            float timeDiff = currentTime - playerState.f;
            float factor = std::min(timeDiff / i, 1.0f);

            // Interpolate position and velocity
            sf::Vector2f pos = playerState.a + (playerState.c - playerState.a) * factor;
            sf::Vector2f vel = playerState.b + (playerState.d - playerState.b) * factor;

            try {
                rocket->setPosition(pos);
                rocket->setVelocity(vel);
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception in interpolateRemotePlayers: " << ex.what() << std::endl;
            }

            ++it;
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in interpolateRemotePlayers: " << ex.what() << std::endl;
    }
}