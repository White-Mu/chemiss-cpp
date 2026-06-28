#ifndef CHEMISS_BOARD_H
#define CHEMISS_BOARD_H

#include "piece.h"
#include <vector>
#include <memory>
#include <utility>
#include <map>

namespace chemiss {

class Board {
public:
    Board();
    
    void initialize(); // Set up initial position
    void reset();
    
    // Piece access
    Piece* getPieceAt(Position pos) const;
    Piece* getPieceById(int id) const;
    std::vector<Piece*> getAllPieces() const;
    std::vector<Piece*> getPlayerPieces(Player player) const;
    std::vector<Piece*> getPiecesInRadius(Position center, int radius) const;
    
    // Board operations
    bool movePiece(int pieceId, Position to);
    bool capturePiece(int pieceId); // remove piece
    bool promotePiece(int pieceId, const std::string& newSymbol, int massNumber);
    void addPiece(std::shared_ptr<Piece> piece);
    
    // Check helpers
    bool isPositionEmpty(Position pos) const;
    bool isPositionOccupiedBy(Position pos, Player player) const;
    bool isPositionOccupiedByEnemy(Position pos, Player player) const;
    
    // King check
    Piece* getKing(Player player) const;
    
    // Serialization
    std::string toJson() const;
    
private:
    std::vector<std::shared_ptr<Piece>> pieces_;
    std::map<std::pair<int,int>, int> positionMap_; // (row,col) -> pieceId
    
    void updatePositionMap();
};

} // namespace chemiss

#endif // CHEMISS_BOARD_H
