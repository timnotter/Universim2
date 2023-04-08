#ifndef COMET_HPP
#define COMET_HPP

#include <vector>
#include "stellarObject.hpp"

class Comet : public StellarObject{
public:
    Comet(const char *name, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination);
    Comet(const char *name, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination, int colour);
};

#endif