// GravitySimulator.cpp
#include "GravitySimulator.h"
#include "VehicleManager.h"
#include "VectorHelper.h"
#include <algorithm>
#include <cmath>
#include <iostream>

GravitySimulator::GravitySimulator(int ownerId)
    : d(GameConstants::G), e(true), f(ownerId)
{
    // Constructor implementation
}

void GravitySimulator::addPlanet(Planet* planet)
{
    if (planet) {
        a.push_back(planet);
    }
}

void GravitySimulator::removePlanet(Planet* planet)
{
    auto it = std::find(a.begin(), a.end(), planet);
    if (it != a.end()) {
        a.erase(it);
    }
}

void GravitySimulator::addRocket(Rocket* rocket)
{
    if (rocket) {
        b.push_back(rocket);
    }
}

void GravitySimulator::removeRocket(Rocket* rocket)
{
    auto it = std::find(b.begin(), b.end(), rocket);
    if (it != b.end()) {
        b.erase(it);
    }
}

void GravitySimulator::clearRockets()
{
    b.clear();
}

bool GravitySimulator::shouldSimulateObject(int objectOwnerId) const
{
    // If no owner is set, simulate everything
    if (f == -1) return true;

    // If object has no owner, it's a common object - simulate it
    if (objectOwnerId == -1) return true;

    // Otherwise, only simulate objects owned by this simulator's owner
    return objectOwnerId == f;
}

void GravitySimulator::updateVehicleManagerPlanets() {
    if (!c) return;

    try {
        c->updatePlanets(a);
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in updateVehicleManagerPlanets: " << ex.what() << std::endl;
    }
}

void GravitySimulator::update(float deltaTime)
{
    // Apply gravity between planets if enabled
    if (e) {
        for (size_t i = 0; i < a.size(); i++) {
            // Only process planets we should simulate
            if (!shouldSimulateObject(a[i]->getOwnerId())) continue;

            for (size_t j = i + 1; j < a.size(); j++) {
                // Only process planets we should simulate
                if (!shouldSimulateObject(a[j]->getOwnerId())) continue;

                // Skip the first planet (index 0) - it's pinned in place
                if (i == 0) {
                    // Only apply gravity from planet1 to planet2
                    sf::Vector2f dir = a[i]->getPosition() - a[j]->getPosition();
                    float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);

                    if (dist > a[i]->getRadius() + a[j]->getRadius()) {
                        float force = d * a[i]->getMass() * a[j]->getMass() / (dist * dist);
                        sf::Vector2f normDir = normalize(dir);
                        sf::Vector2f accel = normDir * force / a[j]->getMass();
                        a[j]->setVelocity(a[j]->getVelocity() + accel * deltaTime);
                    }
                }
                else {
                    // Regular gravity calculation between other planets
                    sf::Vector2f dir = a[j]->getPosition() - a[i]->getPosition();
                    float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);

                    if (dist > a[i]->getRadius() + a[j]->getRadius()) {
                        float force = d * a[i]->getMass() * a[j]->getMass() / (dist * dist);
                        sf::Vector2f normDir = normalize(dir);
                        sf::Vector2f accel1 = normDir * force / a[i]->getMass();
                        sf::Vector2f accel2 = -normDir * force / a[j]->getMass();
                        a[i]->setVelocity(a[i]->getVelocity() + accel1 * deltaTime);
                        a[j]->setVelocity(a[j]->getVelocity() + accel2 * deltaTime);
                    }
                }
            }
        }
    }

    // Apply gravity to the vehicle manager's active vehicle
    if (c) {
        // Only apply if we should simulate this vehicle
        if (shouldSimulateObject(c->getOwnerId())) {
            if (c->getActiveVehicleType() == VehicleType::ROCKET) {
                Rocket* rocket = c->getRocket();
                if (rocket) {
                    for (auto planet : a) {
                        sf::Vector2f dir = planet->getPosition() - rocket->getPosition();
                        float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);

                        // Avoid division by zero and very small distances
                        if (dist > planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                            float force = d * planet->getMass() * rocket->getMass() / (dist * dist);
                            sf::Vector2f accel = normalize(dir) * force / rocket->getMass();
                            sf::Vector2f velChange = accel * deltaTime;
                            rocket->setVelocity(rocket->getVelocity() + velChange);
                        }
                    }
                }
            }
            // Car gravity is handled internally in Car::update
        }
    }
    else {
        // Legacy code for handling individual rockets
        for (auto rocket : b) {
            // Only process rockets we should simulate
            if (!shouldSimulateObject(rocket->getOwnerId())) continue;

            for (auto planet : a) {
                sf::Vector2f dir = planet->getPosition() - rocket->getPosition();
                float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);

                // Avoid division by zero and very small distances
                if (dist > planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                    float force = d * planet->getMass() * rocket->getMass() / (dist * dist);
                    sf::Vector2f accel = normalize(dir) * force / rocket->getMass();
                    sf::Vector2f velChange = accel * deltaTime;
                    rocket->setVelocity(rocket->getVelocity() + velChange);
                }
            }
        }

        // Add rocket-to-rocket gravity interactions
        addRocketGravityInteractions(deltaTime);
    }

    // Check for planet collisions and cleanup
    checkPlanetCollisions();
}

