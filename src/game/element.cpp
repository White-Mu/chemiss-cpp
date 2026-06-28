#include "element.h"
#include <cmath>

namespace chemiss {

ElementTable& ElementTable::instance() {
    static ElementTable table;
    return table;
}

const Element* ElementTable::getElement(const std::string& symbol) const {
    auto it = elements_.find(symbol);
    if (it != elements_.end()) return &(it->second);
    return nullptr;
}

const Element* ElementTable::getElementByZ(int z) const {
    auto it = zToSymbol_.find(z);
    if (it != zToSymbol_.end()) return getElement(it->second);
    return nullptr;
}

const Isotope* ElementTable::getIsotope(const std::string& symbol, int massNumber) const {
    const Element* e = getElement(symbol);
    if (!e) return nullptr;
    for (const auto& iso : e->isotopes) {
        if (iso.massNumber == massNumber) return &iso;
    }
    return nullptr;
}

std::vector<std::string> ElementTable::getAllSymbols() const {
    std::vector<std::string> symbols;
    for (const auto& kv : elements_) {
        symbols.push_back(kv.first);
    }
    return symbols;
}

ElementTable::ElementTable() {
    initElements();
}

void ElementTable::initElements() {
    auto add = [&](const Element& e) {
        elements_[e.symbol] = e;
        zToSymbol_[e.atomicNumber] = e.symbol;
    };
    
    // Helper for isotopes
    auto stable = [](int a) { return Isotope{"", 0, a, DecayMode::None, 0, false}; };
    auto radioactive = [](int a, DecayMode m, int hl) { 
        return Isotope{"", 0, a, m, hl, true}; 
    };
    
    // Period 1
    add(Element{"H", "Hydrogen", 1, 1, 2.20, 1, ElementType::Nonmetal, {
        stable(1), stable(2)
    }});
    add(Element{"He", "Helium", 2, 2, 0.00, 1, ElementType::NobleGas, {
        stable(4), stable(3)
    }});
    
    // Period 2
    add(Element{"Li", "Lithium", 3, 1, 0.98, 2, ElementType::Metal, {
        stable(7), stable(6)
    }});
    add(Element{"Be", "Beryllium", 4, 2, 1.57, 2, ElementType::Metal, {
        stable(9), radioactive(7, DecayMode::Beta, 50)
    }});
    add(Element{"B", "Boron", 5, 3, 2.04, 2, ElementType::Nonmetal, {
        stable(11), stable(10)
    }});
    add(Element{"C", "Carbon", 6, 4, 2.55, 2, ElementType::Nonmetal, {
        stable(12), stable(13), radioactive(14, DecayMode::Beta, 1000)
    }});
    add(Element{"N", "Nitrogen", 7, 5, 3.04, 2, ElementType::Nonmetal, {
        stable(14), stable(15)
    }});
    add(Element{"O", "Oxygen", 8, 6, 3.44, 2, ElementType::Nonmetal, {
        stable(16), stable(17), stable(18)
    }});
    add(Element{"F", "Fluorine", 9, 7, 3.98, 2, ElementType::Nonmetal, {
        stable(19)
    }});
    add(Element{"Ne", "Neon", 10, 8, 0.00, 2, ElementType::NobleGas, {
        stable(20), stable(21), stable(22)
    }});
    
    // Period 3
    add(Element{"Na", "Sodium", 11, 1, 0.93, 3, ElementType::Metal, {
        stable(23), radioactive(22, DecayMode::Beta, 100)
    }});
    add(Element{"Mg", "Magnesium", 12, 2, 1.31, 3, ElementType::Metal, {
        stable(24), stable(25), stable(26), radioactive(28, DecayMode::Beta, 50)
    }});
    add(Element{"Al", "Aluminum", 13, 3, 1.61, 3, ElementType::Metal, {
        stable(27), radioactive(26, DecayMode::Beta, 200)
    }});
    add(Element{"Si", "Silicon", 14, 4, 1.90, 3, ElementType::Nonmetal, {
        stable(28), stable(29), stable(30), radioactive(31, DecayMode::Beta, 150)
    }});
    add(Element{"P", "Phosphorus", 15, 5, 2.19, 3, ElementType::Nonmetal, {
        stable(31), radioactive(32, DecayMode::Beta, 30)
    }});
    add(Element{"S", "Sulfur", 16, 6, 2.58, 3, ElementType::Nonmetal, {
        stable(32), stable(33), stable(34), stable(36)
    }});
    add(Element{"Cl", "Chlorine", 17, 7, 3.16, 3, ElementType::Nonmetal, {
        stable(35), stable(37)
    }});
    add(Element{"Ar", "Argon", 18, 8, 0.00, 3, ElementType::NobleGas, {
        stable(40), stable(36), stable(38)
    }});
    
    // Period 4 (selected elements for transmutation targets)
    add(Element{"K", "Potassium", 19, 1, 0.82, 4, ElementType::Metal, {
        stable(39), stable(41), radioactive(40, DecayMode::Beta, 200)
    }});
    add(Element{"Ca", "Calcium", 20, 2, 1.00, 4, ElementType::Metal, {
        stable(40), stable(42), stable(43), stable(44), stable(46)
    }});
    add(Element{"Fe", "Iron", 26, 2, 1.83, 4, ElementType::Metal, {
        stable(56), stable(54), stable(57), stable(58)
    }});
    add(Element{"Cu", "Copper", 29, 1, 1.90, 4, ElementType::Metal, {
        stable(63), stable(65)
    }});
    add(Element{"Zn", "Zinc", 30, 2, 1.65, 4, ElementType::Metal, {
        stable(64), stable(66), stable(67), stable(68), stable(70)
    }});
    add(Element{"Ga", "Gallium", 31, 3, 1.81, 4, ElementType::Metal, {
        stable(69), stable(71)
    }});
    add(Element{"Ge", "Germanium", 32, 4, 2.01, 4, ElementType::Metal, {
        stable(74), stable(72), stable(73), stable(76)
    }});
    add(Element{"As", "Arsenic", 33, 5, 2.18, 4, ElementType::Nonmetal, {
        stable(75), radioactive(73, DecayMode::Beta, 200)
    }});
    add(Element{"Se", "Selenium", 34, 6, 2.55, 4, ElementType::Nonmetal, {
        stable(80), stable(78), stable(76), stable(77), stable(82)
    }});
    add(Element{"Br", "Bromine", 35, 7, 2.96, 4, ElementType::Nonmetal, {
        stable(79), stable(81), radioactive(80, DecayMode::Beta, 50)
    }});
    add(Element{"Kr", "Krypton", 36, 8, 3.00, 4, ElementType::NobleGas, {
        stable(84), stable(86), stable(82), stable(83), stable(80)
    }});
    
    // Period 5 (selected)
    add(Element{"Rb", "Rubidium", 37, 1, 0.82, 5, ElementType::Metal, {
        stable(85), radioactive(87, DecayMode::Beta, 1000)
    }});
    add(Element{"Sr", "Strontium", 38, 2, 0.95, 5, ElementType::Metal, {
        stable(88), stable(86), stable(87), radioactive(90, DecayMode::Beta, 30)
    }});
    add(Element{"Ag", "Silver", 47, 1, 1.93, 5, ElementType::Metal, {
        stable(107), stable(109), radioactive(110, DecayMode::Beta, 30)
    }});
    add(Element{"I", "Iodine", 53, 7, 2.66, 5, ElementType::Nonmetal, {
        stable(127), radioactive(131, DecayMode::Beta, 20)
    }});
    add(Element{"Xe", "Xenon", 54, 8, 2.60, 5, ElementType::NobleGas, {
        stable(132), stable(129), stable(131), stable(134), stable(136)
    }});
    
    // Period 6 (selected including some heavy elements for fission)
    add(Element{"Cs", "Cesium", 55, 1, 0.79, 6, ElementType::Metal, {
        stable(133), radioactive(137, DecayMode::Beta, 40)
    }});
    add(Element{"Ba", "Barium", 56, 2, 0.89, 6, ElementType::Metal, {
        stable(138), stable(137), stable(136), stable(135), stable(134), stable(132)
    }});
    add(Element{"Au", "Gold", 79, 1, 2.54, 6, ElementType::Metal, {
        stable(197), stable(195), stable(193)
    }});
    add(Element{"Hg", "Mercury", 80, 2, 2.00, 6, ElementType::Metal, {
        stable(202), stable(200), stable(199), stable(198), stable(201), stable(204)
    }});
    add(Element{"Pb", "Lead", 82, 4, 2.33, 6, ElementType::Metal, {
        stable(208), stable(206), stable(207), stable(204), radioactive(210, DecayMode::Beta, 30)
    }});
    add(Element{"Bi", "Bismuth", 83, 5, 2.02, 6, ElementType::Metal, {
        stable(209), radioactive(210, DecayMode::Beta, 7)
    }});
    
    // Period 7 (heavy elements for fission/radioactivity)
    add(Element{"Ra", "Radium", 88, 2, 0.90, 7, ElementType::Metal, {
        radioactive(226, DecayMode::Alpha, 30), radioactive(228, DecayMode::Beta, 10)
    }});
    add(Element{"Th", "Thorium", 90, 4, 1.30, 7, ElementType::Metal, {
        radioactive(232, DecayMode::Alpha, 50), radioactive(230, DecayMode::Alpha, 40)
    }});
    add(Element{"U", "Uranium", 92, 6, 1.38, 7, ElementType::Metal, {
        radioactive(238, DecayMode::Alpha, 60), radioactive(235, DecayMode::Alpha, 50)
    }});
}

} // namespace chemiss
