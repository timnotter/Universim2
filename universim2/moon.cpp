#include "moon.hpp"

Moon::Moon(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination) : StellarObject(name, 4, radius, mass, meanDistance, eccentricity, inclination){
    // ------------------------------------------------- TODO -------------------------------------------------
    // Set random colour
    setColour(0x964B00);
}

Moon::Moon(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination, int colour) : StellarObject(name, 4, radius, mass, meanDistance, eccentricity, inclination, colour){
    
}