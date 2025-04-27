// Planet.h
#pragma once
#include "GameObject.h"
#include <vector>

class Planet : public GameObject {
private:
    float mass;
    float radius;
    uint8_t colorR, colorG, colorB;

public:
    Planet(sf::Vector2f pos, float radius, float mass, uint8_t r = 0, uint8_t g = 0, uint8_t b = 255);
    void update(float deltaTime) override;

    float getMass() const;
    float getRadius() const;
    void setMass(float newMass);
    void updateRadiusFromMass();
    void getColor(uint8_t& r, uint8_t& g, uint8_t& b) const;
    void setPosition(sf::Vector2f pos);
};