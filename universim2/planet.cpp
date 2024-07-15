#include "planet.hpp"
#include "timer.hpp"
#include "cmath"
#include "asteroid.hpp"

Planet::Planet(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination, int rings) : StellarObject(name, 3, radius, mass, meanDistance, eccentricity, inclination){
    // ------------------------------------------------- TODO -------------------------------------------------
    // Set random colour
    setColour(0x0000FF);
    if(rings == 1){
        // addRing(getRadius() * 3, 0, 100);
    }
}

Planet::Planet(const char *name, double radius, double mass, double meanDistance, double eccentricity, double inclination, int rings, int colour) : StellarObject(name, 3, radius, mass, meanDistance, eccentricity, inclination, colour){
    
}

void Planet::addRing(double meanDistance, double meanInclination, int numberOfAsteroids){
    struct timespec currTime;
    long double randomNumber;

    long double characteristicScaleLength = meanDistance;
	// We use a function derived from the cumulative distribution function from hernquist
	auto densityFunction = [characteristicScaleLength](double u) -> long double {
		return characteristicScaleLength * (std::sqrt(u) + u) / (1-u);
	};

    for(int i=0;i<numberOfAsteroids;i++){
        // We generate all values randomly.

        // We set the size of objects to 3000m in radius, just so we can see them
        long double radius = 2000;
        // getTime(&currTime, 0);
        // srand(currTime.tv_nsec);
        // randomNumber = (long double)rand() / RAND_MAX;
        // long double radius = std::max(randomNumber * 1000, (long double) 100);

        // We set the mass to be uniformly to 1000 tons
        long double mass = 1000000;

        // Random distance
        long double meanAsteroidDistance;
        do{
            getTime(&currTime, 0);
            srand(currTime.tv_nsec);
            randomNumber = (long double)rand() / RAND_MAX;
            meanAsteroidDistance = (densityFunction(randomNumber));
        } while (meanAsteroidDistance > getRadius() * 4 || meanAsteroidDistance < getRadius() * 2);

        // We set eccentricity to 0
        long double eccentricity = 0;

        // We cap the inclination at 0.05
        getTime(&currTime, 0);
        srand(currTime.tv_nsec);
        long double inclination = meanInclination + std::min(std::pow((long double)rand() / RAND_MAX, 15), (long double) 0.05);
        addChild(new Asteroid("Ast", radius, mass, meanAsteroidDistance, eccentricity, inclination));
    }

}