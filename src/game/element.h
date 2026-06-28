#ifndef CHEMISS_ELEMENT_H
#define CHEMISS_ELEMENT_H

#include <string>
#include <vector>
#include <map>

namespace chemiss {

enum class ElementType {
    Nonmetal,
    Metal,
    NobleGas
};

enum class DecayMode {
    None,
    Alpha,    // α decay
    Beta,     // β decay
    Gamma     // γ decay (usually accompanies alpha/beta)
};

struct Isotope {
    std::string symbol;      // e.g., "Li-6"
    int atomicNumber;        // Z
    int massNumber;          // A
    DecayMode decayMode;
    int halfLife;            // in turns (0 = stable)
    bool isRadioactive;
};

struct Element {
    std::string symbol;      // e.g., "H"
    std::string name;        // e.g., "Hydrogen"
    int atomicNumber;        // Z
    int valenceElectrons;    // valence e-
    double electronegativity; // Pauling scale
    int period;              // row in periodic table
    ElementType type;
    std::vector<Isotope> isotopes;
};

// Global element database
class ElementTable {
public:
    static ElementTable& instance();
    
    const Element* getElement(const std::string& symbol) const;
    const Element* getElementByZ(int z) const;
    const Isotope* getIsotope(const std::string& symbol, int massNumber) const;
    
    std::vector<std::string> getAllSymbols() const;
    
private:
    ElementTable();
    void initElements();
    
    std::map<std::string, Element> elements_;
    std::map<int, std::string> zToSymbol_;
};

} // namespace chemiss

#endif // CHEMISS_ELEMENT_H
