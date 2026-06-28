#ifndef CHEMISS_RULES_H
#define CHEMISS_RULES_H

#include "board.h"
#include <vector>
#include <functional>

namespace chemiss {

struct Move {
    int pieceId;
    Position from;
    Position to;
    bool isCapture;
    int capturePieceId;
    bool isTransmutation;
    std::string newElement;
    int newMassNumber;
    bool isElectronDonation;
    int donorPieceId;
    int recipientPieceId;
    
    Move() : pieceId(-1), isCapture(false), capturePieceId(-1),
             isTransmutation(false), newMassNumber(0),
             isElectronDonation(false), donorPieceId(-1), recipientPieceId(-1) {}
};

struct RayEffect {
    enum Type { Alpha, Beta, Gamma };
    Type type;
    Position from;
    Direction dir;
    int range;
    int hitPieceId;
};

class RulesEngine {
public:
    RulesEngine(Board* board);
    
    // Movement validation
    std::vector<Position> getValidMoves(int pieceId) const;
    bool isValidMove(int pieceId, Position to) const;
    
    // Capture validation
    bool canCapture(int attackerId, int targetId) const;
    bool isIonicCapture(int attackerId, int targetId) const;
    bool isCovalentOrMetallicCapture(int attackerId, int targetId) const;
    
    // Bonding
    void updateBonds(); // Call after each move to auto-form bonds
    void breakBonds(int pieceId); // Break bonds for a piece
    void calculateSharedElectrons();
    
    // Special abilities
    bool canTransmute(int pieceId) const;
    std::vector<std::string> getTransmutationOptions(int pieceId) const;
    bool canDonateElectron(int donorId, int recipientId) const;
    
    // Electrstatic attraction (forced at start of turn)
    struct AttractionPair {
        int pieceA;
        int pieceB;
        int chargeDiff;
        Position newPosA;
        Position newPosB;
    };
    std::vector<AttractionPair> findElectrostaticAttractions(Player currentPlayer) const;
    
    // Radioactivity
    std::vector<RayEffect> processRadioactiveDecay();
    bool shouldDecay(const Piece* piece) const;
    RayEffect emitRay(int pieceId, DecayMode mode) const;
    bool applyRayEffect(const RayEffect& ray);
    bool triggerFission(int pieceId);
    std::vector<std::shared_ptr<Piece>> getFissionProducts(int pieceId) const;
    
    // Overflow beta ray (when total electrons > 8)
    RayEffect emitBetaRayFromOverflow(int pieceId) const;
    
    // Game end conditions
    bool isKingCaptured(Player player) const;
    bool canPlayerMove(Player player) const;
    
    // Execute move
    bool executeMove(const Move& move);
    bool executeTransmutation(int pieceId, const std::string& element, int massNumber);
    bool executeElectronDonation(int donorId, int recipientId);
    
    // Turn processing
    void processTurnStart(Player currentPlayer);
    void processTurnEnd();
    
    // Noble gas check
    bool isNobleGas(const Piece* piece) const;
    
private:
    Board* board_;
    
    // Movement helpers
    std::vector<Direction> getPieceDirections(int pieceId) const;
    bool isPathClear(Position from, Position to, Direction dir, int steps) const;
    
    // Bonding helpers
    int getCovalentSharedElectrons(const Piece* a, const Piece* b) const;
    int getMetallicSharedElectrons(const Piece* piece) const;
    bool isAdjacent(Position a, Position b) const;
    
    // Ray helpers
    Position traceRay(Position from, Direction dir, int maxRange, int& hitPieceId) const;
    bool attemptAlphaTransmutation(int pieceId);
    bool attemptBetaTransmutation(int pieceId);
    bool attemptGammaEffect(int pieceId);
};

} // namespace chemiss

#endif // CHEMISS_RULES_H
