// Planet.cpp
#include "Planet.h"
#include "GameConstants.h"
#include <cmath>
Planet::Planet(sf::Vector2f pos, float radius, float mass, sf::Color color)
    : GameObject(pos, { 0, 0 }, color), // Pass color to GameObject constructor
    mass(mass), // Just initialize mass directly
    radius(0), // Initialize radius to 0 before potentially setting it
    ownerId(-1) // Initialize ownerId to -1 (no owner)
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
void Planet::draw(sf::RenderWindow& window)
{
    // Server doesn't need to draw anything, this is just a stub implementation
    // to satisfy the linker
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

sf::Color Planet::getColor() const
{
    return color;
}

void Planet::setPosition(sf::Vector2f pos)
{
    position = pos;
}