void GravitySimulator::addRocketGravityInteractions(float deltaTime)
{
    // Apply gravity between rockets
    for (size_t i = 0; i < b.size(); i++) {
        if (!shouldSimulateObject(b[i]->getOwnerId())) continue;

        for (size_t j = i + 1; j < b.size(); j++) {
            if (!shouldSimulateObject(b[j]->getOwnerId())) continue;

            Rocket* r1 = b[i];
            Rocket* r2 = b[j];

            sf::Vector2f dir = r2->getPosition() - r1->getPosition();
            float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);

            // Minimum distance to prevent extreme forces when very close
            const float minDist = GameConstants::TRAJECTORY_COLLISION_RADIUS;
            if (dist < minDist) {
                dist = minDist;
            }

            // Apply inverse square law for gravity
            float force = d * r1->getMass() * r2->getMass() / (dist * dist);

            sf::Vector2f normDir = normalize(dir);
            sf::Vector2f accel1 = normDir * force / r1->getMass();
            sf::Vector2f accel2 = -normDir * force / r2->getMass();

            r1->setVelocity(r1->getVelocity() + accel1 * deltaTime);
            r2->setVelocity(r2->getVelocity() + accel2 * deltaTime);
        }
    }
}

void GravitySimulator::checkPlanetCollisions() {
    if (a.size() < 2) return;

    std::vector<Planet*> keptPlanets;
    std::vector<Planet*> removedPlanets;
    std::vector<bool> markedForDeletion(a.size(), false);

    // First pass: identify small planets to remove and do null checks
    for (size_t i = 0; i < a.size(); i++) {
        Planet* planet = a[i];
        if (!planet) {
            // Mark null planets for deletion
            markedForDeletion[i] = true;
            continue;
        }

        // Only check planets we should simulate
        if (!shouldSimulateObject(planet->getOwnerId())) continue;

        // Check if the planet's mass is below threshold
        if (planet->getMass() < 10.0f) {
            removedPlanets.push_back(planet);
            markedForDeletion[i] = true;
            a[i] = nullptr; // Explicitly set to nullptr
        }
    }

    // Second pass: check for collisions between non-deleted planets
    for (size_t i = 0; i < a.size(); i++) {
        if (markedForDeletion[i]) continue; // Skip already marked planets

        Planet* p1 = a[i];
        if (!p1) continue; // Extra safety check

        // Only process planets we should simulate
        if (!shouldSimulateObject(p1->getOwnerId())) continue;

        for (size_t j = i + 1; j < a.size(); j++) {
            if (markedForDeletion[j]) continue; // Skip already marked planets

            Planet* p2 = a[j];
            if (!p2) continue; // Extra safety check

            // Only process planets we should simulate
            if (!shouldSimulateObject(p2->getOwnerId())) continue;

            // Calculate distance safely
            sf::Vector2f dir;
            try {
                dir = p2->getPosition() - p1->getPosition();
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception calculating planet distance: " << ex.what() << std::endl;
                continue; // Skip this pair
            }

            float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);

            // Check for collision
            if (dist <= p1->getRadius() + p2->getRadius()) {
                try {
                    // Determine which planet is larger
                    if (p1->getMass() >= p2->getMass()) {
                        // Planet 1 absorbs planet 2
                        float newMass = p1->getMass() + p2->getMass();

                        // Conservation of momentum for velocity
                        sf::Vector2f newVel = (p1->getVelocity() * p1->getMass() +
                            p2->getVelocity() * p2->getMass()) / newMass;

                        // Keep original owner
                        int ownerId = p1->getOwnerId();

                        // Update planet 1
                        p1->setMass(newMass);
                        p1->setVelocity(newVel);
                        p1->setOwnerId(ownerId);

                        // Mark planet 2 for deletion
                        if (!markedForDeletion[j]) {
                            removedPlanets.push_back(p2);
                            markedForDeletion[j] = true;
                            a[j] = nullptr; // Explicitly set to nullptr
                        }
                    }
                    else {
                        // Planet 2 absorbs planet 1
                        float newMass = p1->getMass() + p2->getMass();

                        // Conservation of momentum for velocity
                        sf::Vector2f newVel = (p1->getVelocity() * p1->getMass() +
                            p2->getVelocity() * p2->getMass()) / newMass;

                        // Keep original owner
                        int ownerId = p2->getOwnerId();

                        // Update planet 2
                        p2->setMass(newMass);
                        p2->setVelocity(newVel);
                        p2->setOwnerId(ownerId);

                        // Mark planet 1 for deletion
                        if (!markedForDeletion[i]) {
                            removedPlanets.push_back(p1);
                            markedForDeletion[i] = true;
                            a[i] = nullptr; // Explicitly set to nullptr
                        }
                        break; // Exit inner loop since planet 1 is gone
                    }
                }
                catch (const std::exception& ex) {
                    std::cerr << "Exception during planet collision: " << ex.what() << std::endl;
                    continue; // Skip this pair if an exception occurs
                }
            }
        }
    }

    // Build the filtered list of planets to keep
    for (size_t i = 0; i < a.size(); i++) {
        if (!markedForDeletion[i] && a[i]) {
            keptPlanets.push_back(a[i]);
        }
    }

    // Delete the marked planets safely
    for (Planet* planet : removedPlanets) {
        if (planet) { // Extra safety check
            try {
                delete planet;
            }
            catch (const std::exception& ex) {
                std::cerr << "Exception deleting planet: " << ex.what() << std::endl;
            }
        }
    }

    // Replace the planets vector with the filtered list
    a = keptPlanets;

    // Update planets in vehicle manager
    try {
        updateVehicleManagerPlanets();
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception updating vehicle manager planets: " << ex.what() << std::endl;
    }
}