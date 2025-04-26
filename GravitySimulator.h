// GravitySimulator.h
#pragma once
#include <vector>
#include "Planet.h"
#include "Rocket.h"
#include "GameConstants.h"

class GravitySimulator {
private:
    std::vector<Planet*> planets;
    std::vector<Rocket*> rockets;
    const float G = GameConstants::G;
    bool simulatePlanetGravity;

public:
    GravitySimulator();

    void addPlanet(Planet* planet);
    void removePlanet(Planet* planet);
    void addRocket(Rocket* rocket);
    void removeRocket(Rocket* rocket);
    void update(float deltaTime);
    void setSimulatePlanetGravity(bool enable);

    const std::vector<Planet*>& getPlanets() const;
    const std::vector<Rocket*>& getRockets() const;

private:
    void applyGravityBetweenPlanets(float deltaTime);
    void applyGravityToPlanet(Planet* planet, Planet* otherPlanet, float deltaTime);
    void applyGravityToRockets(float deltaTime);
    void handleCollisions();
};