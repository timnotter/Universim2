#ifndef ASTEROID_HPP
#define ASTEROID_HPP

#include <vector>
#include "stellarObject.hpp"

class Asteroid : public StellarObject{
public:
    Asteroid(const char *name, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination);
    Asteroid(const char *name, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination, int colour);

    void setRandomForm();
};

#endif