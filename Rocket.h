// Rocket.h
#pragma once
#include "GameObject.h"
#include <vector>
#include <SFML/Graphics/Color.hpp>

class Rocket : public GameObject {
private:
    float rotation;
    float angularVelocity;
    float thrustLevel;
    float mass;
    sf::Color color;  // Changed to sf::Color
    int playerId;

public:
    Rocket(sf::Vector2f pos, sf::Vector2f vel, int playerId, float mass = 1.0f,
        sf::Color color = sf::Color::White);  // Changed to use sf::Color

    void applyThrust(float amount);
    void rotate(float amount);
    void setThrustLevel(float level);
    void update(float deltaTime) override;

    float getRotation() const;
    void setRotation(float rot);
    float getThrustLevel() const;
    float getMass() const;
    void setMass(float newMass);
    int getPlayerId() const;
    sf::Color getColor() const;  // Changed to return sf::Color
    void setColor(sf::Color newColor);  // Changed to accept sf::Color
};