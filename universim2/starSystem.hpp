#ifndef STARSYSTEM_HPP
#define STARSYSTEM_HPP

#include <vector>
#include "stellarObject.hpp"

class StarSystem : public StellarObject{
private:
    // Flag if starsystem consists only of one star, then we don't need to calculate local acceleration
    bool loneStar = true;
public:
    // Spawn starsystem with a number of "premade" stars
    StarSystem(const char *name, double meanDistance, double eccentricity, double inclination, int numberOfStars = 0);
    StarSystem(int numberOfStars = 1);
    void generateName();
    void setLoneStar(bool loneStar);
    bool getLoneStar();
};

#endif