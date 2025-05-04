// VehicleManager.h
#pragma once
#include "Rocket.h"
#include "Car.h"
#include "Planet.h"
#include <memory>
#include <vector>

enum class VehicleType {
    ROCKET,
    CAR
};

class VehicleManager {
private:
    std::unique_ptr<Rocket> a; // rocket
    std::unique_ptr<Car> b; // car
    VehicleType c; // activeVehicle
    std::vector<Planet*> d; // planets
    int e; // ownerId - which player owns this vehicle manager
    float f; // lastStateTimestamp - when the vehicle state was last updated

public:
    VehicleManager(sf::Vector2f initialPos, const std::vector<Planet*>& planetList, int ownerId = -1);

    void switchVehicle();
    void update(float deltaTime);
    void draw(sf::RenderWindow& window);
    void drawWithConstantSize(sf::RenderWindow& window, float zoomLevel);

    // Pass through functions to active vehicle
    void applyThrust(float amount);
    void rotate(float amount);
    void drawVelocityVector(sf::RenderWindow& window, float scale = 1.0f);

    // Ownership methods
    int getOwnerId() const { return e; }
    void setOwnerId(int id) { e = id; if (a) a->setOwnerId(id); }

    // State timing methods
    float getLastStateTimestamp() const { return f; }
    void setLastStateTimestamp(float timestamp) { f = timestamp; }

    // Modified to include null checks
    Rocket* getRocket() { return a ? a.get() : nullptr; }
    Rocket* getRocket() const { return a ? a.get() : nullptr; }
    Car* getCar() { return b ? b.get() : nullptr; }

    GameObject* getActiveVehicle();
    VehicleType getActiveVehicleType() const { return c; }

    // Add method to update planet references
    void updatePlanets(const std::vector<Planet*>& newPlanets);

    // Create state for serialization
    void createState(RocketState& state) const;
    // Apply state from deserialization
    void applyState(const RocketState& state);
};