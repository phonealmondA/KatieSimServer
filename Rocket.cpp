// Rocket.cpp
#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <cmath>
#include <iostream>

Rocket::Rocket(sf::Vector2f pos, sf::Vector2f vel, int playerId, float mass, sf::Color color)
    : GameObject(pos, vel, color), a(0), b(0), c(0.0f), d(mass), e(color),
    f(playerId), g(0.0f), h(), i(true), j(0.0f), k(GameConstants::BASE_FUEL_CONSUMPTION_RATE)
{
    // Constructor implementation
}

void Rocket::applyThrust(float amount)
{
    // Calculate thrust direction based on rocket rotation
    float p = a * 3.14159f / 180.0f;
    sf::Vector2f q(std::sin(p), -std::cos(p));

    // Apply force with thrust multiplication
    velocity += q * amount * c * 1.0f / d;
}

void Rocket::rotate(float amount)
{
    b += amount;
}

void Rocket::setThrustLevel(float level)
{
    // Clamp level between 0.0 and 1.0
    c = std::max(0.0f, std::min(1.0f, level));
}

void Rocket::update(float deltaTime)
{
    // Update position based on velocity
    position += velocity * deltaTime;

    // Update rotation based on angular velocity
    a += b * deltaTime;

    // Apply damping to angular velocity
    b *= 0.98f;

    // Update timestamp
    g = static_cast<float>(std::time(nullptr));
}

void Rocket::setNearbyPlanets(const std::vector<Planet*>& planets)
{
    try {
        // Clear existing planets
        h.clear();

        // Only add non-null planets
        for (auto* planet : planets) {
            if (planet) {
                h.push_back(planet);
            }
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in setNearbyPlanets: " << ex.what() << std::endl;
        h.clear();
    }
}

bool Rocket::isColliding(const Planet& planet) const
{
    float dist = distance(position, planet.getPosition());
    return dist < planet.getRadius() + GameConstants::ROCKET_SIZE;
}

void Rocket::addStoredMass(float amount)
{
    j += amount;

    // Make sure stored mass doesn't go negative
    if (j < 0.0f) {
        j = 0.0f;
    }

    // Update total mass (base mass + stored mass)
    d = 1.0f + j;

    // Update timestamp
    g = static_cast<float>(std::time(nullptr));
}

RocketState Rocket::createState() const
{
    RocketState state;
    state.a = f;       // playerId = ownerId
    state.b = position; // position
    state.c = velocity; // velocity
    state.d = a;        // rotation
    state.e = b;        // angularVelocity
    state.f = c;        // thrustLevel
    state.g = d;        // mass
    state.h = e;        // color
    state.i = g;        // lastStateTimestamp
    state.j = true;     // isAuthoritative

    return state;
}

void Rocket::applyState(const RocketState& state)
{
    // Only apply if this state is for our rocket
    if (state.a != f) return;

    // Only apply if newer than our current state
    if (state.i <= g) return;

    position = state.b;
    velocity = state.c;
    a = state.d;
    b = state.e;
    c = state.f;
    d = state.g;
    e = state.h;
    g = state.i;
}