#include "stellarObjects/comet.hpp"

Comet::Comet(const char *name, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination) : StellarObject(name, 5, radius, mass, meanDistance, eccentricity, inclination){
    // ------------------------------------------------- TODO -------------------------------------------------
    // Set random colour
    setColour(0xFFFF);
}

Comet::Comet(const char *name, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination, int colour) : StellarObject(name, 5, radius, mass, meanDistance, eccentricity, inclination, colour){
    
}
