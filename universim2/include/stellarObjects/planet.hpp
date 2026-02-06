#ifndef PLANET_HPP
#define PLANET_HPP

#include <vector>
#include "stellarObjects/stellarObject.hpp"

class Planet : public StellarObject{
public:
    // Planets should orbit around a star, or in case of circumbinary planets a star system with multiple stars - circumbinary planets cannot be initialised yet
    Planet(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination, int rings);
    Planet(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination, int rings, int colour);

    void addRing(double meanDistance, double meanInclination, int numberOfAsteroids);
};

#endif
