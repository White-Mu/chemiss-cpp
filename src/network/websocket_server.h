#ifndef CHEMISS_WEBSOCKET_SERVER_H
#define CHEMISS_WEBSOCKET_SERVER_H

#include "../game/game_state.h"
#include <string>
#include <vector>
#include <thread>
#include <map>
#include <mutex>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

namespace chemiss {

struct ClientConnection {
    int socket;
    bool isWebSocket;
    std::string buffer;
};

class WebSocketServer {
public:
    WebSocketServer(int port);
    ~WebSocketServer();
    
    bool start();
    void stop();
    void run();
    
    void broadcast(const std::string& message);
    void sendTo(int clientSocket, const std::string& message);
    
    void setGameState(GameState* state) { gameState_ = state; }
    
private:
    int port_;
    int serverSocket_;
    bool running_;
    GameState* gameState_;
    std::vector<ClientConnection> clients_;
    std::mutex clientsMutex_;
    std::thread acceptThread_;
    
    bool initSocket();
    void acceptConnections();
    void handleClient(int clientSocket);
    
    // HTTP handling
    bool handleHttpRequest(int clientSocket, const std::string& request);
    bool serveStaticFile(int clientSocket, const std::string& path);
    std::string getContentType(const std::string& path);
    
    // WebSocket handling
    bool handleWebSocketUpgrade(int clientSocket, const std::string& request);
    bool performWebSocketHandshake(int clientSocket, const std::string& request);
    std::string computeWebSocketAccept(const std::string& key);
    std::string decodeWebSocketFrame(const std::string& data, bool& fin, int& opcode);
    std::string encodeWebSocketFrame(const std::string& data, int opcode = 0x1);
    
    // Message handling
    void processMessage(int clientSocket, const std::string& message);
    
    // Close socket helper
    void closeSocket(int sock);
};

} // namespace chemiss

#endif // CHEMISS_WEBSOCKET_SERVER_H
