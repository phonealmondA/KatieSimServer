// GravitySimulator.h
#pragma once
#include <vector>
#include "Planet.h"
#include "Rocket.h"
#include "GameConstants.h"

// Forward declaration
class VehicleManager;

class GravitySimulator {
private:
    std::vector<Planet*> a; // planets
    std::vector<Rocket*> b; // rockets
    VehicleManager* c; // vehicleManager - used to update vehicles
    const float d; // G - gravitational constant
    bool e; // simulatePlanetGravity
    int f; // ownerId - which player this simulator belongs to (for filtering)

public:
    GravitySimulator(int ownerId = -1);

    void addPlanet(Planet* planet);
    void removePlanet(Planet* planet);
    void addRocket(Rocket* rocket);
    void removeRocket(Rocket* rocket);
    void addVehicleManager(VehicleManager* manager) { c = manager; }
    void update(float deltaTime);
    void clearRockets();

    const std::vector<Planet*>& getPlanets() const { return a; }
    const std::vector<Rocket*>& getRockets() const { return b; }
    void setSimulatePlanetGravity(bool enable) { e = enable; }

    // Set owner ID to limit simulation to owned objects
    void setOwnerId(int id) { f = id; }
    int getOwnerId() const { return f; }
    bool shouldSimulateObject(int objectOwnerId) const;

    void removeVehicleManager(VehicleManager* manager) { if (c == manager) { c = nullptr; } }

private:
    void applyGravityBetweenPlanets(float deltaTime);
    void applyGravityToRockets(float deltaTime);
    void addRocketGravityInteractions(float deltaTime);
    void checkPlanetCollisions();
    void updateVehicleManagerPlanets();
};