// GameState.h
#pragma once
#include <SFML/Network.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>
#include <vector>
#include <SFML/Graphics.hpp>
// Forward declarations for packet operators
struct RocketState;
struct PlanetState;
struct GameState;

// Packet operators for sf::Vector2f
sf::Packet& operator<<(sf::Packet& packet, const sf::Vector2f& vector);
sf::Packet& operator>>(sf::Packet& packet, sf::Vector2f& vector);

// Packet operators for sf::Color
sf::Packet& operator<<(sf::Packet& packet, const sf::Color& color);
sf::Packet& operator>>(sf::Packet& packet, sf::Color& color);

// Serializable state for a rocket
struct RocketState {
    int a; // playerId
    sf::Vector2f b; // position
    sf::Vector2f c; // velocity
    float d; // rotation
    float e; // angularVelocity
    float f; // thrustLevel
    float g; // mass
    sf::Color h; // color
    float i; // timestamp of this state
    bool j; // isAuthoritative - whether this is definitive state from server

    // Packet operators for serialization
    friend sf::Packet& operator <<(sf::Packet& packet, const RocketState& state);
    friend sf::Packet& operator >>(sf::Packet& packet, RocketState& state);
};

// Serializable state for a planet
struct PlanetState {
    int a; // planetId
    sf::Vector2f b; // position
    sf::Vector2f c; // velocity
    float d; // mass
    float e; // radius
    sf::Color f; // color
    int g; // ownerId - which player owns/created this planet
    float h; // timestamp of this state

    // Packet operators for serialization
    friend sf::Packet& operator <<(sf::Packet& packet, const PlanetState& state);
    friend sf::Packet& operator >>(sf::Packet& packet, PlanetState& state);
};

// Complete game state for synchronization
struct GameState {
    unsigned long a; // sequenceNumber
    float b; // timestamp
    std::vector<RocketState> c; // rockets
    std::vector<PlanetState> d; // planets
    bool e; // isInitialState - if this is the first state sent to client

    // Packet operators for serialization
    friend sf::Packet& operator <<(sf::Packet& packet, const GameState& state);
    friend sf::Packet& operator >>(sf::Packet& packet, GameState& state);
};