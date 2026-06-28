#include "piece.h"
#include <sstream>

namespace chemiss {

int Piece::nextId_ = 0;

Piece::Piece() : id_(nextId_++), element_(nullptr), owner_(Player::White),
    position_{-1,-1}, massNumber_(0), valenceElectrons_(0), electronegativity_(0),
    period_(0), type_(ElementType::Nonmetal), sharedElectrons_(0), extraElectrons_(0),
    charge_(0), turnsRemaining_(0), extraTurnsRemaining_(0), status_(PieceStatus::Normal),
    stunTurns_(0), isRadioactive_(false), decayMode_(DecayMode::None), halfLife_(0),
    decayCounter_(0), bondType_(BondType::None), alive_(true) {}

Piece::Piece(const std::string& elementSymbol, Player owner, Position pos, int massNumber)
    : Piece() {
    elementSymbol_ = elementSymbol;
    owner_ = owner;
    position_ = pos;
    massNumber_ = massNumber;
    setElement(elementSymbol, massNumber);
}

void Piece::setElement(const std::string& symbol, int massNumber) {
    elementSymbol_ = symbol;
    element_ = ElementTable::instance().getElement(symbol);
    if (element_) {
        valenceElectrons_ = element_->valenceElectrons;
        electronegativity_ = element_->electronegativity;
        period_ = element_->period;
        type_ = element_->type;
        if (massNumber > 0) {
            massNumber_ = massNumber;
        } else if (!element_->isotopes.empty()) {
            massNumber_ = element_->isotopes[0].massNumber;
        }
        // Check if selected isotope is radioactive
        if (massNumber_ > 0) {
            const Isotope* iso = ElementTable::instance().getIsotope(symbol, massNumber_);
            if (iso && iso->isRadioactive) {
                isRadioactive_ = true;
                decayMode_ = iso->decayMode;
                halfLife_ = iso->halfLife;
            } else {
                isRadioactive_ = false;
                decayMode_ = DecayMode::None;
                halfLife_ = 0;
            }
        }
    }
}

void Piece::setExtraElectrons(int e, int duration) {
    extraElectrons_ = e;
    extraTurnsRemaining_ = duration;
}

void Piece::setCharge(int c, int duration) {
    charge_ = c;
    turnsRemaining_ = duration;
}

void Piece::addBond(int pieceId) {
    for (int id : bondedTo_) {
        if (id == pieceId) return;
    }
    bondedTo_.push_back(pieceId);
}

void Piece::removeBond(int pieceId) {
    for (auto it = bondedTo_.begin(); it != bondedTo_.end(); ++it) {
        if (*it == pieceId) {
            bondedTo_.erase(it);
            return;
        }
    }
}

void Piece::clearBonds() {
    bondedTo_.clear();
    bondType_ = BondType::None;
    sharedElectrons_ = 0;
}

void Piece::stun(int turns) {
    status_ = PieceStatus::Stunned;
    stunTurns_ = turns;
}

void Piece::setRadioactive(bool r, DecayMode mode, int halfLife) {
    isRadioactive_ = r;
    decayMode_ = mode;
    halfLife_ = halfLife;
    decayCounter_ = 0;
    if (r) {
        status_ = PieceStatus::Radioactive;
    } else if (status_ == PieceStatus::Radioactive) {
        status_ = PieceStatus::Normal;
    }
}

void Piece::tickEffects() {
    if (stunTurns_ > 0) {
        stunTurns_--;
        if (stunTurns_ <= 0) {
            status_ = isRadioactive_ ? PieceStatus::Radioactive : PieceStatus::Normal;
        }
    }
    if (turnsRemaining_ > 0) {
        turnsRemaining_--;
        if (turnsRemaining_ <= 0) {
            charge_ = 0;
        }
    }
    if (extraTurnsRemaining_ > 0) {
        extraTurnsRemaining_--;
        if (extraTurnsRemaining_ <= 0) {
            extraElectrons_ = 0;
        }
    }
}

std::vector<Direction> Piece::getMovementDirections() const {
    std::vector<Direction> baseDirs;
    int totalE = getTotalElectrons();
    int numDirs = totalE;
    if (numDirs > 8) numDirs = 8;
    
    // Even periods (2,4,6): prioritize orthogonal
    // Odd periods (1,3,5): prioritize diagonal
    bool evenPeriod = (period_ % 2 == 0);
    
    // All 8 directions: orthogonal first or diagonal first depending on period
    // Direction order: N, NE, E, SE, S, SW, W, NW
    Direction allDirs[8] = {
        {-1,0}, {-1,1}, {0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}
    };
    
    // Reorder based on period parity
    std::vector<int> order;
    if (evenPeriod) {
        order = {0, 2, 4, 6, 1, 3, 5, 7}; // N, E, S, W, NE, SE, SW, NW
    } else {
        order = {1, 3, 5, 7, 0, 2, 4, 6}; // NE, SE, SW, NW, N, E, S, W
    }
    
    for (int i = 0; i < numDirs && i < 8; i++) {
        baseDirs.push_back(allDirs[order[i]]);
    }
    
    // Apply owner faction (white moves up = negative row, black moves down = positive row)
    // For white, forward is row decrease (toward 0), so row delta stays as is
    // For black, forward is row increase (toward 7), so invert row delta
    for (auto& dir : baseDirs) {
        if (owner_ == Player::Black) {
            dir.dr = -dir.dr;
        }
    }
    
    return baseDirs;
}

std::string Piece::toJson() const {
    std::stringstream ss;
    ss << "{";
    ss << "\"id\":" << id_ << ",";
    ss << "\"symbol\":\"" << elementSymbol_ << "\",";
    ss << "\"owner\":" << (owner_ == Player::White ? 0 : 1) << ",";
    ss << "\"row\":" << position_.row << ",";
    ss << "\"col\":" << position_.col << ",";
    ss << "\"massNumber\":" << massNumber_ << ",";
    ss << "\"valenceElectrons\":" << valenceElectrons_ << ",";
    ss << "\"totalElectrons\":" << getTotalElectrons() << ",";
    ss << "\"electronegativity\":" << electronegativity_ << ",";
    ss << "\"period\":" << period_ << ",";
    ss << "\"type\":\"" << (type_ == ElementType::Metal ? "metal" : (type_ == ElementType::NobleGas ? "noble" : "nonmetal")) << "\",";
    ss << "\"sharedElectrons\":" << sharedElectrons_ << ",";
    ss << "\"extraElectrons\":" << extraElectrons_ << ",";
    ss << "\"charge\":" << charge_ << ",";
    ss << "\"status\":\"" << (status_ == PieceStatus::Stunned ? "stunned" : (status_ == PieceStatus::Radioactive ? "radioactive" : "normal")) << "\",";
    ss << "\"stunTurns\":" << stunTurns_ << ",";
    ss << "\"isRadioactive\":" << (isRadioactive_ ? "true" : "false") << ",";
    ss << "\"decayCounter\":" << decayCounter_ << ",";
    ss << "\"halfLife\":" << halfLife_ << ",";
    ss << "\"bondedTo\":[";
    for (size_t i = 0; i < bondedTo_.size(); i++) {
        if (i > 0) ss << ",";
        ss << bondedTo_[i];
    }
    ss << "],";
    ss << "\"bondType\":\"" << (bondType_ == BondType::Ionic ? "ionic" : (bondType_ == BondType::Covalent ? "covalent" : (bondType_ == BondType::Metallic ? "metallic" : "none"))) << "\",";
    ss << "\"alive\":" << (alive_ ? "true" : "false");
    ss << "}";
    return ss.str();
}

} // namespace chemiss
