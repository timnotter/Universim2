#include <fstream>
#include <string>
#include <time.h>
#include "star.hpp"
#include "constants.hpp"

static int randomStarsGenerated = 0;

Star::Star(const char *name, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination, int surfaceTemperature) : StellarObject(name, 2, radius, mass, meanDistance, eccentricity, inclination){
    if(surfaceTemperature==0){
        // Calculate surface temperature
    }
    else{
        this->surfaceTemperature = surfaceTemperature;
    }
    determineClassification();
    findColour();
}

Star::Star(const char *name, long double mass, long double meanDistance, long double eccentricity, long double inclination) : StellarObject(name, 2, 0, mass, meanDistance, eccentricity, inclination){
    //------------------------------------------------------------ TODO, but probably being phased out ------------------------------------------------------------
    // Adjust radius and surface temperature to fit mass
    surfaceTemperature = 6000;
    determineClassification();
    approximateRadiusFromMassAndSpectralType();
    findColour();
}

Star::Star(const char *name, long double meanDistance, long double eccentricity, long double inclination) : StellarObject(name, 2, 0, 0, meanDistance, eccentricity, inclination){
    //------------------------------------------------------------ TODO ------------------------------------------------------------
    // Create a random star with real probabilities
    generateAndDetermineClassification();

}

Star::Star() : StellarObject("", 2, 0, 0, 0, 0, 0){
    // Randomize all values, with the star being at the center of the star system
    // This function generates a surfaceTemperature, mass and radius and saves spectral classification
    generateAndDetermineClassification();
    findColour();
    generateName();
    // printf("Generated star %s with radius %fm = %fsR and mass %fm = %fsM\n", getName(), getRadius(), getRadius() / solarRadius, getMass(), getMass() / solarMass);
}

int Star::getSurfaceTemperature(){
    return surfaceTemperature;
}

char Star::getSpectralType(){
    return spectralType;
}

char Star::getSpectralSubType(){
    return spectralSubType;
}

char Star::getLuminosityClass(){
    return luminosityClass;
}

void Star::findColour(){
    // Round temperature to 100 K an ensure that it is between 1000 K and 40000 K
    int surfaceTemperatureForSearching = std::max(std::min(((surfaceTemperature / 100) * 100), 40000), 1000);
    // Get colour from surface temperature
    std::string temp;
    int r;
    int g;
    int b;
    int hex;
    std::ifstream starColourFile;
    starColourFile.open("./files/StarColours.txt");
    starColourFile >> temp;
    // printf("temp: %s\n", temp.c_str());
    while((std::stoi(temp.c_str())) != surfaceTemperatureForSearching){
        for(int i=0;i<13;i++) {
            // printf("temp: %s\n", temp.c_str());
            starColourFile >> temp;
        }
        // printf("\n");
    }
    // printf("Exited\n");
    for(int i=0;i<9;i++) starColourFile >> temp;
    // printf("temp: %s\n", temp.c_str());
    r = std::stoi(temp.c_str());
    starColourFile >> temp;
    // printf("temp: %s\n", temp.c_str());
    g = std::stoi(temp.c_str());
    starColourFile >> temp;
    // printf("temp: %s\n", temp.c_str());
    b = std::stoi(temp.c_str());
    // We are using individual rgb values, not the hex value
    // starColourFile >> temp;
    // printf("temp: %s\n", temp.c_str()+1);
    // hex = std::stoi(temp.c_str()+1);
    starColourFile.close();
    // printf("For star %s, rgb values are %d, %d, %d\n", getName(), r, g, b);
    setColour((r<<16) + (g<<8) + b);
}

