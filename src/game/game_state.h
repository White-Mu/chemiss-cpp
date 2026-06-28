#ifndef CHEMISS_GAME_STATE_H
#define CHEMISS_GAME_STATE_H

#include "board.h"
#include "rules.h"
#include <string>
#include <vector>

namespace chemiss {

enum class GamePhase {
    Setup,
    Playing,
    GameOver
};

enum class GameResult {
    Ongoing,
    WhiteWin,
    BlackWin,
    Draw
};

class GameState {
public:
    GameState();
    
    void startNewGame();
    bool makeMove(int pieceId, Position to);
    bool capture(int attackerId, Position targetPos);
    bool transmute(int pieceId, const std::string& element, int massNumber);
    bool donateElectron(int donorId, int recipientId);
    bool requestDraw();
    bool acceptDraw();
    
    // Turn management
    Player getCurrentPlayer() const { return currentPlayer_; }
    int getTurnNumber() const { return turnNumber_; }
    void nextTurn();
    
    // State queries
    GamePhase getPhase() const { return phase_; }
    GameResult getResult() const { return result_; }
    std::string getResultString() const;
    Board* getBoard() { return &board_; }
    const Board* getBoard() const { return &board_; }
    RulesEngine* getRules() { return &rules_; }
    
    // Legal moves for UI
    std::vector<Position> getLegalMoves(int pieceId) const;
    bool isPieceMovable(int pieceId) const;
    
    // Serialization
    std::string toJson() const;
    std::string getGameStateJsonForPlayer(Player player) const;
    
    // Events log
    void addEvent(const std::string& event);
    std::vector<std::string> getEvents() const { return eventLog_; }
    
private:
    Board board_;
    RulesEngine rules_;
    Player currentPlayer_;
    int turnNumber_;
    GamePhase phase_;
    GameResult result_;
    std::vector<std::string> eventLog_;
    bool drawRequested_;
    Player drawRequestedBy_;
    
    void checkGameEnd();
    void processAutomaticEffects();
};

} // namespace chemiss

#endif // CHEMISS_GAME_STATE_H
