#include "game_state.h"
#include "../utils/json_utils.h"
#include <sstream>

namespace chemiss {

GameState::GameState()
    : board_(), rules_(&board_),
      currentPlayer_(Player::White), turnNumber_(1),
      phase_(GamePhase::Setup), result_(GameResult::Ongoing),
      drawRequested_(false), drawRequestedBy_(Player::White) {}

void GameState::startNewGame() {
    board_.initialize();
    rules_.updateBonds();
    currentPlayer_ = Player::White;
    turnNumber_ = 1;
    phase_ = GamePhase::Playing;
    result_ = GameResult::Ongoing;
    eventLog_.clear();
    drawRequested_ = false;
    addEvent("Game started. White to move.");
}

bool GameState::makeMove(int pieceId, Position to) {
    if (phase_ != GamePhase::Playing) return false;
    
    Piece* piece = board_.getPieceById(pieceId);
    if (!piece || !piece->isAlive()) return false;
    if (piece->getOwner() != currentPlayer_) return false;
    
    auto legalMoves = rules_.getValidMoves(pieceId);
    bool isLegal = false;
    for (const auto& pos : legalMoves) {
        if (pos == to) { isLegal = true; break; }
    }
    if (!isLegal) return false;
    
    // Process automatic effects at turn start
    rules_.processTurnStart(currentPlayer_);
    
    Move move;
    move.pieceId = pieceId;
    move.from = piece->getPosition();
    move.to = to;
    
    Piece* target = board_.getPieceAt(to);
    if (target && target->isAlive()) {
        move.isCapture = true;
        move.capturePieceId = target->getId();
    }
    
    bool success = rules_.executeMove(move);
    if (!success) return false;
    
    std::stringstream ss;
    ss << (currentPlayer_ == Player::White ? "White" : "Black");
    ss << " " << piece->getSymbol() << " moved from (" << move.from.row << "," << move.from.col;
    ss << ") to (" << to.row << "," << to.col << ")";
    if (move.isCapture) {
        ss << " capturing " << target->getSymbol();
    }
    addEvent(ss.str());
    
    // Process turn end effects (decay, etc.)
    rules_.processTurnEnd();
    
    checkGameEnd();
    if (phase_ == GamePhase::Playing) {
        nextTurn();
    }
    return true;
}

bool GameState::capture(int attackerId, Position targetPos) {
    Piece* attacker = board_.getPieceById(attackerId);
    if (!attacker || !attacker->isAlive()) return false;
    if (attacker->getOwner() != currentPlayer_) return false;
    
    Piece* target = board_.getPieceAt(targetPos);
    if (!target || !target->isAlive()) return false;
    if (target->getOwner() == currentPlayer_) return false;
    
    if (!rules_.canCapture(attackerId, target->getId())) return false;
    
    rules_.processTurnStart(currentPlayer_);
    
    Move move;
    move.pieceId = attackerId;
    move.from = attacker->getPosition();
    move.to = targetPos;
    move.isCapture = true;
    move.capturePieceId = target->getId();
    
    bool success = rules_.executeMove(move);
    if (!success) return false;
    
    std::stringstream ss;
    ss << (currentPlayer_ == Player::White ? "White" : "Black");
    ss << " " << attacker->getSymbol() << " captured " << target->getSymbol();
    ss << " at (" << targetPos.row << "," << targetPos.col << ")";
    addEvent(ss.str());
    
    rules_.processTurnEnd();
    checkGameEnd();
    if (phase_ == GamePhase::Playing) {
        nextTurn();
    }
    return true;
}

bool GameState::transmute(int pieceId, const std::string& element, int massNumber) {
    if (phase_ != GamePhase::Playing) return false;
    if (!rules_.canTransmute(pieceId)) return false;
    
    Piece* piece = board_.getPieceById(pieceId);
    if (!piece || piece->getOwner() != currentPlayer_) return false;
    
    rules_.processTurnStart(currentPlayer_);
    
    std::string oldSymbol = piece->getSymbol();
    bool success = rules_.executeTransmutation(pieceId, element, massNumber);
    if (!success) return false;
    
    std::stringstream ss;
    ss << (currentPlayer_ == Player::White ? "White" : "Black");
    ss << " " << oldSymbol << " transmuted into " << element;
    if (massNumber > 0) ss << "-" << massNumber;
    addEvent(ss.str());
    
    rules_.processTurnEnd();
    checkGameEnd();
    if (phase_ == GamePhase::Playing) {
        nextTurn();
    }
    return true;
}

bool GameState::donateElectron(int donorId, int recipientId) {
    if (phase_ != GamePhase::Playing) return false;
    
    Piece* donor = board_.getPieceById(donorId);
    if (!donor || donor->getOwner() != currentPlayer_) return false;
    
    rules_.processTurnStart(currentPlayer_);
    
    bool success = rules_.executeElectronDonation(donorId, recipientId);
    if (!success) return false;
    
    Piece* recipient = board_.getPieceById(recipientId);
    std::stringstream ss;
    ss << (currentPlayer_ == Player::White ? "White" : "Black");
    ss << " " << donor->getSymbol() << " donated electron to ";
    if (recipient) ss << recipient->getSymbol();
    addEvent(ss.str());
    
    rules_.processTurnEnd();
    checkGameEnd();
    if (phase_ == GamePhase::Playing) {
        nextTurn();
    }
    return true;
}

bool GameState::requestDraw() {
    if (phase_ != GamePhase::Playing) return false;
    if (drawRequested_) return false;
    drawRequested_ = true;
    drawRequestedBy_ = currentPlayer_;
    addEvent(std::string(currentPlayer_ == Player::White ? "White" : "Black") + " requested a draw.");
    return true;
}

bool GameState::acceptDraw() {
    if (!drawRequested_) return false;
    if (drawRequestedBy_ == currentPlayer_) return false;
    result_ = GameResult::Draw;
    phase_ = GamePhase::GameOver;
    addEvent("Draw accepted. Game over.");
    return true;
}

void GameState::nextTurn() {
    currentPlayer_ = (currentPlayer_ == Player::White) ? Player::Black : Player::White;
    if (currentPlayer_ == Player::White) {
        turnNumber_++;
    }
    drawRequested_ = false;
}

std::string GameState::getResultString() const {
    switch (result_) {
        case GameResult::WhiteWin: return "white_win";
        case GameResult::BlackWin: return "black_win";
        case GameResult::Draw: return "draw";
        default: return "ongoing";
    }
}

std::vector<Position> GameState::getLegalMoves(int pieceId) const {
    return rules_.getValidMoves(pieceId);
}

bool GameState::isPieceMovable(int pieceId) const {
    Piece* p = board_.getPieceById(pieceId);
    if (!p || !p->isAlive()) return false;
    if (p->getOwner() != currentPlayer_) return false;
    return !getLegalMoves(pieceId).empty();
}

std::string GameState::toJson() const {
    return JsonUtils::serializeGameState(*this);
}

std::string GameState::getGameStateJsonForPlayer(Player player) const {
    return toJson(); // Same for both in this version
}

void GameState::addEvent(const std::string& event) {
    eventLog_.push_back(event);
    if (eventLog_.size() > 100) {
        eventLog_.erase(eventLog_.begin());
    }
}

void GameState::checkGameEnd() {
    if (rules_.isKingCaptured(Player::White)) {
        result_ = GameResult::BlackWin;
        phase_ = GamePhase::GameOver;
        addEvent("Black wins! White's H (King) was captured.");
        return;
    }
    if (rules_.isKingCaptured(Player::Black)) {
        result_ = GameResult::WhiteWin;
        phase_ = GamePhase::GameOver;
        addEvent("White wins! Black's H (King) was captured.");
        return;
    }
    
    // Check if current player can move
    if (!rules_.canPlayerMove(currentPlayer_)) {
        // This is a stalemate-like situation, but per rules it's just a draw if agreed
        // For now, we don't auto-declare draw
    }
}

} // namespace chemiss
