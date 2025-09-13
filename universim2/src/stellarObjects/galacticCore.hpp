#ifndef GALACTICCORE_HPP
#define GALACTICCORE_HPP

#include <vector>
#include "stellarObject.hpp"

class GalacticCore : public StellarObject{
public:
    GalacticCore(const char *name, long double radius, long double mass, long double meanDistance);
    GalacticCore(const char *name, long double radius, long double mass, long double meanDistance, int colour);
};

#endif