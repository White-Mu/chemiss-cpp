#ifndef CHEMISS_PIECE_H
#define CHEMISS_PIECE_H

#include "element.h"
#include <string>
#include <vector>

namespace chemiss {

enum class Player {
    White = 0,
    Black = 1
};

enum class BondType {
    None,
    Ionic,      // ionic bond (not actually stored between pieces, immediate)
    Covalent,   // covalent bond
    Metallic    // metallic bond
};

enum class PieceStatus {
    Normal,
    Stunned,    // cannot move next turn
    Radioactive // emits radiation
};

struct Position {
    int row;
    int col;
    bool operator==(const Position& other) const {
        return row == other.row && col == other.col;
    }
    bool isValid() const {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }
};

struct Direction {
    int dr; // row delta
    int dc; // col delta
};

class Piece {
public:
    Piece();
    Piece(const std::string& elementSymbol, Player owner, Position pos, int massNumber = 0);
    
    // Getters
    const std::string& getSymbol() const { return elementSymbol_; }
    const Element* getElement() const { return element_; }
    Player getOwner() const { return owner_; }
    Position getPosition() const { return position_; }
    int getMassNumber() const { return massNumber_; }
    int getValenceElectrons() const { return valenceElectrons_; }
    int getTotalElectrons() const { return valenceElectrons_ + sharedElectrons_ + extraElectrons_; }
    double getElectronegativity() const { return electronegativity_; }
    int getPeriod() const { return period_; }
    ElementType getType() const { return type_; }
    PieceStatus getStatus() const { return status_; }
    int getCharge() const { return charge_; }
    int getSharedElectrons() const { return sharedElectrons_; }
    int getExtraElectrons() const { return extraElectrons_; }
    int getTurnsRemaining() const { return turnsRemaining_; } // for charge/extra electron duration
    int getDecayCounter() const { return decayCounter_; }
    bool isRadioactive() const { return isRadioactive_; }
    DecayMode getDecayMode() const { return decayMode_; }
    int getHalfLife() const { return halfLife_; }
    bool isBonded() const { return !bondedTo_.empty(); }
    const std::vector<int>& getBondedPieces() const { return bondedTo_; }
    BondType getBondType() const { return bondType_; }
    bool isAlive() const { return alive_; }
    
    // Setters
    void setPosition(Position pos) { position_ = pos; }
    void setValenceElectrons(int v) { valenceElectrons_ = v; }
    void setExtraElectrons(int e, int duration);
    void setCharge(int c, int duration);
    void setSharedElectrons(int s) { sharedElectrons_ = s; }
    void addBond(int pieceId);
    void removeBond(int pieceId);
    void clearBonds();
    void setBondType(BondType bt) { bondType_ = bt; }
    void setStatus(PieceStatus s) { status_ = s; }
    void stun(int turns);
    void setRadioactive(bool r, DecayMode mode, int halfLife);
    void incrementDecayCounter() { decayCounter_++; }
    void resetDecayCounter() { decayCounter_ = 0; }
    void setElement(const std::string& symbol, int massNumber = 0);
    void setDead() { alive_ = false; }
    void tickEffects(); // reduce duration counters
    
    // Movement
    std::vector<Direction> getMovementDirections() const;
    int getMaxMovementSteps() const { return getTotalElectrons(); }
    
    // Serialization
    std::string toJson() const;
    
    int getId() const { return id_; }
    static void resetIdCounter() { nextId_ = 0; }
    
private:
    int id_;
    static int nextId_;
    
    std::string elementSymbol_;
    const Element* element_;
    Player owner_;
    Position position_;
    int massNumber_;
    
    // Dynamic state
    int valenceElectrons_;
    double electronegativity_;
    int period_;
    ElementType type_;
    int sharedElectrons_;
    int extraElectrons_;
    int charge_;
    int turnsRemaining_; // for charge and extra electrons
    int extraTurnsRemaining_;
    PieceStatus status_;
    int stunTurns_;
    
    // Radioactivity
    bool isRadioactive_;
    DecayMode decayMode_;
    int halfLife_;
    int decayCounter_;
    
    // Bonding
    std::vector<int> bondedTo_;
    BondType bondType_;
    
    bool alive_;
};

} // namespace chemiss

#endif // CHEMISS_PIECE_H
