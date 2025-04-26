// GameState.cpp
#include "GameState.h"

// Implement serialization for sf::Vector2f
sf::Packet& operator<<(sf::Packet& packet, const sf::Vector2f& vector) {
    return packet << vector.x << vector.y;
}

sf::Packet& operator>>(sf::Packet& packet, sf::Vector2f& vector) {
    return packet >> vector.x >> vector.y;
}

// Implement RocketState serialization
sf::Packet& operator<<(sf::Packet& packet, const RocketState& state) {
    return packet << state.playerId << state.position << state.velocity
        << state.rotation << state.angularVelocity << state.thrustLevel
        << state.mass << state.colorR << state.colorG << state.colorB;
}

sf::Packet& operator>>(sf::Packet& packet, RocketState& state) {
    return packet >> state.playerId >> state.position >> state.velocity
        >> state.rotation >> state.angularVelocity >> state.thrustLevel
        >> state.mass >> state.colorR >> state.colorG >> state.colorB;
}

// Implement PlanetState serialization
sf::Packet& operator<<(sf::Packet& packet, const PlanetState& state) {
    return packet << state.planetId << state.position << state.velocity
        << state.mass << state.radius << state.colorR << state.colorG << state.colorB;
}

sf::Packet& operator>>(sf::Packet& packet, PlanetState& state) {
    return packet >> state.planetId >> state.position >> state.velocity
        >> state.mass >> state.radius >> state.colorR >> state.colorG >> state.colorB;
}

// Implement GameState serialization
sf::Packet& operator<<(sf::Packet& packet, const GameState& state) {
    packet << static_cast<uint32_t>(state.sequenceNumber) << state.timestamp;

    // Serialize rockets
    packet << static_cast<uint32_t>(state.rockets.size());
    for (const auto& rocket : state.rockets) {
        packet << rocket;
    }

    // Serialize planets
    packet << static_cast<uint32_t>(state.planets.size());
    for (const auto& planet : state.planets) {
        packet << planet;
    }

    return packet;
}

sf::Packet& operator>>(sf::Packet& packet, GameState& state) {
    uint32_t seqNum;
    packet >> seqNum >> state.timestamp;
    state.sequenceNumber = seqNum;

    // Deserialize rockets
    uint32_t rocketCount;
    packet >> rocketCount;
    state.rockets.resize(rocketCount);
    for (uint32_t i = 0; i < rocketCount; ++i) {
        packet >> state.rockets[i];
    }

    // Deserialize planets
    uint32_t planetCount;
    packet >> planetCount;
    state.planets.resize(planetCount);
    for (uint32_t i = 0; i < planetCount; ++i) {
        packet >> state.planets[i];
    }

    return packet;
}