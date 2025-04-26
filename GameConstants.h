// GameConstants.h
#pragma once
#include <cmath>

namespace GameConstants {
    // Gravitational constants
    constexpr float G = 100.0f;  // Gravitational constant
    constexpr float PI = 3.14159265358979323846f;

    // Mass-radius relationship constants
    constexpr float BASE_RADIUS_FACTOR = 100.0f;  // Base size factor for planets
    constexpr float REFERENCE_MASS = 50000.0f;  // Reference mass for radius scaling

    // Primary inputs
    constexpr float MAIN_PLANET_MASS = 50000.0f;  // Primary parameter to adjust
    constexpr float ORBIT_PERIOD = 420.0f;  // Desired orbit period in seconds

    // Derived parameters
    constexpr float SECONDARY_PLANET_MASS = MAIN_PLANET_MASS * 0.06f;  // 6% of main planet mass

    // Fixed radius values
    constexpr float MAIN_PLANET_RADIUS = 100.0f;
    constexpr float SECONDARY_PLANET_RADIUS = 40.0f;

    // Planet positions
    constexpr float MAIN_PLANET_X = 400.0f;
    constexpr float MAIN_PLANET_Y = 300.0f;

    // Non-constexpr calculations for orbital parameters
    const float PLANET_ORBIT_DISTANCE = std::pow((G * MAIN_PLANET_MASS * ORBIT_PERIOD * ORBIT_PERIOD) / (4.0f * PI * PI), 1.0f / 3.0f);

    // Secondary planet position based on orbital distance
    const float SECONDARY_PLANET_X = MAIN_PLANET_X + PLANET_ORBIT_DISTANCE;
    const float SECONDARY_PLANET_Y = MAIN_PLANET_Y;

    // Pre-calculate orbital velocity for a circular orbit
    const float SECONDARY_PLANET_ORBITAL_VELOCITY = std::sqrt(G * MAIN_PLANET_MASS / PLANET_ORBIT_DISTANCE);

    // Rocket parameters
    constexpr float ROCKET_MASS = 1.0f;
    constexpr float ROCKET_SIZE = 15.0f;

    // Trajectory calculation settings
    constexpr float TRAJECTORY_TIME_STEP = 0.05f;
    constexpr int TRAJECTORY_STEPS = 5000;
    constexpr float TRAJECTORY_COLLISION_RADIUS = 12.0f;

    // Vehicle physics
    constexpr float FRICTION = 0.98f;
    constexpr float TRANSFORM_DISTANCE = 40.0f;
    constexpr float TRANSFORM_VELOCITY_FACTOR = 0.1f;

    // Engine parameters
    constexpr float BASE_THRUST_MULTIPLIER = 1.875f;
    constexpr float ENGINE_THRUST_POWER = G * BASE_THRUST_MULTIPLIER;

    // Fuel constants
    constexpr float BASE_FUEL_CONSUMPTION_RATE = 0.5f;

    // Server specific constants
    constexpr unsigned short DEFAULT_PORT = 5000;
    constexpr float SERVER_UPDATE_RATE = 0.05f;  // 20 updates per second
    constexpr int MAX_CLIENTS = 16;  // Maximum number of clients
    constexpr float CLIENT_TIMEOUT = 5.0f;  // Timeout in seconds
}