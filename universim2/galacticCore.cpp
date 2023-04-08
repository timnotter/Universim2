#include "galacticCore.hpp"

GalacticCore::GalacticCore(const char *name, long double radius, long double mass, long double meanDistance) : StellarObject(name, 0, radius, mass, meanDistance, 0, 0){
    
}

GalacticCore::GalacticCore(const char *name, long double radius, long double mass, long double meanDistance, int colour) : StellarObject(name, 0, radius, mass, meanDistance, 0, 0, colour){
    
}