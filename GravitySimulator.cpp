// GravitySimulator.cpp
#include "GravitySimulator.h"
#include "VectorHelper.h"
#include <algorithm>
#include <cmath>

GravitySimulator::GravitySimulator()
    : simulatePlanetGravity(true)
{
}

void GravitySimulator::addPlanet(Planet* planet)
{
    if (planet) {
        planets.push_back(planet);
    }
}

void GravitySimulator::removePlanet(Planet* planet)
{
    auto it = std::find(planets.begin(), planets.end(), planet);
    if (it != planets.end()) {
        planets.erase(it);
    }
}

void GravitySimulator::addRocket(Rocket* rocket)
{
    if (rocket) {
        rockets.push_back(rocket);
    }
}

void GravitySimulator::removeRocket(Rocket* rocket)
{
    auto it = std::find(rockets.begin(), rockets.end(), rocket);
    if (it != rockets.end()) {
        rockets.erase(it);
    }
}

void GravitySimulator::update(float deltaTime)
{
    // Apply gravity between planets if enabled
    if (simulatePlanetGravity) {
        applyGravityBetweenPlanets(deltaTime);
    }

    // Apply gravity to rockets
    applyGravityToRockets(deltaTime);

    // Handle collisions
    handleCollisions();

    // Update positions
    for (auto planet : planets) {
        planet->update(deltaTime);
    }

    for (auto rocket : rockets) {
        rocket->update(deltaTime);
    }
}

void GravitySimulator::setSimulatePlanetGravity(bool enable)
{
    simulatePlanetGravity = enable;
}

const std::vector<Planet*>& GravitySimulator::getPlanets() const
{
    return planets;
}

const std::vector<Rocket*>& GravitySimulator::getRockets() const
{
    return rockets;
}

void GravitySimulator::applyGravityBetweenPlanets(float deltaTime)
{
    for (size_t i = 0; i < planets.size(); i++) {
        for (size_t j = i + 1; j < planets.size(); j++) {
            // Skip the first planet (index 0) - it's pinned in place
            if (i == 0) {
                // Only apply gravity from planet1 to planet2
                applyGravityToPlanet(planets[j], planets[i], deltaTime);
            }
            else {
                // Regular gravity calculation between other planets
                applyGravityToPlanet(planets[i], planets[j], deltaTime);
                applyGravityToPlanet(planets[j], planets[i], deltaTime);
            }
        }
    }
}

void GravitySimulator::applyGravityToPlanet(Planet* planet, Planet* otherPlanet, float deltaTime)
{
    sf::Vector2f direction = otherPlanet->getPosition() - planet->getPosition();
    float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

    // Skip if planets are colliding
    if (distance <= otherPlanet->getRadius() + planet->getRadius()) {
        return;
    }

    float forceMagnitude = G * otherPlanet->getMass() * planet->getMass() / (distance * distance);
    sf::Vector2f normalizedDir = normalize(direction);
    sf::Vector2f acceleration = normalizedDir * forceMagnitude / planet->getMass();

    planet->setVelocity(planet->getVelocity() + acceleration * deltaTime);
}

void GravitySimulator::applyGravityToRockets(float deltaTime)
{
    for (auto rocket : rockets) {
        for (auto planet : planets) {
            sf::Vector2f direction = planet->getPosition() - rocket->getPosition();
            float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            // Avoid division by zero and very small distances
            if (distance > planet->getRadius() + GameConstants::TRAJECTORY_COLLISION_RADIUS) {
                float forceMagnitude = G * planet->getMass() * rocket->getMass() / (distance * distance);
                sf::Vector2f acceleration = normalize(direction) * forceMagnitude / rocket->getMass();
                sf::Vector2f velocityChange = acceleration * deltaTime;
                rocket->setVelocity(rocket->getVelocity() + velocityChange);
            }
        }
    }
}

void GravitySimulator::handleCollisions()
{
    // Handle rocket-planet collisions
    for (auto rocket : rockets) {
        for (auto planet : planets) {
            sf::Vector2f direction = rocket->getPosition() - planet->getPosition();
            float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            // If rocket collides with planet
            if (distance <= planet->getRadius() + GameConstants::ROCKET_SIZE) {
                // Calculate normal force direction (away from planet center)
                sf::Vector2f normal = normalize(direction);

                // Project velocity onto normal to see if we're moving into the planet
                float velDotNormal = rocket->getVelocity().x * normal.x + rocket->getVelocity().y * normal.y;

                if (velDotNormal < 0) {
                    // Remove velocity component toward the planet
                    sf::Vector2f newVelocity = rocket->getVelocity() - normal * velDotNormal;

                    // Apply a small friction to velocity parallel to surface
                    sf::Vector2f tangent(-normal.y, normal.x);
                    float velDotTangent = newVelocity.x * tangent.x + newVelocity.y * tangent.y;
                    newVelocity = tangent * velDotTangent * GameConstants::FRICTION;

                    rocket->setVelocity(newVelocity);

                    // Position correction to stay exactly on surface
                    sf::Vector2f newPosition = planet->getPosition() + normal * (planet->getRadius() + GameConstants::ROCKET_SIZE);
                    rocket->setPosition(newPosition);
                }
            }
        }
    }

    // Handle planet-planet collisions (merge planets)
    // This is simplified compared to your game implementation
    for (size_t i = 1; i < planets.size(); i++) {  // Start at 1 to skip the sun
        for (size_t j = i + 1; j < planets.size(); j++) {
            sf::Vector2f direction = planets[j]->getPosition() - planets[i]->getPosition();
            float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);

            if (distance <= planets[i]->getRadius() + planets[j]->getRadius()) {
                // Determine which planet is larger
                if (planets[i]->getMass() >= planets[j]->getMass()) {
                    // Planet i absorbs planet j
                    float newMass = planets[i]->getMass() + planets[j]->getMass();

                    // Conservation of momentum for velocity
                    sf::Vector2f newVelocity = (planets[i]->getVelocity() * planets[i]->getMass() +
                        planets[j]->getVelocity() * planets[j]->getMass()) / newMass;

                    // Update planet i
                    planets[i]->setMass(newMass);
                    planets[i]->setVelocity(newVelocity);

                    // Remove planet j
                    removePlanet(planets[j]);
                    j--; // Adjust index since we removed an element
                }
                else {
                    // Planet j absorbs planet i
                    float newMass = planets[i]->getMass() + planets[j]->getMass();

                    // Conservation of momentum for velocity
                    sf::Vector2f newVelocity = (planets[i]->getVelocity() * planets[i]->getMass() +
                        planets[j]->getVelocity() * planets[j]->getMass()) / newMass;

                    // Update planet j
                    planets[j]->setMass(newMass);
                    planets[j]->setVelocity(newVelocity);

                    // Remove planet i
                    removePlanet(planets[i]);
                    i--; // Adjust index since we removed an element
                    break; // Break out of inner loop since planet i is gone
                }
            }
        }
    }
}