// Rocket.cpp
#include "Rocket.h"
#include "VectorHelper.h"
#include "GameConstants.h"
#include <cmath>

Rocket::Rocket(sf::Vector2f pos, sf::Vector2f vel, int playerId, float mass,
    uint8_t r, uint8_t g, uint8_t b)
    : GameObject(pos, vel),
    rotation(0), angularVelocity(0), thrustLevel(0.0f),
    mass(mass), colorR(r), colorG(g), colorB(b), playerId(playerId)
{
}

void Rocket::applyThrust(float amount)
{
    // Calculate thrust direction based on rocket rotation
    float radians = rotation * 3.14159f / 180.0f;
    sf::Vector2f thrustDir(std::sin(radians), -std::cos(radians));

    // Apply force
    velocity += thrustDir * amount * thrustLevel * GameConstants::ENGINE_THRUST_POWER / mass;
}

void Rocket::rotate(float amount)
{
    angularVelocity += amount;
}

void Rocket::setThrustLevel(float level)
{
    // Clamp level between 0.0 and 1.0
    thrustLevel = std::max(0.0f, std::min(1.0f, level));
}

void Rocket::update(float deltaTime)
{
    // Update position based on velocity
    position += velocity * deltaTime;

    // Update rotation based on angular velocity
    rotation += angularVelocity * deltaTime;

    // Apply damping to angular velocity
    angularVelocity *= 0.98f;
}

float Rocket::getRotation() const
{
    return rotation;
}

void Rocket::setRotation(float rot)
{
    rotation = rot;
}

float Rocket::getThrustLevel() const
{
    return thrustLevel;
}

float Rocket::getMass() const
{
    return mass;
}

void Rocket::setMass(float newMass)
{
    mass = newMass;
}

int Rocket::getPlayerId() const
{
    return playerId;
}

void Rocket::getColor(uint8_t& r, uint8_t& g, uint8_t& b) const
{
    r = colorR;
    g = colorG;
    b = colorB;
}

void Rocket::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    colorR = r;
    colorG = g;
    colorB = b;
}