void Star::generateAndDetermineClassification(){
    // For simplicity, only main sequence stars get generated and "subspectral" classification is evenly distributed
    luminosityClass = 'V';

    // Initialise random number generator
    struct timespec currTime;
    clock_gettime(CLOCK_MONOTONIC, &currTime);
    srand(currTime.tv_nsec);

    // We generate a number between 0 and 1
    long double randomNumber = (long double)rand() / RAND_MAX;

    // Now we transform this number into a spectral classification according to measured probability in solar neighbourhood
    const long double OClassProbability = 0.1133334E-4;
    const long double atLeastBClassProbability = 0.5533334E-4 + OClassProbability;
    const long double atLeastAClassProbability = 0.3635334E-2 + atLeastBClassProbability;
    const long double atLeastFClassProbability = 0.8269334E-2 + atLeastAClassProbability;
    const long double atLeastGClassProbability = 0.38151E-1 + atLeastFClassProbability;
    const long double atLeastKClassProbability = 0.1656757 + atLeastGClassProbability;
    const long double atLeastMClassProbability = 0.693402 + atLeastKClassProbability;

    int maxTemperature;
    int minTemperature;
    int temperatureDifference;

    if(randomNumber < OClassProbability){
        // Here we assume there exists subclasses from 9 up until 0
        maxTemperature = 52300;
        minTemperature = 32300;
        spectralType = 'O';
    }
    else if(randomNumber < atLeastBClassProbability){
        maxTemperature = 32300;
        minTemperature = 10200;
        spectralType = 'B';
    }
    else if(randomNumber < atLeastAClassProbability){
        maxTemperature = 10200;
        minTemperature = 7300;
        spectralType = 'A';
    }
    else if(randomNumber < atLeastFClassProbability){
        maxTemperature = 7300;
        minTemperature = 6000;
        spectralType = 'F';
    }
    else if(randomNumber < atLeastGClassProbability){
        maxTemperature = 6000;
        minTemperature = 5300;
        spectralType = 'G';
    }
    else if(randomNumber < atLeastKClassProbability){
        maxTemperature = 5300;
        minTemperature = 3900;
        spectralType = 'K';
    }
    else if(randomNumber < atLeastMClassProbability){
        generateMClass:
        maxTemperature = 3900;
        minTemperature = 2350;
        spectralType = 'M';
        // printf("MClass\n");
    }
    else{
        // Generate white dwarf
        // printf("White dwarf not implemented, rerouting to M-Class\n");
        goto generateMClass;
    }
    temperatureDifference = maxTemperature - minTemperature;

    // Now we determine the subclass with each having the same probability
    randomNumber = (long double)rand() / RAND_MAX;
    surfaceTemperature = minTemperature + (int) (temperatureDifference * randomNumber);
    if(randomNumber < 0.1){
        spectralSubType = '9';
    }
    else if(randomNumber < 0.2){
        spectralSubType = '8';
    }
    else if(randomNumber < 0.3){
        spectralSubType = '7';
    }
    else if(randomNumber < 0.4){
        spectralSubType = '6';
    }
    else if(randomNumber < 0.5){
        spectralSubType = '5';
    }
    else if(randomNumber < 0.6){
        spectralSubType = '4';
    }
    else if(randomNumber < 0.7){
        spectralSubType = '3';
    }
    else if(randomNumber < 0.8){
        spectralSubType = '2';
    }
    else if(randomNumber < 0.9){
        spectralSubType = '1';
    }
    else{
        spectralSubType = '0';
    }

    generateMass();
}

void Star::generateMass(){
    // The mass that is generated here is not totally realistic, we don't tighten the mass possibilities around the spectral subtype
    // but only around the spectral type itself
    long double maxMass;
    long double minMass;
    long double massDifference;
    switch(spectralType){
        case 'O':
            maxMass = 150 * solarMass;
            minMass = 18 * solarMass;
            // printf("O-Type Star!!\n");
            break;
        case 'B':
            maxMass = 18 * solarMass;
            minMass = 2.5 * solarMass;
            // printf("B-Type Star!!\n");
            break;
        case 'A':
            maxMass = 2.5 * solarMass;
            minMass = 1.7 * solarMass;
            break;
        case 'F':
            maxMass = 1.7 * solarMass;
            minMass = 1.1 * solarMass;
            break;
        case 'G':
            maxMass = 1.1 * solarMass;
            minMass = 0.89 * solarMass;
            break;
        case 'K':
            maxMass = 0.89 * solarMass;
            minMass = 0.58 * solarMass;
            break;
        case 'M':
            maxMass = 0.58 * solarMass;
            minMass = 0.075 * solarMass;
            break;
        default:
            printf("Spectral type not of legal value!\n");
            break;
    }
    massDifference = maxMass - minMass;

    // Initialise random number generator
    struct timespec currTime;
    clock_gettime(CLOCK_MONOTONIC, &currTime);
    srand(currTime.tv_nsec);

    // We generate a number between 0 and 1, the square will determine the total mass - to model somewhat of a bias towards smaller objects
    // The power of 2 is arbitrarily chosen and not based on science
    long double randomNumber = (long double)rand() / RAND_MAX;
    setMass(minMass + std::pow(randomNumber, 2) * massDifference);

    approximateRadiusFromMassAndSpectralType();
}

