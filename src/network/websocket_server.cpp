#include "websocket_server.h"
#include "../utils/json_utils.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <algorithm>

namespace chemiss {

// SHA-1 implementation
class SHA1 {
public:
    SHA1() { reset(); }
    void reset() {
        length_low = 0; length_high = 0;
        message_block_index = 0;
        H[0] = 0x67452301; H[1] = 0xEFCDAB89;
        H[2] = 0x98BADCFE; H[3] = 0x10325476; H[4] = 0xC3D2E1F0;
        computed = false; corrupted = false;
    }
    void update(const unsigned char* data, size_t len) {
        for (size_t i = 0; i < len && !corrupted; i++) {
            message_block[message_block_index++] = data[i];
            length_low += 8;
            if (length_low == 0) { length_high++; if (length_high == 0) corrupted = true; }
            if (message_block_index == 64) processMessageBlock();
        }
    }
    void final(unsigned char digest[20]) {
        if (corrupted) return;
        if (!computed) padMessage();
        for (int i = 0; i < 5; i++) {
            digest[i*4] = (H[i] >> 24) & 0xFF;
            digest[i*4+1] = (H[i] >> 16) & 0xFF;
            digest[i*4+2] = (H[i] >> 8) & 0xFF;
            digest[i*4+3] = H[i] & 0xFF;
        }
        reset();
    }
private:
    unsigned long H[5];
    unsigned long length_low, length_high;
    unsigned char message_block[64];
    int message_block_index;
    bool computed, corrupted;
    void processMessageBlock() {
        const unsigned long K[4] = {0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6};
        unsigned long W[80];
        for (int t = 0; t < 16; t++) {
            W[t] = ((unsigned long)message_block[t*4]) << 24;
            W[t] |= ((unsigned long)message_block[t*4+1]) << 16;
            W[t] |= ((unsigned long)message_block[t*4+2]) << 8;
            W[t] |= ((unsigned long)message_block[t*4+3]);
        }
        for (int t = 16; t < 80; t++) W[t] = circularShift(1, W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
        unsigned long A = H[0], B = H[1], C = H[2], D = H[3], E = H[4];
        for (int t = 0; t < 80; t++) {
            unsigned long temp = circularShift(5, A) + f(t, B, C, D) + E + W[t] + K[t/20];
            E = D; D = C; C = circularShift(30, B); B = A; A = temp;
        }
        H[0] += A; H[1] += B; H[2] += C; H[3] += D; H[4] += E;
        message_block_index = 0;
    }
    unsigned long circularShift(int bits, unsigned long word) {
        return ((word << bits) & 0xFFFFFFFF) | ((word & 0xFFFFFFFF) >> (32 - bits));
    }
    unsigned long f(int t, unsigned long B, unsigned long C, unsigned long D) {
        if (t < 20) return (B & C) | ((~B) & D);
        if (t < 40) return B ^ C ^ D;
        if (t < 60) return (B & C) | (B & D) | (C & D);
        return B ^ C ^ D;
    }
    void padMessage() {
        message_block[message_block_index++] = 0x80;
        if (message_block_index > 56) {
            while (message_block_index < 64) message_block[message_block_index++] = 0;
            processMessageBlock();
        }
        while (message_block_index < 56) message_block[message_block_index++] = 0;
        message_block[56] = (length_high >> 24) & 0xFF;
        message_block[57] = (length_high >> 16) & 0xFF;
        message_block[58] = (length_high >> 8) & 0xFF;
        message_block[59] = length_high & 0xFF;
        message_block[60] = (length_low >> 24) & 0xFF;
        message_block[61] = (length_low >> 16) & 0xFF;
        message_block[62] = (length_low >> 8) & 0xFF;
        message_block[63] = length_low & 0xFF;
        processMessageBlock();
        computed = true;
    }
};

std::string base64Encode(const unsigned char* data, size_t len) {
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    for (size_t i = 0; i < len; i += 3) {
        unsigned long val = data[i] << 16;
        if (i + 1 < len) val |= data[i+1] << 8;
        if (i + 2 < len) val |= data[i+2];
        result += chars[(val >> 18) & 0x3F];
        result += chars[(val >> 12) & 0x3F];
        result += (i + 1 < len) ? chars[(val >> 6) & 0x3F] : '=';
        result += (i + 2 < len) ? chars[val & 0x3F] : '=';
    }
    return result;
}

WebSocketServer::WebSocketServer(int port)
    : port_(port), serverSocket_(-1), running_(false), gameState_(nullptr) {}

WebSocketServer::~WebSocketServer() {
    stop();
}

bool WebSocketServer::start() {
    if (!initSocket()) return false;
    running_ = true;
    acceptThread_ = std::thread(&WebSocketServer::acceptConnections, this);
    std::cout << "Chemiss server started on port " << port_ << std::endl;
    std::cout << "Open http://localhost:" << port_ << " in your browser to play." << std::endl;
    return true;
}

void WebSocketServer::stop() {
    running_ = false;
    if (serverSocket_ >= 0) {
        closeSocket(serverSocket_);
        serverSocket_ = -1;
    }
    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (auto& client : clients_) {
            closeSocket(client.socket);
        }
        clients_.clear();
    }
}

void WebSocketServer::run() {
    acceptThread_.join();
}

void WebSocketServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    std::string frame = encodeWebSocketFrame(message, 0x1);
    for (auto& client : clients_) {
        if (client.isWebSocket) {
#ifdef _WIN32
            send(client.socket, frame.c_str(), (int)frame.size(), 0);
#else
            send(client.socket, frame.c_str(), frame.size(), 0);
#endif
        }
    }
}

