#ifndef COMET_HPP
#define COMET_HPP

#include <vector>
#include "stellarObject.hpp"

class Comet : public StellarObject{
public:
    Comet(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination);
    Comet(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination, int colour);
};

#endif