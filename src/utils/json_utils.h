#ifndef CHEMISS_JSON_UTILS_H
#define CHEMISS_JSON_UTILS_H

#include "../game/game_state.h"
#include <string>
#include <vector>

namespace chemiss {

class JsonUtils {
public:
    static std::string serializeGameState(const GameState& state);
    static std::string serializeLegalMoves(int pieceId, const std::vector<Position>& moves);
    static std::string serializeElementList(const std::vector<std::string>& elements);
    static std::string serializeIsotopeList(const std::string& element, const std::vector<int>& isotopes);
    static std::string escapeJson(const std::string& s);
};

} // namespace chemiss

#endif // CHEMISS_JSON_UTILS_H
