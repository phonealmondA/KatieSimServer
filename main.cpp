// main.cpp
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>
#include <SFML/Network.hpp>
#include "ServerLogger.h"
#include "ServerConfig.h"
#include "ClientManager.h"
#include "NetworkManager.h"
#include "GameServer.h"
#include "GameState.h"
#include "PlayerInput.h"

#ifdef _DEBUG
#pragma comment(lib, "sfml-system-d.lib")
#pragma comment(lib, "sfml-network-d.lib")
#else
#pragma comment(lib, "sfml-system.lib")
#pragma comment(lib, "sfml-network.lib")
#endif





// Global flag for graceful shutdown
volatile bool running = true;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "Caught signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

// Parse command line arguments
void parseCommandLine(int argc, char* argv[], ServerConfig& config) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--port" && i + 1 < argc) {
            config.setPort(static_cast<unsigned short>(std::stoi(argv[++i])));
        }
        else if (arg == "--max-clients" && i + 1 < argc) {
            config.setMaxClients(std::stoi(argv[++i]));
        }
        else if (arg == "--update-rate" && i + 1 < argc) {
            config.setUpdateRate(std::stof(argv[++i]));
        }
        else if (arg == "--quiet") {
            config.setVerbose(false);
        }
        else if (arg == "--log" && i + 1 < argc) {
            config.setLogFile(argv[++i]);
        }
        else if (arg == "--help") {
            std::cout << "KatieServer - Standalone Game Server" << std::endl;
            std::cout << "Usage: KatieServer [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --port PORT          Set server port (default: 5000)" << std::endl;
            std::cout << "  --max-clients NUM    Set maximum number of clients (default: 16)" << std::endl;
            std::cout << "  --update-rate RATE   Set update rate in seconds (default: 0.05)" << std::endl;
            std::cout << "  --quiet              Disable verbose logging" << std::endl;
            std::cout << "  --log FILE           Specify log file path" << std::endl;
            std::cout << "  --help               Display this help message" << std::endl;
            exit(0);
        }
    }
}

int main(int argc, char* argv[]) {
    // Configure signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize configuration with defaults
    ServerConfig config;
    parseCommandLine(argc, argv, config);

    // Initialize logger
    ServerLogger logger(config.getLogFile(), config.isVerbose());
    logger.info("KatieServer starting up...");

    // Initialize client manager
    ClientManager clientManager(logger, config);

    // Initialize network manager
    NetworkManager networkManager(clientManager, logger, config);

    // Initialize game server
    GameServer gameServer(logger, config);
    gameServer.initialize();

    // Set up callbacks
    networkManager.setPlayerInputCallback([&gameServer](int clientId, const PlayerInput& input) {
        gameServer.handlePlayerInput(clientId, input);
        });

    networkManager.setClientDisconnectedCallback([&gameServer](int clientId) {
        gameServer.handlePlayerDisconnect(clientId);
        });

    networkManager.setClientAuthenticatedCallback([&gameServer](int clientId, const std::string& username) {
        gameServer.addPlayer(clientId);
        });

    // Start network manager
    if (!networkManager.start()) {
        logger.error("Failed to start network manager, exiting...");
        return 1;
    }

    logger.info("Server started successfully!");

    // Main loop timing variables
    auto lastUpdateTime = std::chrono::steady_clock::now();
    auto lastStatusTime = std::chrono::steady_clock::now();

    // Main server loop
    while (running) {
        // Calculate delta time
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastUpdateTime).count();

        if (deltaTime >= config.getUpdateRate()) {
            // Update game state
            gameServer.update(deltaTime);

            // Send game state to all clients
            GameState state = gameServer.getGameState();
            networkManager.sendGameState(state);

            // Reset timer
            lastUpdateTime = currentTime;
        }

        // Periodically log status (every 10 seconds)
        auto statusDuration = std::chrono::duration_cast<std::chrono::seconds>(currentTime - lastStatusTime).count();
        if (statusDuration >= 10) {
            clientManager.logClientInfo();
            lastStatusTime = currentTime;
        }

        // Sleep a bit to avoid hogging CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Graceful shutdown
    logger.info("Server shutting down...");
    networkManager.stop();

    return 0;
}