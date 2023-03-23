#include "planet.hpp"

Planet::Planet(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination) : StellarObject(name, 3, radius, mass, meanDistance, eccentricity, inclination){
    // ------------------------------------------------- TODO -------------------------------------------------
    // Set random colour
    setColour(0x0000FF);
}

Planet::Planet(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination, int colour) : StellarObject(name, 3, radius, mass, meanDistance, eccentricity, inclination, colour){
    
}