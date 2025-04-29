// Planet.h
#pragma once
#include "GameObject.h"
#include <SFML/Graphics/Color.hpp>
#include <vector>

class Planet : public GameObject {
private:
    float mass;
    float radius;
    sf::Color color;  // Changed to sf::Color

public:
    Planet(sf::Vector2f pos, float radius, float mass, sf::Color color = sf::Color::Blue);
    void update(float deltaTime) override;

    float getMass() const;
    float getRadius() const;
    void setMass(float newMass);
    void updateRadiusFromMass();
    sf::Color getColor() const;  // Changed to return sf::Color
    void setPosition(sf::Vector2f pos);
};