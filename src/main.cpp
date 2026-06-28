#include "network/websocket_server.h"
#include <iostream>

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    chemiss::GameState gameState;
    gameState.startNewGame();

    chemiss::WebSocketServer server(port);
    server.setGameState(&gameState);

    if (!server.start()) {
        std::cerr << "Failed to start server on port " << port << std::endl;
        return 1;
    }

    std::cout << "Press Enter to stop the server..." << std::endl;
    std::cin.get();

    server.stop();
    return 0;
}
