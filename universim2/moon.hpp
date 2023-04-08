#ifndef MOON_HPP
#define MOON_HPP

#include <vector>
#include "stellarObject.hpp"

class Moon : public StellarObject{
public:
    Moon(const char *name, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination);
    Moon(const char *name, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination, int colour);
};

#endif