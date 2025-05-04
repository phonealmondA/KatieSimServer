// GameState.cpp
#include "GameState.h"

// Implement serialization for sf::Vector2f
sf::Packet& operator<<(sf::Packet& packet, const sf::Vector2f& vector) {
    return packet << vector.x << vector.y;
}

sf::Packet& operator>>(sf::Packet& packet, sf::Vector2f& vector) {
    return packet >> vector.x >> vector.y;
}

// Implement serialization for sf::Color
sf::Packet& operator<<(sf::Packet& packet, const sf::Color& color) {
    return packet << color.r << color.g << color.b << color.a;
}

sf::Packet& operator>>(sf::Packet& packet, sf::Color& color) {
    return packet >> color.r >> color.g >> color.b >> color.a;
}

// Implement RocketState serialization
sf::Packet& operator<<(sf::Packet& packet, const RocketState& state) {
    return packet << state.a << state.b << state.c
        << state.d << state.e << state.f
        << state.g << state.h << state.i << state.j;
}

sf::Packet& operator>>(sf::Packet& packet, RocketState& state) {
    return packet >> state.a >> state.b >> state.c
        >> state.d >> state.e >> state.f
        >> state.g >> state.h >> state.i >> state.j;
}

// Implement PlanetState serialization
sf::Packet& operator<<(sf::Packet& packet, const PlanetState& state) {
    return packet << state.a << state.b << state.c
        << state.d << state.e << state.f << state.g << state.h;
}

sf::Packet& operator>>(sf::Packet& packet, PlanetState& state) {
    return packet >> state.a >> state.b >> state.c
        >> state.d >> state.e >> state.f >> state.g >> state.h;
}

// Implement GameState serialization
sf::Packet& operator<<(sf::Packet& packet, const GameState& state) {
    packet << static_cast<uint32_t>(state.a) << state.b << state.e;

    // Serialize rockets
    packet << static_cast<uint32_t>(state.c.size());
    for (const auto& rocket : state.c) {
        packet << rocket;
    }

    // Serialize planets
    packet << static_cast<uint32_t>(state.d.size());
    for (const auto& planet : state.d) {
        packet << planet;
    }

    return packet;
}

sf::Packet& operator>>(sf::Packet& packet, GameState& state) {
    uint32_t seqNum;
    packet >> seqNum >> state.b >> state.e;
    state.a = seqNum;

    // Deserialize rockets
    uint32_t rocketCount;
    packet >> rocketCount;
    state.c.resize(rocketCount);
    for (uint32_t i = 0; i < rocketCount; ++i) {
        packet >> state.c[i];
    }

    // Deserialize planets
    uint32_t planetCount;
    packet >> planetCount;
    state.d.resize(planetCount);
    for (uint32_t i = 0; i < planetCount; ++i) {
        packet >> state.d[i];
    }

    return packet;
}