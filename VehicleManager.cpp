// VehicleManager.cpp
#include "VehicleManager.h"
#include "GameConstants.h"
#include "VectorHelper.h"
#include <iostream>

VehicleManager::VehicleManager(sf::Vector2f initialPos, const std::vector<Planet*>& planetList, int ownerId)
    : a(nullptr), b(nullptr), c(VehicleType::ROCKET), e(ownerId), f(0.0f)
{
    try {
        // First create rocket and car
        a = std::make_unique<Rocket>(initialPos, sf::Vector2f(0, 0), sf::Color::White, 1.0f, ownerId);
        b = std::make_unique<Car>(initialPos, sf::Vector2f(0, 0));

        // Handle planets safely
        if (!planetList.empty()) {
            // Only add non-null planets
            for (auto* planet : planetList) {
                if (planet) {
                    d.push_back(planet);
                }
            }

            // Set planets for rocket
            if (!d.empty() && a) {
                a->setNearbyPlanets(d);
            }
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in VehicleManager constructor: " << ex.what() << std::endl;

        // Make sure objects are created
        try {
            if (!a) a = std::make_unique<Rocket>(initialPos, sf::Vector2f(0, 0), sf::Color::White, 1.0f, ownerId);
            if (!b) b = std::make_unique<Car>(initialPos, sf::Vector2f(0, 0));
        }
        catch (const std::exception& ex2) {
            std::cerr << "Failed to create vehicles: " << ex2.what() << std::endl;
        }

        // Clear problematic planets
        d.clear();
    }
}

void VehicleManager::switchVehicle()
{
    if (!a || !b) {
        std::cerr << "Error: Missing vehicle in switchVehicle" << std::endl;
        return;
    }

    if (d.empty()) {
        return; // Can't switch without planets
    }

    if (c == VehicleType::ROCKET) {
        // Check if near a planet
        bool canSwitch = false;
        for (const auto& planet : d) {
            if (!planet) continue;

            float dist = distance(a->getPosition(), planet->getPosition());
            if (dist <= planet->getRadius() + GameConstants::TRANSFORM_DISTANCE) {
                canSwitch = true;
                break;
            }
        }

        if (canSwitch) {
            // Transfer rocket state to car
            b->initializeFromRocket(a.get());
            b->checkGrounding(d);
            c = VehicleType::CAR;
        }
    }
    else {
        // Only switch back if on ground
        if (b->isOnGround()) {
            // Transfer car state to rocket
            a->setPosition(b->getPosition());
            a->setVelocity(sf::Vector2f(0, 0));
            c = VehicleType::ROCKET;
        }
    }
}

void VehicleManager::update(float deltaTime)
{
    // Update with safety checks
    if (d.empty()) {
        // Still update the active vehicle
        if (c == VehicleType::ROCKET) {
            if (a) a->update(deltaTime);
        }
        else {
            if (b) b->update(deltaTime);
        }
        return;
    }

    // Create safe list of valid planets
    std::vector<Planet*> validPlanets;
    for (auto* planet : d) {
        if (planet) validPlanets.push_back(planet);
    }

    if (validPlanets.empty()) {
        // Still update the active vehicle
        if (c == VehicleType::ROCKET) {
            if (a) a->update(deltaTime);
        }
        else {
            if (b) b->update(deltaTime);
        }
        return;
    }

    // Update with valid planets
    if (c == VehicleType::ROCKET) {
        if (a) {
            try {
                a->setNearbyPlanets(validPlanets);
                a->update(deltaTime);
                // Update timestamp after successful update
                f = a->getLastStateTimestamp();
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception in rocket update: " << ex.what() << std::endl;
            }
        }
    }
    else {
        if (b) {
            try {
                b->checkGrounding(validPlanets);
                b->update(deltaTime);
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception in car update: " << ex.what() << std::endl;
            }
        }
    }
}

void VehicleManager::draw(sf::RenderWindow& window)
{
    if (!window.isOpen()) return;

    if (c == VehicleType::ROCKET) {
        if (a) {
            try {
                a->draw(window);
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception drawing rocket: " << ex.what() << std::endl;
            }
        }
    }
    else {
        if (b) {
            try {
                b->draw(window);
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception drawing car: " << ex.what() << std::endl;
            }
        }
    }
}

void VehicleManager::drawWithConstantSize(sf::RenderWindow& window, float zoomLevel)
{
    if (!window.isOpen()) return;

    if (c == VehicleType::ROCKET) {
        if (!a) {
            std::cerr << "Warning: Null rocket in drawWithConstantSize" << std::endl;
            return;
        }

        try {
            a->drawWithConstantSize(window, zoomLevel);
        }
        catch (const std::exception& ex) {
            std::cerr << "Exception drawing rocket: " << ex.what() << std::endl;
        }
    }
    else {
        if (!b) {
            std::cerr << "Warning: Null car in drawWithConstantSize" << std::endl;
            return;
        }

        try {
            b->drawWithConstantSize(window, zoomLevel);
        }
        catch (const std::exception& ex) {
            std::cerr << "Exception drawing car: " << ex.what() << std::endl;
        }
    }
}

void VehicleManager::applyThrust(float amount)
{
    if (c == VehicleType::ROCKET) {
        if (a) {
            try {
                a->applyThrust(amount);
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception applying thrust: " << ex.what() << std::endl;
            }
        }
    }
    else {
        if (b) {
            try {
                b->accelerate(amount);
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception accelerating car: " << ex.what() << std::endl;
            }
        }
    }
}

void VehicleManager::rotate(float amount)
{
    if (c == VehicleType::ROCKET) {
        if (a) {
            try {
                a->rotate(amount);
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception rotating rocket: " << ex.what() << std::endl;
            }
        }
    }
    else {
        if (b) {
            try {
                b->rotate(amount);
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception rotating car: " << ex.what() << std::endl;
            }
        }
    }
}

void VehicleManager::drawVelocityVector(sf::RenderWindow& window, float scale)
{
    if (!window.isOpen()) return;

    if (c == VehicleType::ROCKET) {
        if (a) {
            try {
                a->drawVelocityVector(window, scale);
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception drawing velocity vector: " << ex.what() << std::endl;
            }
        }
    }
    // Car doesn't have a velocity vector display
}

GameObject* VehicleManager::getActiveVehicle()
{
    if (c == VehicleType::ROCKET) {
        return a ? a.get() : nullptr;
    }
    else {
        return b ? b.get() : nullptr;
    }
}

void VehicleManager::updatePlanets(const std::vector<Planet*>& newPlanets)
{
    try {
        // Clear current planets first
        d.clear();

        // Copy valid planets
        for (auto* planet : newPlanets) {
            if (planet) {
                d.push_back(planet);
            }
        }

        // Update rocket's planets
        if (a) {
            a->setNearbyPlanets(d);
        }

        // Update car's planets if needed
        if (b) {
            b->checkGrounding(d);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in updatePlanets: " << ex.what() << std::endl;
    }
}

void VehicleManager::createState(RocketState& state) const
{
    if (a && c == VehicleType::ROCKET) {
        // Set player ID based on owner
        state.a = e;

        // Set position, velocity, rotation
        state.b = a->getPosition();
        state.c = a->getVelocity();
        state.d = a->getRotation();

        // Set other rocket properties
        state.e = 0.0f; // Angular velocity not tracked directly
        state.f = a->getThrustLevel();
        state.g = a->getMass();
        state.h = a->getColor();

        // Set timestamp
        state.i = f;

        // Flag this as authoritative for this client
        state.j = true;
    }
    else {
        // Create an empty state if no rocket exists
        state.a = e;
        state.b = sf::Vector2f(0, 0);
        state.c = sf::Vector2f(0, 0);
        state.d = 0.0f;
        state.e = 0.0f;
        state.f = 0.0f;
        state.g = 1.0f;
        state.h = sf::Color::White;
        state.i = f;
        state.j = false;
    }
}

void VehicleManager::applyState(const RocketState& state)
{
    // Only apply states for our owner
    if (state.a != e) return;

    // Only apply if we're in rocket mode
    if (c != VehicleType::ROCKET || !a) return;

    // Only apply if the state is newer than our current state
    if (state.i <= f) return;

    // Apply the rocket state
    a->setPosition(state.b);
    a->setVelocity(state.c);
    a->setRotation(state.d);
    a->setThrustLevel(state.f);

    // Update timestamp
    f = state.i;
    a->setLastStateTimestamp(state.i);
}