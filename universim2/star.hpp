#ifndef STAR_HPP
#define STAR_HPP

#include <vector>
#include "stellarObject.hpp"

class Star : public StellarObject{
    private:
        int surfaceTemperature;
        // Classification: O, B, A, F, G, K, M + D, S, C, indicating surface temperature - D, S, C not implemented yet
        char spectralType;
        // Number from 0 to 9, grouping spectralType further
        char spectralSubType;
        // Classification from 0 to VII, distinguishing giants from dwarfs - V is main sequence (dwarf)
        char luminosityClass;

    public:
        // Mean distance only matters for star systems with multiple stars, else it should always be 0. If it is, eccentricity and inclination should also be 0
        Star(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination, int surfaceTemperature);
        // Create a randomly sized and tempered star with mass constraint
        Star(const char *name, double mass, double meanDistance, double eccentricity, double inclination);
        // Create a randomly sized and tempered star with no mass constraint
        Star(const char *name, double meanDistance, double eccentricity, double inclination);
        // Create a fully random star
        Star();

        void findColour();
        void generateAndDetermineClassification();
        void generateMass();
        void approximateRadiusFromMassAndSpectralType();
        void determineClassification();
        void generateName();

        int getSurfaceTemperature();
        char getSpectralType();
        char getSpectralSubType();
        char getLuminosityClass();
};

#endif