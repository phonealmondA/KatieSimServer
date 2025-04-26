// Rocket.h
#pragma once
#include "GameObject.h"
#include <vector>

class Planet;

class Rocket : public GameObject {
private:
    float rotation;
    float angularVelocity;
    float thrustLevel;
    float mass;
    uint8_t colorR, colorG, colorB;
    int playerId;

public:
    Rocket(sf::Vector2f pos, sf::Vector2f vel, int playerId, float mass = 1.0f,
        uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);

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
    void getColor(uint8_t& r, uint8_t& g, uint8_t& b) const;
    void setColor(uint8_t r, uint8_t g, uint8_t b);
};