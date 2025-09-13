#include "moon.hpp"

Moon::Moon(const char *name, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination) : StellarObject(name, 4, radius, mass, meanDistance, eccentricity, inclination){
    // ------------------------------------------------- TODO -------------------------------------------------
    // Set random colour
    setColour(0x964B00);

    // Set the form. If the moon is smaller that 3000m in radius, we set an irregular form
    if(getRadius() <= 3000){
        // ------------------------------------------------- TODO -------------------------------------------------
        // Generate a random form for 
        // setForm(PositionVector(1.5, 1, 0.75));
    }
}

Moon::Moon(const char *name, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination, int colour) : StellarObject(name, 4, radius, mass, meanDistance, eccentricity, inclination, colour){
    
}