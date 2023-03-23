#ifndef MOON_HPP
#define MOON_HPP

#include <vector>
#include "stellarObject.hpp"

class Moon : public StellarObject{
public:
    Moon(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination);
    Moon(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination, int colour);
};

#endif