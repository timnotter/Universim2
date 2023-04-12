#include <string>
#include "star.hpp"
#include "starSystem.hpp"
#include "constants.hpp"

static int randomStarSystemsGenerated = 0;

StarSystem::StarSystem(const char *name, long double meanDistance, long double eccentricity, long double inclination, int numberOfStars) : StellarObject(name, 1, 0, 0, meanDistance, eccentricity, inclination){
    loneStar = true;
    if(numberOfStars != 0){
        // Create needed number of stars with functioning orbits
    }
}

StarSystem::StarSystem(std::function<long double(double)> densityFunction, int numberOfStars) : StellarObject("", 1, 0, 0, 0, 0, 0){
    loneStar = true;
    generateName();
    struct timespec currTime;
    clock_gettime(CLOCK_MONOTONIC, &currTime);
    srand(currTime.tv_nsec);

    // // We generate a number between 0 and 1 to determine mean distance, according to the delivered density function

    long double randomNumber = (long double)rand() / RAND_MAX;
    setMeanDistance(densityFunction(randomNumber));



    // Random number between 0 and 0.2 for inclination
    randomNumber = (long double)rand() / RAND_MAX / 5;
    setInclination(randomNumber);
    // Random number between 0 and 0.1 for eccentricity
    randomNumber = (long double)rand() / RAND_MAX / 10;
    setEccentricity(randomNumber);
    while(numberOfStars-- > 0){
        // printf("Adding child to %s\n", getName());
        addChild(new Star());
    }
    // printf("%s has now %ld children\n", getName(), getChildren()->size());
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

