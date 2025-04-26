// PlayerInput.h
#pragma once
#include <SFML/Network.hpp>

struct PlayerInput {
    int playerId;
    bool thrustForward;
    bool thrustBackward;
    bool rotateLeft;
    bool rotateRight;
    bool switchVehicle;
    float thrustLevel;
    float deltaTime;

    // Default constructor
    PlayerInput() : playerId(0), thrustForward(false), thrustBackward(false),
        rotateLeft(false), rotateRight(false), switchVehicle(false),
        thrustLevel(0.0f), deltaTime(0.0f) {
    }

    // Packet operators for serialization
    friend sf::Packet& operator <<(sf::Packet& packet, const PlayerInput& input);
    friend sf::Packet& operator >>(sf::Packet& packet, PlayerInput& input);
};

// Inline implementation of the packet operators
inline sf::Packet& operator <<(sf::Packet& packet, const PlayerInput& input) {
    return packet << input.playerId
        << input.thrustForward << input.thrustBackward
        << input.rotateLeft << input.rotateRight
        << input.switchVehicle << input.thrustLevel
        << input.deltaTime;
}

inline sf::Packet& operator >>(sf::Packet& packet, PlayerInput& input) {
    return packet >> input.playerId
        >> input.thrustForward >> input.thrustBackward
        >> input.rotateLeft >> input.rotateRight
        >> input.switchVehicle >> input.thrustLevel
        >> input.deltaTime;
}