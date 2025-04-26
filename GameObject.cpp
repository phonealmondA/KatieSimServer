// GameObject.cpp
#include "GameObject.h"

GameObject::GameObject(sf::Vector2f pos, sf::Vector2f vel)
    : position(pos), velocity(vel)
{
}

sf::Vector2f GameObject::getPosition() const
{
    return position;
}

sf::Vector2f GameObject::getVelocity() const
{
    return velocity;
}

void GameObject::setPosition(sf::Vector2f pos)
{
    position = pos;
}

void GameObject::setVelocity(sf::Vector2f vel)
{
    velocity = vel;
}