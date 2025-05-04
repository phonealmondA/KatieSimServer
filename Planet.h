// Planet.h
#pragma once
#include "GameObject.h"
#include <SFML/Graphics.hpp>

class Planet : public GameObject {
private:
    float mass;
    float radius;
    int ownerId; // Added owner ID
    // Color is already in GameObject as it inherits from it

public:
    Planet(sf::Vector2f pos, float radius, float mass, sf::Color color = sf::Color::Blue);
    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;

    float getMass() const;
    float getRadius() const;
    void setMass(float newMass);
    void updateRadiusFromMass();
    sf::Color getColor() const;
    void setPosition(sf::Vector2f pos);

    // Add owner ID getters/setters
    int getOwnerId() const { return ownerId; }
    void setOwnerId(int id) { ownerId = id; }
};