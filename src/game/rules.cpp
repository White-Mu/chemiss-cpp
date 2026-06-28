#include "rules.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>

namespace chemiss {

RulesEngine::RulesEngine(Board* board) : board_(board) {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

std::vector<Position> RulesEngine::getValidMoves(int pieceId) const {
    Piece* piece = board_->getPieceById(pieceId);
    if (!piece || !piece->isAlive()) return {};
    if (piece->getStatus() == PieceStatus::Stunned) return {};
    
    std::vector<Position> validMoves;
    Position from = piece->getPosition();
    int maxSteps = piece->getMaxMovementSteps();
    auto directions = getPieceDirections(pieceId);
    
    for (const auto& dir : directions) {
        for (int step = 1; step <= maxSteps; step++) {
            Position to{from.row + dir.dr * step, from.col + dir.dc * step};
            if (!to.isValid()) break;
            
            // Check if path is clear (can't jump over pieces)
            if (!isPathClear(from, to, dir, step)) break;
            
            // If target is empty, we can move there
            if (board_->isPositionEmpty(to)) {
                validMoves.push_back(to);
            } else {
                // Target has a piece - check if we can capture
                Piece* target = board_->getPieceAt(to);
                if (target && target->isAlive() && target->getOwner() != piece->getOwner()) {
                    if (canCapture(pieceId, target->getId())) {
                        validMoves.push_back(to); // capture move
                    }
                }
                break; // Can't move past any piece
            }
        }
    }
    
    return validMoves;
}

bool RulesEngine::isValidMove(int pieceId, Position to) const {
    auto moves = getValidMoves(pieceId);
    for (const auto& pos : moves) {
        if (pos == to) return true;
    }
    return false;
}

bool RulesEngine::canCapture(int attackerId, int targetId) const {
    if (isIonicCapture(attackerId, targetId)) return true;
    if (isCovalentOrMetallicCapture(attackerId, targetId)) return true;
    return false;
}

bool RulesEngine::isIonicCapture(int attackerId, int targetId) const {
    Piece* attacker = board_->getPieceById(attackerId);
    Piece* target = board_->getPieceById(targetId);
    if (!attacker || !target || !attacker->isAlive() || !target->isAlive()) return false;
    
    // Noble gases cannot be captured
    if (target->getType() == ElementType::NobleGas) return false;
    if (attacker->getType() == ElementType::NobleGas) return false;
    
    // One metal, one nonmetal
    bool attackerMetal = attacker->getType() == ElementType::Metal;
    bool targetMetal = target->getType() == ElementType::Metal;
    if (attackerMetal == targetMetal) return false; // both same type
    
    // Electronegativity difference > 1.7
    double diff = std::abs(attacker->getElectronegativity() - target->getElectronegativity());
    if (diff > 1.7) return true;
    return false;
}

bool RulesEngine::isCovalentOrMetallicCapture(int attackerId, int targetId) const {
    Piece* attacker = board_->getPieceById(attackerId);
    Piece* target = board_->getPieceById(targetId);
    if (!attacker || !target || !attacker->isAlive() || !target->isAlive()) return false;
    
    // Noble gases cannot be captured
    if (target->getType() == ElementType::NobleGas) return false;
    if (attacker->getType() == ElementType::NobleGas) return false;
    
    // Target must be in bonded state
    if (!target->isBonded()) return false;
    
    // Attacker must not be a bond partner of target
    const auto& bonds = target->getBondedPieces();
    for (int id : bonds) {
        if (id == attackerId) return false;
    }
    
    return true;
}

void RulesEngine::updateBonds() {
    // Clear all bonds first
    auto pieces = board_->getAllPieces();
    for (auto* p : pieces) {
        p->clearBonds();
    }
    
    // Find all adjacent pairs of different colors
    for (auto* a : pieces) {
        for (auto* b : pieces) {
            if (a->getId() >= b->getId()) continue; // avoid duplicates
            if (!a->isAlive() || !b->isAlive()) continue;
            if (a->getOwner() == b->getOwner()) continue; // must be different colors
            
            if (!isAdjacent(a->getPosition(), b->getPosition())) continue;
            
            // Noble gases don't bond
            if (a->getType() == ElementType::NobleGas || b->getType() == ElementType::NobleGas) continue;
            
            double diff = std::abs(a->getElectronegativity() - b->getElectronegativity());
            bool bothMetal = a->getType() == ElementType::Metal && b->getType() == ElementType::Metal;
            bool canBond = (diff <= 1.7) || (a->getType() == ElementType::Nonmetal && b->getType() == ElementType::Nonmetal);
            
            if (bothMetal) {
                // Metallic bond
                a->addBond(b->getId());
                b->addBond(a->getId());
                a->setBondType(BondType::Metallic);
                b->setBondType(BondType::Metallic);
            } else if (canBond) {
                // Covalent bond - each atom can only have one covalent bond
                if (a->getBondedPieces().empty() && b->getBondedPieces().empty()) {
                    a->addBond(b->getId());
                    b->addBond(a->getId());
                    a->setBondType(BondType::Covalent);
                    b->setBondType(BondType::Covalent);
                }
            }
        }
    }
    
    // Calculate shared electrons for bonded pieces
    calculateSharedElectrons();
}

void RulesEngine::breakBonds(int pieceId) {
    Piece* p = board_->getPieceById(pieceId);
    if (!p) return;
    const auto& bonds = p->getBondedPieces();
    for (int otherId : bonds) {
        Piece* other = board_->getPieceById(otherId);
        if (other) other->removeBond(pieceId);
    }
    p->clearBonds();
}

void RulesEngine::calculateSharedElectrons() {
    // For covalent bonds: calculate shared electrons for each pair
    auto pieces = board_->getAllPieces();
    for (auto* p : pieces) {
        if (!p->isAlive() || !p->isBonded()) continue;
        
        if (p->getBondType() == BondType::Covalent) {
            const auto& bonds = p->getBondedPieces();
            if (bonds.empty()) continue;
            Piece* partner = board_->getPieceById(bonds[0]);
            if (!partner) continue;
            int shared = getCovalentSharedElectrons(p, partner);
            p->setSharedElectrons(shared);
        } else if (p->getBondType() == BondType::Metallic) {
            int shared = getMetallicSharedElectrons(p);
            p->setSharedElectrons(shared);
        }
    }
}

bool RulesEngine::canTransmute(int pieceId) const {
    Piece* p = board_->getPieceById(pieceId);
    if (!p || !p->isAlive()) return false;
    if (p->getSymbol() != "Li") return false;
    
    // Must be at enemy's back rank
    int backRow = (p->getOwner() == Player::White) ? 0 : 7;
    return p->getPosition().row == backRow;
}

std::vector<std::string> RulesEngine::getTransmutationOptions(int pieceId) const {
    // Return all elements except H (can't transmute to H)
    auto symbols = ElementTable::instance().getAllSymbols();
    std::vector<std::string> options;
    for (const auto& s : symbols) {
        if (s != "H") options.push_back(s);
    }
    return options;
}

bool RulesEngine::canDonateElectron(int donorId, int recipientId) const {
    Piece* donor = board_->getPieceById(donorId);
    Piece* recipient = board_->getPieceById(recipientId);
    if (!donor || !recipient) return false;
    if (!donor->isAlive() || !recipient->isAlive()) return false;
    if (donor->getOwner() != recipient->getOwner()) return false;
    if (donor->getType() != ElementType::Metal) return false;
    if (recipient->getType() != ElementType::Nonmetal) return false;
    if (recipient->getType() == ElementType::NobleGas) return false;
    if (!isAdjacent(donor->getPosition(), recipient->getPosition())) return false;
    return true;
}

std::vector<RulesEngine::AttractionPair> RulesEngine::findElectrostaticAttractions(Player currentPlayer) const {
    std::vector<AttractionPair> result;
    
    // Find all charged pieces
    auto pieces = board_->getAllPieces();
    Piece* maxA = nullptr;
    Piece* maxB = nullptr;
    int maxDiff = -1;
    
    for (auto* a : pieces) {
        if (!a->isAlive() || a->getCharge() == 0) continue;
        for (auto* b : pieces) {
            if (!b->isAlive() || b->getId() <= a->getId()) continue;
            if (b->getCharge() == 0) continue;
            if (a->getCharge() * b->getCharge() >= 0) continue; // same sign or zero
            
            int diff = std::abs(a->getCharge() - b->getCharge());
            if (diff > maxDiff) {
                maxDiff = diff;
                maxA = a;
                maxB = b;
            }
        }
    }
    
    if (maxA && maxB && maxDiff > 0) {
        AttractionPair pair;
        pair.pieceA = maxA->getId();
        pair.pieceB = maxB->getId();
        pair.chargeDiff = maxDiff;
        
        // Calculate movement: ceil(diff/2) total, split as floor/ceil
        int totalMove = (maxDiff + 1) / 2; // ceil(diff/2)
        int moveA = totalMove / 2;
        int moveB = totalMove - moveA;
        
        // For balance: non-current player may be reduced by 1 if they're the smaller mover
        if (maxA->getOwner() != currentPlayer && moveA > 0) moveA--;
        else if (maxB->getOwner() != currentPlayer && moveB > 0) moveB--;
        
        // Simple approach: move towards each other along the line connecting them
        // Calculate direction
        int dr = maxB->getPosition().row - maxA->getPosition().row;
        int dc = maxB->getPosition().col - maxA->getPosition().col;
        
        // Normalize to unit direction
        int stepDr = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);
        int stepDc = (dc == 0) ? 0 : (dc > 0 ? 1 : -1);
        
        pair.newPosA = Position{maxA->getPosition().row + stepDr * moveA,
                                 maxA->getPosition().col + stepDc * moveA};
        pair.newPosB = Position{maxB->getPosition().row - stepDr * moveB,
                                 maxB->getPosition().col - stepDc * moveB};
        
        // Clamp to board
        if (pair.newPosA.row < 0) pair.newPosA.row = 0;
        if (pair.newPosA.row > 7) pair.newPosA.row = 7;
        if (pair.newPosA.col < 0) pair.newPosA.col = 0;
        if (pair.newPosA.col > 7) pair.newPosA.col = 7;
        if (pair.newPosB.row < 0) pair.newPosB.row = 0;
        if (pair.newPosB.row > 7) pair.newPosB.row = 7;
        if (pair.newPosB.col < 0) pair.newPosB.col = 0;
        if (pair.newPosB.col > 7) pair.newPosB.col = 7;
        
        result.push_back(pair);
    }
    
    return result;
}

std::vector<RayEffect> RulesEngine::processRadioactiveDecay() {
    std::vector<RayEffect> rays;
    auto pieces = board_->getAllPieces();
    
    for (auto* p : pieces) {
        if (!p->isAlive() || !p->isRadioactive()) continue;
        
        p->incrementDecayCounter();
        if (p->getDecayCounter() >= p->getHalfLife()) {
            // 55% chance to decay
            if (std::rand() % 100 < 55) {
                // Emit ray based on decay mode
                RayEffect ray = emitRay(p->getId(), p->getDecayMode());
                rays.push_back(ray);
                
                // Change element (simple decay: Z decreases for alpha, changes for beta)
                const Element* elem = p->getElement();
                if (elem) {
                    int newZ = elem->atomicNumber;
                    if (p->getDecayMode() == DecayMode::Alpha) newZ -= 2;
                    else if (p->getDecayMode() == DecayMode::Beta) newZ += 1;
                    
                    const Element* newElem = ElementTable::instance().getElementByZ(newZ);
                    if (newElem) {
                        board_->promotePiece(p->getId(), newElem->symbol, 0);
                    }
                }
                p->resetDecayCounter();
            }
        }
    }
    
    return rays;
}

bool RulesEngine::shouldDecay(const Piece* piece) const {
    return piece && piece->isRadioactive() && piece->getDecayCounter() >= piece->getHalfLife();
}

RayEffect RulesEngine::emitRay(int pieceId, DecayMode mode) const {
    RayEffect ray;
    Piece* p = board_->getPieceById(pieceId);
    if (!p) return ray;
    
    // Random direction
    int dirIdx = std::rand() % 8;
    Direction dirs[8] = {{-1,0}, {-1,1}, {0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}};
    
    ray.from = p->getPosition();
    ray.dir = dirs[dirIdx];
    
    if (mode == DecayMode::Alpha) {
        ray.type = RayEffect::Alpha;
        ray.range = 2;
    } else if (mode == DecayMode::Beta) {
        ray.type = RayEffect::Beta;
        ray.range = 4;
    } else {
        ray.type = RayEffect::Gamma;
        ray.range = 8;
    }
    
    int hitId = -1;
    traceRay(ray.from, ray.dir, ray.range, hitId);
    ray.hitPieceId = hitId;
    
    return ray;
}

bool RulesEngine::applyRayEffect(const RayEffect& ray) {
    if (ray.hitPieceId < 0) return false;
    Piece* target = board_->getPieceById(ray.hitPieceId);
    if (!target || !target->isAlive()) return false;
    
    if (ray.type == RayEffect::Alpha) {
        return attemptAlphaTransmutation(ray.hitPieceId);
    } else if (ray.type == RayEffect::Beta) {
        return attemptBetaTransmutation(ray.hitPieceId);
    } else {
        return attemptGammaEffect(ray.hitPieceId);
    }
}

bool RulesEngine::triggerFission(int pieceId) {
    Piece* p = board_->getPieceById(pieceId);
    if (!p || !p->isAlive()) return false;
    if (p->getElement()->atomicNumber < 90) return false;
    
    // 40% chance for fission
    if (std::rand() % 100 >= 40) return false;
    
    auto products = getFissionProducts(pieceId);
    Position pos = p->getPosition();
    board_->capturePiece(pieceId);
    
    // Place products in adjacent empty cells
    int placed = 0;
    for (int dr = -1; dr <= 1 && placed < (int)products.size(); dr++) {
        for (int dc = -1; dc <= 1 && placed < (int)products.size(); dc++) {
            if (dr == 0 && dc == 0) continue;
            Position adj{pos.row + dr, pos.col + dc};
            if (adj.isValid() && board_->isPositionEmpty(adj)) {
                products[placed]->setPosition(adj);
                board_->addPiece(products[placed]);
                placed++;
            }
        }
    }
    
    return true;
}

std::vector<std::shared_ptr<Piece>> RulesEngine::getFissionProducts(int pieceId) const {
    std::vector<std::shared_ptr<Piece>> products;
    Piece* p = board_->getPieceById(pieceId);
    if (!p) return products;
    
    Player owner = p->getOwner();
    
    // Generate 2 random fission products with Z between 30-60
    for (int i = 0; i < 2; i++) {
        int z = 30 + (std::rand() % 31); // 30-60
        const Element* elem = ElementTable::instance().getElementByZ(z);
        if (elem) {
            products.push_back(std::make_shared<Piece>(elem->symbol, owner, Position{-1, -1}));
        }
    }
    return products;
}

RayEffect RulesEngine::emitBetaRayFromOverflow(int pieceId) const {
    RayEffect ray;
    ray.type = RayEffect::Beta;
    ray.range = 4;
    Piece* p = board_->getPieceById(pieceId);
    if (!p) return ray;
    
    int dirIdx = std::rand() % 8;
    Direction dirs[8] = {{-1,0}, {-1,1}, {0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}};
    ray.from = p->getPosition();
    ray.dir = dirs[dirIdx];
    
    int hitId = -1;
    traceRay(ray.from, ray.dir, ray.range, hitId);
    ray.hitPieceId = hitId;
    return ray;
}

bool RulesEngine::executeMove(const Move& move) {
    if (move.isTransmutation) {
        return executeTransmutation(move.pieceId, move.newElement, move.newMassNumber);
    }
    if (move.isElectronDonation) {
        return executeElectronDonation(move.donorPieceId, move.recipientPieceId);
    }
    
    Piece* piece = board_->getPieceById(move.pieceId);
    if (!piece || !piece->isAlive()) return false;
    
    if (move.isCapture) {
        Piece* target = board_->getPieceById(move.capturePieceId);
        if (target) {
            if (isIonicCapture(move.pieceId, move.capturePieceId)) {
                // Gain target's valence electrons as extra electrons for 3 turns
                int extra = target->getValenceElectrons();
                int total = piece->getTotalElectrons() + extra;
                if (total > 8) {
                    // Emit beta ray
                    RayEffect ray = emitBetaRayFromOverflow(move.pieceId);
                    if (ray.hitPieceId >= 0) {
                        applyRayEffect(ray);
                    }
                }
                piece->setExtraElectrons(extra, 3);
            }
            board_->capturePiece(move.capturePieceId);
        }
    }
    
    board_->movePiece(move.pieceId, move.to);
    
    // Update bonds after move
    updateBonds();
    
    return true;
}

bool RulesEngine::executeTransmutation(int pieceId, const std::string& element, int massNumber) {
    if (!canTransmute(pieceId)) return false;
    return board_->promotePiece(pieceId, element, massNumber);
}

bool RulesEngine::executeElectronDonation(int donorId, int recipientId) {
    if (!canDonateElectron(donorId, recipientId)) return false;
    
    Piece* donor = board_->getPieceById(donorId);
    Piece* recipient = board_->getPieceById(recipientId);
    if (!donor || !recipient) return false;
    
    donor->setCharge(+1, 3);       // Cation
    recipient->setCharge(-1, 3);  // Anion
    recipient->setValenceElectrons(recipient->getValenceElectrons() + 1);
    
    return true;
}

void RulesEngine::processTurnStart(Player currentPlayer) {
    // Apply electrostatic attractions
    auto attractions = findElectrostaticAttractions(currentPlayer);
    for (const auto& attr : attractions) {
        Piece* a = board_->getPieceById(attr.pieceA);
        Piece* b = board_->getPieceById(attr.pieceB);
        if (a && b) {
            if (board_->isPositionEmpty(attr.newPosA)) {
                board_->movePiece(a->getId(), attr.newPosA);
            }
            if (board_->isPositionEmpty(attr.newPosB)) {
                board_->movePiece(b->getId(), attr.newPosB);
            }
        }
    }
}

void RulesEngine::processTurnEnd() {
    // Process radioactive decay
    auto rays = processRadioactiveDecay();
    for (const auto& ray : rays) {
        applyRayEffect(ray);
    }
    
    // Tick effect durations
    auto pieces = board_->getAllPieces();
    for (auto* p : pieces) {
        p->tickEffects();
    }
    
    updateBonds();
}

bool RulesEngine::isKingCaptured(Player player) const {
    return board_->getKing(player) == nullptr;
}

bool RulesEngine::canPlayerMove(Player player) const {
    auto pieces = board_->getPlayerPieces(player);
    for (auto* p : pieces) {
        if (!getValidMoves(p->getId()).empty()) return true;
    }
    return false;
}

bool RulesEngine::isNobleGas(const Piece* piece) const {
    return piece && piece->getType() == ElementType::NobleGas;
}

// Private helpers

std::vector<Direction> RulesEngine::getPieceDirections(int pieceId) const {
    Piece* piece = board_->getPieceById(pieceId);
    if (!piece) return {};
    
    std::vector<Direction> baseDirs = piece->getMovementDirections();
    
    // Apply mirror for right half of board (col 4-7)
    Position pos = piece->getPosition();
    if (pos.col >= 4) {
        for (auto& dir : baseDirs) {
            dir.dc = -dir.dc;
        }
    }
    
    return baseDirs;
}

bool RulesEngine::isPathClear(Position from, Position to, Direction dir, int steps) const {
    for (int s = 1; s < steps; s++) {
        Position check{from.row + dir.dr * s, from.col + dir.dc * s};
        if (!check.isValid()) return false;
        if (!board_->isPositionEmpty(check)) return false;
    }
    return true;
}

bool RulesEngine::isAdjacent(Position a, Position b) const {
    return std::abs(a.row - b.row) <= 1 && std::abs(a.col - b.col) <= 1 && !(a == b);
}

int RulesEngine::getCovalentSharedElectrons(const Piece* a, const Piece* b) const {
    if (!a || !b) return 0;
    
    // For H: needs 1 more to reach 2 (Helium-like)
    int needA = (a->getSymbol() == "H") ? 1 : (8 - a->getValenceElectrons());
    int needB = (b->getSymbol() == "H") ? 1 : (8 - b->getValenceElectrons());
    
    if (needA < 0) needA = 0;
    if (needB < 0) needB = 0;
    
    return std::min(needA, needB);
}

int RulesEngine::getMetallicSharedElectrons(const Piece* piece) const {
    if (!piece || piece->getBondType() != BondType::Metallic) return 0;
    // In metallic cluster, each gets +2
    return 2;
}

Position RulesEngine::traceRay(Position from, Direction dir, int maxRange, int& hitPieceId) const {
    hitPieceId = -1;
    for (int step = 1; step <= maxRange; step++) {
        Position pos{from.row + dir.dr * step, from.col + dir.dc * step};
        if (!pos.isValid()) break;
        Piece* p = board_->getPieceAt(pos);
        if (p && p->isAlive()) {
            hitPieceId = p->getId();
            return pos;
        }
    }
    return from;
}

bool RulesEngine::attemptAlphaTransmutation(int pieceId) {
    if (triggerFission(pieceId)) return true;
    
    Piece* p = board_->getPieceById(pieceId);
    if (!p) return false;
    
    // Alpha absorption: Z + 2, mass + 4
    const Element* elem = p->getElement();
    if (!elem) return false;
    
    int newZ = elem->atomicNumber + 2;
    int newMass = p->getMassNumber() + 4;
    
    const Element* newElem = ElementTable::instance().getElementByZ(newZ);
    if (newElem) {
        const Isotope* iso = ElementTable::instance().getIsotope(newElem->symbol, newMass);
        if (iso) {
            board_->promotePiece(pieceId, newElem->symbol, newMass);
            p->stun(1);
            return true;
        }
    }
    
    // Fallback: valence -1 and stun
    int ve = p->getValenceElectrons() - 1;
    if (ve < 1) ve = 1;
    p->setValenceElectrons(ve);
    p->stun(1);
    return true;
}

bool RulesEngine::attemptBetaTransmutation(int pieceId) {
    if (triggerFission(pieceId)) return true;
    
    Piece* p = board_->getPieceById(pieceId);
    if (!p) return false;
    
    // Random +/-1 valence electron
    int change = (std::rand() % 2 == 0) ? 1 : -1;
    int newValence = p->getValenceElectrons() + change;
    if (newValence < 1) newValence = 1;
    if (newValence > 8) newValence = 8;
    
    int extra = newValence - p->getValenceElectrons();
    p->setValenceElectrons(newValence);
    if (extra > 0) {
        p->setExtraElectrons(extra, 3);
    }
    return true;
}

bool RulesEngine::attemptGammaEffect(int pieceId) {
    if (triggerFission(pieceId)) return true;
    
    Piece* p = board_->getPieceById(pieceId);
    if (!p) return false;
    
    p->stun(1);
    return true;
}

} // namespace chemiss
