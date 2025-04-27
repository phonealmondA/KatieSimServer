// Planet.cpp
#include "Planet.h"
#include "GameConstants.h"
#include <cmath>

Planet::Planet(sf::Vector2f pos, float radius, float mass, uint8_t r, uint8_t g, uint8_t b)
    : GameObject(pos, { 0, 0 }), mass(mass), colorR(r), colorG(g), colorB(b)
{
    // If a specific radius was provided, use it
    if (radius > 0) {
        this->radius = radius;
    }
    else {
        // Otherwise calculate from mass
        updateRadiusFromMass();
    }
}

void Planet::update(float deltaTime)
{
    position += velocity * deltaTime;
}

float Planet::getMass() const
{
    return mass;
}

float Planet::getRadius() const
{
    return radius;
}

void Planet::setMass(float newMass)
{
    mass = newMass;
    updateRadiusFromMass();
}

void Planet::updateRadiusFromMass()
{
    // Use cube root relationship between mass and radius
    radius = GameConstants::BASE_RADIUS_FACTOR *
        std::pow(mass / GameConstants::REFERENCE_MASS, 1.0f / 3.0f);
}

void Planet::getColor(uint8_t& r, uint8_t& g, uint8_t& b) const
{
    r = colorR;
    g = colorG;
    b = colorB;
}

void Planet::setPosition(sf::Vector2f pos)
{
    position = pos;
}