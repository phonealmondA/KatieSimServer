// GameObject.h
#pragma once
#include <SFML/System/Vector2.hpp>

class GameObject {
protected:
    sf::Vector2f position;
    sf::Vector2f velocity;

public:
    GameObject(sf::Vector2f pos, sf::Vector2f vel);
    virtual ~GameObject() = default;

    virtual void update(float deltaTime) = 0;

    sf::Vector2f getPosition() const;
    sf::Vector2f getVelocity() const;
    void setPosition(sf::Vector2f pos);
    void setVelocity(sf::Vector2f vel);
};