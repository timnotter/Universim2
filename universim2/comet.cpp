#include "comet.hpp"

Comet::Comet(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination) : StellarObject(name, 5, radius, mass, meanDistance, eccentricity, inclination){
    // ------------------------------------------------- TODO -------------------------------------------------
    // Set random colour
    setColour(0xFFFF);
}

Comet::Comet(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination, int colour) : StellarObject(name, 5, radius, mass, meanDistance, eccentricity, inclination, colour){
    
}