#include "json_utils.h"
#include <sstream>

namespace chemiss {

std::string JsonUtils::serializeGameState(const GameState& state) {
    std::stringstream ss;
    ss << "{";
    ss << "\"phase\":\"" << (state.getPhase() == GamePhase::Setup ? "setup" : 
                              (state.getPhase() == GamePhase::Playing ? "playing" : "gameover")) << "\",";
    ss << "\"result\":\"" << state.getResultString() << "\",";
    ss << "\"currentPlayer\":" << (state.getCurrentPlayer() == Player::White ? 0 : 1) << ",";
    ss << "\"turnNumber\":" << state.getTurnNumber() << ",";
    ss << "\"board\":" << state.getBoard()->toJson() << ",";
    ss << "\"events\":[";
    const auto& events = state.getEvents();
    for (size_t i = 0; i < events.size(); i++) {
        if (i > 0) ss << ",";
        ss << "\"" << escapeJson(events[i]) << "\"";
    }
    ss << "]";
    ss << "}";
    return ss.str();
}

std::string JsonUtils::serializeLegalMoves(int pieceId, const std::vector<Position>& moves) {
    std::stringstream ss;
    ss << "{\"pieceId\":" << pieceId << ",\"moves\":[";
    for (size_t i = 0; i < moves.size(); i++) {
        if (i > 0) ss << ",";
        ss << "{\"row\":" << moves[i].row << ",\"col\":" << moves[i].col << "}";
    }
    ss << "]}";
    return ss.str();
}

std::string JsonUtils::serializeElementList(const std::vector<std::string>& elements) {
    std::stringstream ss;
    ss << "{\"elements\":[";
    for (size_t i = 0; i < elements.size(); i++) {
        if (i > 0) ss << ",";
        ss << "\"" << elements[i] << "\"";
    }
    ss << "]}";
    return ss.str();
}

std::string JsonUtils::serializeIsotopeList(const std::string& element, const std::vector<int>& isotopes) {
    std::stringstream ss;
    ss << "{\"element\":\"" << element << "\",\"isotopes\":[";
    for (size_t i = 0; i < isotopes.size(); i++) {
        if (i > 0) ss << ",";
        ss << isotopes[i];
    }
    ss << "]}";
    return ss.str();
}

std::string JsonUtils::escapeJson(const std::string& s) {
    std::string result;
    for (char c : s) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

} // namespace chemiss
