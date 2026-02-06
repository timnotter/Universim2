#include "stellarObjects/asteroid.hpp"
#include "helpers/timer.hpp"
#include "cmath"

Asteroid::Asteroid(const char *name, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination) : StellarObject(name, 6, radius, mass, meanDistance, eccentricity, inclination){
    // ------------------------------------------------- TODO -------------------------------------------------
    // Set random colour
    setRandomForm();
    setColour(0x666666);
}

Asteroid::Asteroid(const char *name, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination, int colour) : StellarObject(name, 6, radius, mass, meanDistance, eccentricity, inclination, colour){
    
}

void Asteroid::setRandomForm(){
    struct timespec currTime;
    // We get a random number between 0 and 1, square it and add one to it for the length of the xAxis
    getTime(&currTime, 0);
    srand(currTime.tv_nsec); 
    long double xAxis = std::pow((long double)rand() / RAND_MAX, 2) + 1;
    // We get a random number between 0 and 1, take the fourth power and subtract it from oneto get the length of the yAxis
    getTime(&currTime, 0);
    srand(currTime.tv_nsec); 
    long double yAxis = 1 - std::pow((long double)rand() / RAND_MAX, 4);
    // We get a random number between 0 and 1, take second power and subtract it from oneto get the length of the zAxis
    getTime(&currTime, 0);
    srand(currTime.tv_nsec); 
    long double zAxis = 1 - std::pow((long double)rand() / RAND_MAX, 2);
    setForm(PositionVector(xAxis, yAxis, zAxis));
}
