#ifndef GALACTICCORE_HPP
#define GALACTICCORE_HPP

#include <vector>
#include "stellarObject.hpp"

class GalacticCore : public StellarObject{
    public:
        GalacticCore(const char *name, double radius, double mass, double meanDistance);
        GalacticCore(const char *name, double radius, double mass, double meanDistance, int colour);
};

#endif