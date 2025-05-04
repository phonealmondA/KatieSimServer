// ClientData.h
#pragma once
#include <SFML/Network.hpp>
#include <chrono>
#include <string>
#include <SFML/Graphics.hpp>
struct ClientData {
    int clientId;
    sf::TcpSocket* socket;
    sf::IpAddress address;
    unsigned short port;
    std::chrono::steady_clock::time_point lastActivity;
    bool authenticated;
    std::string username;
    int pingMs;
    int packetLoss;
    bool pendingDisconnect;

    ClientData(int id, sf::TcpSocket* sock)
        : clientId(id),
        socket(sock),
        // Use the factory method in SFML 3.0
        address(sock&& sock->getRemoteAddress() ? *sock->getRemoteAddress() : sf::IpAddress::Any),
        port(0),
        lastActivity(std::chrono::steady_clock::now()),
        authenticated(false),
        username("Player_" + std::to_string(id)),
        pingMs(0),
        packetLoss(0),
        pendingDisconnect(false)
    {
    }

    ~ClientData() {
        if (socket) {
            socket->disconnect();
            delete socket;
            socket = nullptr;
        }
    }

    void updateActivity() {
        lastActivity = std::chrono::steady_clock::now();
    }

    bool isTimedOut(float timeoutSeconds) const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastActivity).count();
        return duration > timeoutSeconds;
    }
};