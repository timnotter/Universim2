#include "galacticCore.hpp"

GalacticCore::GalacticCore(const char *name, double radius, double mass, double meanDistance) : StellarObject(name, 0, radius, mass, meanDistance, 0, 0){
    
}

GalacticCore::GalacticCore(const char *name, double radius, double mass, double meanDistance, int colour) : StellarObject(name, 0, radius, mass, meanDistance, 0, 0, colour){
    
}