void WebSocketServer::sendTo(int clientSocket, const std::string& message) {
    std::string frame = encodeWebSocketFrame(message, 0x1);
#ifdef _WIN32
    send(clientSocket, frame.c_str(), (int)frame.size(), 0);
#else
    send(clientSocket, frame.c_str(), frame.size(), 0);
#endif
}

bool WebSocketServer::initSocket() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;
#endif

    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) return false;

    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(serverSocket_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        closeSocket(serverSocket_);
        return false;
    }

    if (listen(serverSocket_, 10) < 0) {
        closeSocket(serverSocket_);
        return false;
    }

    return true;
}

void WebSocketServer::acceptConnections() {
    while (running_) {
        sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket_, (sockaddr*)&clientAddr, &addrLen);
        if (clientSocket < 0) continue;

        std::thread clientThread(&WebSocketServer::handleClient, this, clientSocket);
        clientThread.detach();
    }
}

void WebSocketServer::handleClient(int clientSocket) {
    char buffer[4096];
    int bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
        closeSocket(clientSocket);
        return;
    }
    buffer[bytes] = '\0';
    std::string request(buffer);

    if (request.find("Upgrade: websocket") != std::string::npos ||
        request.find("upgrade: websocket") != std::string::npos) {
        if (performWebSocketHandshake(clientSocket, request)) {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            clients_.push_back({clientSocket, true, ""});
            
            // Send initial game state
            if (gameState_) {
                sendTo(clientSocket, "{\"type\":\"state\",\"state\":" + gameState_->toJson() + "}");
            }
            
            // WebSocket read loop
            while (running_) {
                char frameBuf[4096];
                int frameBytes = recv(clientSocket, frameBuf, sizeof(frameBuf), 0);
                if (frameBytes <= 0) break;
                bool fin; int opcode;
                std::string msg = decodeWebSocketFrame(std::string(frameBuf, frameBytes), fin, opcode);
                if (opcode == 0x8) break; // close frame
                if (opcode == 0x1 && !msg.empty()) {
                    processMessage(clientSocket, msg);
                }
            }
            
            {
                std::lock_guard<std::mutex> lock(clientsMutex_);
                for (auto it = clients_.begin(); it != clients_.end(); ++it) {
                    if (it->socket == clientSocket) { clients_.erase(it); break; }
                }
            }
        }
    } else {
        handleHttpRequest(clientSocket, request);
    }
    closeSocket(clientSocket);
}

bool WebSocketServer::handleHttpRequest(int clientSocket, const std::string& request) {
    std::string path = "/";
    size_t pathStart = request.find("GET ");
    if (pathStart != std::string::npos) {
        pathStart += 4;
        size_t pathEnd = request.find(" ", pathStart);
        if (pathEnd != std::string::npos) {
            path = request.substr(pathStart, pathEnd - pathStart);
        }
    }
    if (path.find("?") != std::string::npos) {
        path = path.substr(0, path.find("?"));
    }
    if (path == "/") path = "/index.html";

    std::string filePath = "frontend" + path;
    return serveStaticFile(clientSocket, filePath);
}

bool WebSocketServer::serveStaticFile(int clientSocket, const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(clientSocket, response.c_str(), (int)response.size(), 0);
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string content(size, '\0');
    file.read(&content[0], size);

    std::string contentType = getContentType(path);
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + std::to_string(size) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += content;

    send(clientSocket, response.c_str(), (int)response.size(), 0);
    return true;
}

