// Rocket.h
#pragma once
#include "GameObject.h"
#include "GameConstants.h"
#include "Planet.h"
#include "GameState.h"
#include <vector>
#include <SFML/Graphics/Color.hpp>

class Rocket : public GameObject {
private:
    float a; // rotation
    float b; // angularVelocity
    float c; // thrustLevel
    float d; // mass
    sf::Color e; // color
    int f; // playerId/ownerId
    float g; // lastStateTimestamp - when rocket state was last updated
    std::vector<Planet*> h; // nearbyPlanets for collision detection
    bool i; // hasFuel - tracking if rocket has fuel
    float j; // storedMass - amount of mass taken from planets
    float k; // fuelConsumptionRate - how fast fuel is used

public:
    Rocket(sf::Vector2f pos, sf::Vector2f vel, int playerId, float mass = 1.0f,
        sf::Color color = sf::Color::White);

    void applyThrust(float amount);
    void rotate(float amount);
    void setThrustLevel(float level);
    void update(float deltaTime) override;
    void setNearbyPlanets(const std::vector<Planet*>& planets);
    bool isColliding(const Planet& planet) const;

    // State methods
    RocketState createState() const;
    void applyState(const RocketState& state);

    // Getters/setters
    float getRotation() const { return a; }
    void setRotation(float rot) { a = rot; }
    float getThrustLevel() const { return c; }
    float getMass() const { return d; }
    void setMass(float newMass) { d = newMass; }
    int getPlayerId() const { return f; }
    int getOwnerId() const { return f; }
    void setOwnerId(int id) { f = id; }
    float getLastStateTimestamp() const { return g; }
    void setLastStateTimestamp(float timestamp) { g = timestamp; }
    bool hasFuel() const { return i; }
    float getStoredMass() const { return j; }
    void addStoredMass(float amount);
    const std::vector<Planet*>& getNearbyPlanets() const { return h; }
    sf::Color getColor() const { return e; }
    void setColor(sf::Color newColor) { e = newColor; }
};