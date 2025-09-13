#ifndef STARSYSTEM_HPP
#define STARSYSTEM_HPP

#include <vector>
#include <functional>
#include "stellarObject.hpp"

class StarSystem : public StellarObject{
private:
    // Flag if starsystem consists only of one star, then we don't need to calculate local acceleration
    bool loneStar;
public:
    // Spawn starsystem with a number of "premade" stars
    StarSystem(const char *name, long double meanDistance, long double eccentricity, long double inclination, int numberOfStars = 0);
    StarSystem(std::function<long double(double)> densityFunction, int numberOfStars = 1);
    void generateName();
    void setLoneStar(bool loneStar);
    bool getLoneStar();
};

#endif