void Star::approximateRadiusFromMassAndSpectralType(){
    // We use the formula radius = solarRadius (mass/solar mass)^a, where a is a constant. We use approximations, such that we have a different
    // but also reasonably accurate a for each spectral type. 
    // Only ChatGPT knows if there values are correct :)
    // Could be done for every subtype, for which the values would be stored in a txt
    long double a;
    switch(spectralType){
        case 'O':
            a = 1.65;
            break;
        case 'B':
            a = 1.45;
            break;
        case 'A':
            a = 1.3;
            break;
        case 'F':
            a = 1.2;
            break;
        case 'G':
            a = 1;
            break;
        case 'K':
            a = 0.8;
            break;
        case 'M':
            a = 0.7;
            break;
        default:
            printf("Spectral type not of legal value!\n");
    }
    setRadius(solarRadius * std::pow((getMass()/solarMass), a));
    // printf("For %s the radius is %fm = %fsR and the mass is %fkg = %fsM\n", getName(), getRadius(), getRadius()/solarRadius, getMass(), getMass()/solarMass);
}

void Star::determineClassification(){
    // --------------------------------- Only determines spectral classification ---------------------------------
    luminosityClass = 'V';

    int maxTemperature;
    int minTemperature;
    int temperatureDifference;
    if(surfaceTemperature > 32300){
        // Here we assume there exists subclasses from 9 up until 0
        maxTemperature = 52300;
        minTemperature = 32300;
        spectralType = 'O';
    }
    else if(surfaceTemperature > 10200){
        maxTemperature = 32300;
        minTemperature = 10200;
        spectralType = 'B';
    }
    else if(surfaceTemperature > 7300){
        maxTemperature = 10200;
        minTemperature = 7300;
        spectralType = 'A';
    }
    else if(surfaceTemperature > 6000){
        maxTemperature = 7300;
        minTemperature = 6000;
        spectralType = 'F';
    }
    else if(surfaceTemperature > 5300){
        maxTemperature = 6000;
        minTemperature = 5300;
        spectralType = 'G';
    }
    else if(surfaceTemperature > 3900){
        maxTemperature = 5300;
        minTemperature = 3900;
        spectralType = 'K';
    }
    else if(surfaceTemperature > 2350){
        generateMClass:
        maxTemperature = 3900;
        minTemperature = 2350;
        spectralType = 'M';
        // printf("MClass\n");
    }
    else{
        // Generate white dwarf
        // printf("White dwarf not implemented, rerouting to M-Class\n");
        goto generateMClass;
    }
    temperatureDifference = maxTemperature - minTemperature;
    int surfaceTemperatureOverMinTemperature = surfaceTemperature - minTemperature;
    long double temperatureRelation = ((long double)surfaceTemperatureOverMinTemperature) / temperatureDifference;

    if(temperatureRelation < 0.1){
        spectralSubType = '9';
    }
    else if(temperatureRelation < 0.2){
        spectralSubType = '8';
    }
    else if(temperatureRelation < 0.3){
        spectralSubType = '7';
    }
    else if(temperatureRelation < 0.4){
        spectralSubType = '6';
    }
    else if(temperatureRelation < 0.5){
        spectralSubType = '5';
    }
    else if(temperatureRelation < 0.6){
        spectralSubType = '4';
    }
    else if(temperatureRelation < 0.7){
        spectralSubType = '3';
    }
    else if(temperatureRelation < 0.8){
        spectralSubType = '2';
    }
    else if(temperatureRelation < 0.9){
        spectralSubType = '1';
    }
    else{
        spectralSubType = '0';
    }

    // printf("Determined spectral class of %s: it is of type %c%c%c\n", getName(), spectralType, spectralSubType, luminosityClass);
}

void Star::generateName(){
    // Generates name according to spectral classification
    // Fuck knows why this stringification is neccesary
    std::string starName = std::to_string(randomStarsGenerated++) + std::string(1, '-') + std::string(1, spectralType) + std::string(1, spectralSubType) + std::string(1, luminosityClass);
    // printf("Setting name to %s\n", starName.c_str());
    setName(starName.c_str());
}