std::string WebSocketServer::getContentType(const std::string& path) {
    if (path.find(".html") != std::string::npos) return "text/html";
    if (path.find(".css") != std::string::npos) return "text/css";
    if (path.find(".js") != std::string::npos) return "application/javascript";
    if (path.find(".png") != std::string::npos) return "image/png";
    if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos) return "image/jpeg";
    return "text/plain";
}

bool WebSocketServer::performWebSocketHandshake(int clientSocket, const std::string& request) {
    size_t keyPos = request.find("Sec-WebSocket-Key: ");
    if (keyPos == std::string::npos) keyPos = request.find("sec-websocket-key: ");
    if (keyPos == std::string::npos) return false;

    keyPos += 19;
    size_t keyEnd = request.find("\r\n", keyPos);
    std::string key = request.substr(keyPos, keyEnd - keyPos);
    std::string accept = computeWebSocketAccept(key);

    std::string response = "HTTP/1.1 101 Switching Protocols\r\n";
    response += "Upgrade: websocket\r\n";
    response += "Connection: Upgrade\r\n";
    response += "Sec-WebSocket-Accept: " + accept + "\r\n";
    response += "\r\n";

    send(clientSocket, response.c_str(), (int)response.size(), 0);
    return true;
}

std::string WebSocketServer::computeWebSocketAccept(const std::string& key) {
    std::string combined = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    SHA1 sha1;
    sha1.update((const unsigned char*)combined.c_str(), combined.length());
    unsigned char digest[20];
    sha1.final(digest);
    return base64Encode(digest, 20);
}

std::string WebSocketServer::decodeWebSocketFrame(const std::string& data, bool& fin, int& opcode) {
    if (data.size() < 2) { fin = false; opcode = -1; return ""; }
    fin = (data[0] & 0x80) != 0;
    opcode = data[0] & 0x0F;
    bool masked = (data[1] & 0x80) != 0;
    unsigned long long payloadLen = data[1] & 0x7F;
    size_t pos = 2;
    if (payloadLen == 126) {
        if (data.size() < 4) { fin = false; return ""; }
        payloadLen = ((unsigned char)data[2] << 8) | (unsigned char)data[3];
        pos = 4;
    } else if (payloadLen == 127) {
        if (data.size() < 10) { fin = false; return ""; }
        payloadLen = 0;
        for (int i = 0; i < 8; i++) payloadLen = (payloadLen << 8) | (unsigned char)data[2+i];
        pos = 10;
    }
    unsigned char mask[4] = {0, 0, 0, 0};
    if (masked) {
        if (data.size() < pos + 4) { fin = false; return ""; }
        for (int i = 0; i < 4; i++) mask[i] = data[pos + i];
        pos += 4;
    }
    if (data.size() < pos + payloadLen) { fin = false; return ""; }
    std::string result;
    result.reserve(payloadLen);
    for (unsigned long long i = 0; i < payloadLen; i++) {
        unsigned char c = data[pos + i];
        if (masked) c ^= mask[i % 4];
        result += c;
    }
    return result;
}

std::string WebSocketServer::encodeWebSocketFrame(const std::string& data, int opcode) {
    std::string frame;
    frame += (char)(0x80 | (opcode & 0x0F)); // FIN + opcode
    size_t len = data.size();
    if (len < 126) {
        frame += (char)len;
    } else if (len < 65536) {
        frame += (char)126;
        frame += (char)((len >> 8) & 0xFF);
        frame += (char)(len & 0xFF);
    } else {
        frame += (char)127;
        for (int i = 7; i >= 0; i--) frame += (char)((len >> (i * 8)) & 0xFF);
    }
    frame += data;
    return frame;
}

