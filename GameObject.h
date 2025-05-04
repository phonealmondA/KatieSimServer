// GameObject.h
#pragma once
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics.hpp>

class GameObject {
protected:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;  // Make sure this is declared here for Planet to inherit

public:
    GameObject(sf::Vector2f pos, sf::Vector2f vel);
    GameObject(sf::Vector2f pos, sf::Vector2f vel, sf::Color col);
    virtual ~GameObject() = default;

    virtual void update(float deltaTime) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;

    sf::Vector2f getPosition() const;
    sf::Vector2f getVelocity() const;
    void setPosition(sf::Vector2f pos);
    void setVelocity(sf::Vector2f vel);
    sf::Color getColor() const { return color; }
};