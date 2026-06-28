#include "board.h"
#include <sstream>

namespace chemiss {

Board::Board() {}

void Board::initialize() {
    reset();
    Piece::resetIdCounter();
    
    // White pieces (bottom rows 6,7)
    // Row 7: O P Si F H C N S
    addPiece(std::make_shared<Piece>("O", Player::White, Position{7,0}));
    addPiece(std::make_shared<Piece>("P", Player::White, Position{7,1}));
    addPiece(std::make_shared<Piece>("Si", Player::White, Position{7,2}));
    addPiece(std::make_shared<Piece>("F", Player::White, Position{7,3}));
    addPiece(std::make_shared<Piece>("H", Player::White, Position{7,4}));
    addPiece(std::make_shared<Piece>("C", Player::White, Position{7,5}));
    addPiece(std::make_shared<Piece>("N", Player::White, Position{7,6}));
    addPiece(std::make_shared<Piece>("S", Player::White, Position{7,7}));
    // Row 6: Na Na Li Li Li Li Na Na
    addPiece(std::make_shared<Piece>("Na", Player::White, Position{6,0}));
    addPiece(std::make_shared<Piece>("Na", Player::White, Position{6,1}));
    addPiece(std::make_shared<Piece>("Li", Player::White, Position{6,2}));
    addPiece(std::make_shared<Piece>("Li", Player::White, Position{6,3}));
    addPiece(std::make_shared<Piece>("Li", Player::White, Position{6,4}));
    addPiece(std::make_shared<Piece>("Li", Player::White, Position{6,5}));
    addPiece(std::make_shared<Piece>("Na", Player::White, Position{6,6}));
    addPiece(std::make_shared<Piece>("Na", Player::White, Position{6,7}));
    
    // Black pieces (top rows 0,1)
    // Row 0: O P Si F H C N S
    addPiece(std::make_shared<Piece>("O", Player::Black, Position{0,0}));
    addPiece(std::make_shared<Piece>("P", Player::Black, Position{0,1}));
    addPiece(std::make_shared<Piece>("Si", Player::Black, Position{0,2}));
    addPiece(std::make_shared<Piece>("F", Player::Black, Position{0,3}));
    addPiece(std::make_shared<Piece>("H", Player::Black, Position{0,4}));
    addPiece(std::make_shared<Piece>("C", Player::Black, Position{0,5}));
    addPiece(std::make_shared<Piece>("N", Player::Black, Position{0,6}));
    addPiece(std::make_shared<Piece>("S", Player::Black, Position{0,7}));
    // Row 1: Na Na Li Li Li Li Na Na
    addPiece(std::make_shared<Piece>("Na", Player::Black, Position{1,0}));
    addPiece(std::make_shared<Piece>("Na", Player::Black, Position{1,1}));
    addPiece(std::make_shared<Piece>("Li", Player::Black, Position{1,2}));
    addPiece(std::make_shared<Piece>("Li", Player::Black, Position{1,3}));
    addPiece(std::make_shared<Piece>("Li", Player::Black, Position{1,4}));
    addPiece(std::make_shared<Piece>("Li", Player::Black, Position{1,5}));
    addPiece(std::make_shared<Piece>("Na", Player::Black, Position{1,6}));
    addPiece(std::make_shared<Piece>("Na", Player::Black, Position{1,7}));
    
    updatePositionMap();
}

void Board::reset() {
    pieces_.clear();
    positionMap_.clear();
}

Piece* Board::getPieceAt(Position pos) const {
    auto it = positionMap_.find({pos.row, pos.col});
    if (it != positionMap_.end()) {
        return getPieceById(it->second);
    }
    return nullptr;
}

Piece* Board::getPieceById(int id) const {
    for (const auto& p : pieces_) {
        if (p->getId() == id) return p.get();
    }
    return nullptr;
}

std::vector<Piece*> Board::getAllPieces() const {
    std::vector<Piece*> result;
    for (const auto& p : pieces_) {
        if (p->isAlive()) result.push_back(p.get());
    }
    return result;
}

std::vector<Piece*> Board::getPlayerPieces(Player player) const {
    std::vector<Piece*> result;
    for (const auto& p : pieces_) {
        if (p->isAlive() && p->getOwner() == player) result.push_back(p.get());
    }
    return result;
}

std::vector<Piece*> Board::getPiecesInRadius(Position center, int radius) const {
    std::vector<Piece*> result;
    for (const auto& p : pieces_) {
        if (!p->isAlive()) continue;
        int dr = std::abs(p->getPosition().row - center.row);
        int dc = std::abs(p->getPosition().col - center.col);
        if (dr <= radius && dc <= radius) result.push_back(p.get());
    }
    return result;
}

bool Board::movePiece(int pieceId, Position to) {
    Piece* p = getPieceById(pieceId);
    if (!p || !p->isAlive()) return false;
    Position from = p->getPosition();
    positionMap_.erase({from.row, from.col});
    p->setPosition(to);
    positionMap_[{to.row, to.col}] = pieceId;
    return true;
}

bool Board::capturePiece(int pieceId) {
    Piece* p = getPieceById(pieceId);
    if (!p || !p->isAlive()) return false;
    Position pos = p->getPosition();
    p->setDead();
    p->clearBonds();
    positionMap_.erase({pos.row, pos.col});
    return true;
}

bool Board::promotePiece(int pieceId, const std::string& newSymbol, int massNumber) {
    Piece* p = getPieceById(pieceId);
    if (!p) return false;
    p->setElement(newSymbol, massNumber);
    p->clearBonds();
    return true;
}

void Board::addPiece(std::shared_ptr<Piece> piece) {
    pieces_.push_back(piece);
    positionMap_[{piece->getPosition().row, piece->getPosition().col}] = piece->getId();
}

bool Board::isPositionEmpty(Position pos) const {
    return positionMap_.find({pos.row, pos.col}) == positionMap_.end();
}

bool Board::isPositionOccupiedBy(Position pos, Player player) const {
    Piece* p = getPieceAt(pos);
    return p && p->isAlive() && p->getOwner() == player;
}

bool Board::isPositionOccupiedByEnemy(Position pos, Player player) const {
    Piece* p = getPieceAt(pos);
    return p && p->isAlive() && p->getOwner() != player;
}

Piece* Board::getKing(Player player) const {
    for (const auto& p : pieces_) {
        if (p->isAlive() && p->getOwner() == player && p->getSymbol() == "H") {
            return p.get();
        }
    }
    return nullptr;
}

std::string Board::toJson() const {
    std::stringstream ss;
    ss << "{\"pieces\":[";
    bool first = true;
    for (const auto& p : pieces_) {
        if (!p->isAlive()) continue;
        if (!first) ss << ",";
        first = false;
        ss << p->toJson();
    }
    ss << "]}";
    return ss.str();
}

void Board::updatePositionMap() {
    positionMap_.clear();
    for (const auto& p : pieces_) {
        if (p->isAlive()) {
            positionMap_[{p->getPosition().row, p->getPosition().col}] = p->getId();
        }
    }
}

} // namespace chemiss