void WebSocketServer::processMessage(int clientSocket, const std::string& message) {
    if (!gameState_) return;
    
    // Parse simple JSON-like commands
    // Expected: {"type":"move","pieceId":1,"to":{"row":5,"col":3}}
    
    if (message.find("\"type\":\"get_state\"") != std::string::npos ||
        message.find("'type':'get_state'") != std::string::npos) {
        sendTo(clientSocket, "{\"type\":\"state\",\"data\":" + gameState_->toJson() + "}");
        return;
    }
    
    if (message.find("\"type\":\"move\"") != std::string::npos) {
        int pieceId = -1;
        int row = -1, col = -1;
        
        size_t pidPos = message.find("\"pieceId\":");
        if (pidPos == std::string::npos) pidPos = message.find("'pieceId':");
        if (pidPos != std::string::npos) pieceId = std::atoi(message.c_str() + pidPos + 10);
        
        size_t rowPos = message.find("\"row\":");
        if (rowPos == std::string::npos) rowPos = message.find("'row':");
        if (rowPos != std::string::npos) row = std::atoi(message.c_str() + rowPos + 6);
        
        size_t colPos = message.find("\"col\":");
        if (colPos == std::string::npos) colPos = message.find("'col':");
        if (colPos != std::string::npos) col = std::atoi(message.c_str() + colPos + 6);
        
        if (pieceId >= 0 && row >= 0 && col >= 0) {
            bool ok = gameState_->makeMove(pieceId, Position{row, col});
            std::string resp = "{\"type\":\"move_result\",\"success\":" + std::string(ok ? "true" : "false") +
                              ",\"state\":" + gameState_->toJson() + "}";
            broadcast(resp);
        }
        return;
    }
    
    if (message.find("\"type\":\"legal_moves\"") != std::string::npos) {
        int pieceId = -1;
        size_t pidPos = message.find("\"pieceId\":");
        if (pidPos == std::string::npos) pidPos = message.find("'pieceId':");
        if (pidPos != std::string::npos) pieceId = std::atoi(message.c_str() + pidPos + 10);
        
        if (pieceId >= 0) {
            auto moves = gameState_->getLegalMoves(pieceId);
            sendTo(clientSocket, "{\"type\":\"legal_moves\",\"data\":" +
                   JsonUtils::serializeLegalMoves(pieceId, moves) + "}");
        }
        return;
    }
    
    if (message.find("\"type\":\"transmute\"") != std::string::npos) {
        int pieceId = -1;
        std::string element;
        int mass = 0;
        
        size_t pidPos = message.find("\"pieceId\":");
        if (pidPos == std::string::npos) pidPos = message.find("'pieceId':");
        if (pidPos != std::string::npos) pieceId = std::atoi(message.c_str() + pidPos + 10);
        
        size_t elemPos = message.find("\"element\":\"");
        if (elemPos == std::string::npos) elemPos = message.find("'element':'");
        if (elemPos != std::string::npos) {
            elemPos += 11;
            size_t elemEnd = message.find("\"", elemPos);
            if (elemEnd == std::string::npos) elemEnd = message.find("'", elemPos);
            element = message.substr(elemPos, elemEnd - elemPos);
        }
        
        size_t massPos = message.find("\"massNumber\":");
        if (massPos == std::string::npos) massPos = message.find("'massNumber':");
        if (massPos != std::string::npos) mass = std::atoi(message.c_str() + massPos + 13);
        
        if (pieceId >= 0 && !element.empty()) {
            bool ok = gameState_->transmute(pieceId, element, mass);
            std::string resp = "{\"type\":\"transmute_result\",\"success\":" + std::string(ok ? "true" : "false") +
                              ",\"state\":" + gameState_->toJson() + "}";
            broadcast(resp);
        }
        return;
    }
    
    if (message.find("\"type\":\"donate\"") != std::string::npos) {
        int donorId = -1, recipientId = -1;
        size_t dPos = message.find("\"donorId\":");
        if (dPos == std::string::npos) dPos = message.find("'donorId':");
        if (dPos != std::string::npos) donorId = std::atoi(message.c_str() + dPos + 10);
        
        size_t rPos = message.find("\"recipientId\":");
        if (rPos == std::string::npos) rPos = message.find("'recipientId':");
        if (rPos != std::string::npos) recipientId = std::atoi(message.c_str() + rPos + 14);
        
        if (donorId >= 0 && recipientId >= 0) {
            bool ok = gameState_->donateElectron(donorId, recipientId);
            std::string resp = "{\"type\":\"donate_result\",\"success\":" + std::string(ok ? "true" : "false") +
                              ",\"state\":" + gameState_->toJson() + "}";
            broadcast(resp);
        }
        return;
    }
    
    if (message.find("\"type\":\"draw\"") != std::string::npos) {
        gameState_->requestDraw();
        broadcast("{\"type\":\"draw_requested\",\"state\":" + gameState_->toJson() + "}");
        return;
    }
    
    if (message.find("\"type\":\"accept_draw\"") != std::string::npos) {
        gameState_->acceptDraw();
        broadcast("{\"type\":\"draw_accepted\",\"state\":" + gameState_->toJson() + "}");
        return;
    }
    
    if (message.find("\"type\":\"new_game\"") != std::string::npos) {
        gameState_->startNewGame();
        broadcast("{\"type\":\"new_game\",\"state\":" + gameState_->toJson() + "}");
        return;
    }
}

void WebSocketServer::closeSocket(int sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

} // namespace chemiss
