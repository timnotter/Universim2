#include <string>
#include "star.hpp"
#include "starSystem.hpp"
#include "constants.hpp"

static int randomStarSystemsGenerated = 0;

StarSystem::StarSystem(const char *name, double meanDistance, double eccentricity, double inclination, int numberOfStars) : StellarObject(name, 1, 0, 0, meanDistance, eccentricity, inclination){
    if(numberOfStars != 0){
        // Create needed number of stars with functioning orbits
    }
}

StarSystem::StarSystem(int numberOfStars) : StellarObject("", 1, 0, 0, 0, 0, 0){
    generateName();
    struct timespec currTime;
    clock_gettime(CLOCK_MONOTONIC, &currTime);
    srand(currTime.tv_nsec);

    // We generate a number between 0 and 5 to determine distance to galactic core
    double randomNumber = (double)rand() / RAND_MAX * 5;
    setMeanDistance(randomNumber * distanceSagittariusSun);
    // Random number between 0 and 0.2 for inclination
    randomNumber = (double)rand() / RAND_MAX / 5;
    setInclination(randomNumber);
    // Random number between 0 and 0.1 for eccentricity
    randomNumber = (double)rand() / RAND_MAX / 10;
    setEccentricity(randomNumber);
    while(numberOfStars-- > 0){
        addChild(new Star());
    }
}

void StarSystem::generateName(){
    // Generates name according to spectral classification
    // Fuck knows why this stringification is neccesary
    std::string starSystemName = "Starsystem" + std::to_string(randomStarSystemsGenerated++);
    setName(starSystemName.c_str());
}

void StarSystem::setLoneStar(bool loneStar){
    this->loneStar = loneStar;
}

bool StarSystem::getLoneStar(){
    return loneStar;
}

