// Car.h
#pragma once
#include "GameObject.h"
#include <vector>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics.hpp>

// Forward declaration
class Rocket;
class Planet;

class Car : public GameObject {
public:
    Car(sf::Vector2f pos, sf::Vector2f vel) : GameObject(pos, vel) {}
    void update(float deltaTime) override {}
    void draw(sf::RenderWindow& window) override {}
    void checkGrounding(const std::vector<Planet*>& planets) {}
    bool isOnGround() const { return false; }
    void accelerate(float amount) {}
    void rotate(float amount) {}
    void initializeFromRocket(const Rocket* rocket) {}
    void drawWithConstantSize(sf::RenderWindow& window, float zoomLevel) {}
};