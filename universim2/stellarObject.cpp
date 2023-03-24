#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <string>
#include "stellarObject.hpp"
#include "starSystem.hpp"
#include "constants.hpp"
#include "tree.hpp"

StellarObject::StellarObject(const char *name, int type, double radius, double mass, double meanDistance, double eccentricity, double inclination){
    // this->name = name;
    this->name = new std::string(name);
    this->type = type;
    this->radius = radius;
    this->mass = mass;
    this->meanDistance = meanDistance;
    this->eccentricity = eccentricity;
    this->inclination = inclination;

    // Correct all units in relation to respective reference units
    switch(type){
        case GALACTIC_CORE:
            this->radius *= sagittariusRadius;
            this->mass *= sagittariusMass;
            this->meanDistance *= lightyear;
            homeSystem = NULL;
            break;
        case STARSYSTEM:
            // If object is a star system, initialise homeSystem pointer to itself, such that children then can reuse it
            homeSystem = static_cast<StarSystem*>(this);
            // printf("Home system of %s has been set to %p\n", getName(), homeSystem);
            this->meanDistance *= distanceSagittariusSun;
            break;
        case STAR:
            this->radius *= solarRadius;
            this->mass *= solarMass;
            this->meanDistance *= astronomicalUnit;
            break;
        case PLANET:
            this->radius *= terranRadius;
            this->mass *= terranMass;
            this->meanDistance *= astronomicalUnit;
            break;
        case MOON:
            this->radius *= lunarRadius;
            this->mass *= lunarMass;
            this->meanDistance *= distanceEarthMoon;
            break;
        case COMET:
            this->radius *= 1;
            this->mass *= 1;
            this->meanDistance *= astronomicalUnit;
            break;
    }
}

StellarObject::StellarObject(const char *name, int type, double radius, double mass, double meanDistance, double eccentricity, double inclination, int colour) : StellarObject(name, type, radius, mass, meanDistance, eccentricity, inclination){
    this->colour = colour;
}

void StellarObject::place(){
    // ------------------------------------------------------------------------ TODO ------------------------------------------------------------------------
    // First go through all children and find total mass
    calculateTotalMass();

    // Initialise position with correct orbit in mind

    // Initialise random object with current time
    struct timespec currTime;
    clock_gettime(CLOCK_MONOTONIC, &currTime);
    srand(currTime.tv_nsec);

    // First place center of mass of system at appropriate place, then place children at appropriate places,
    // lastly, adjust position of object, such that center of mass is correct
    
    // To start, we consider a central plane on which all orbits lay, when eccentricity is 0, namely the plane z = 0

    double apoapsis;
    double periapsis;
    double distance;
    double semiMajorAxis;
    double trueAnomaly;
    double argumentOfPeriapsis;
    double longitudeOfAscendingNode;
    double eccentricAnomaly;
    double meanAnomaly;

    // If object is galactic core, place it anywhere with distance to (0/0/0) = meanDistance
    // printf("Trying to place %s\n", name);
    if(type == GALACTIC_CORE){
        centreOfMass = PositionVector();
        if(meanDistance != 0){
            // ------------------------------------------------------------------------ TODO ------------------------------------------------------------------------
            // Place different galactic core at appropriate position, for example: take two random angles to take a position of the border of ball with radius meanDistance
            centreOfMass.setX(meanDistance);
            position = centreOfMass;
        }
        for(StellarObject *child: children){
            child->place();
        }
        // --------------------------------------- Currently galactic core is not shifted to correct for centre of mass ---------------------------------------
        position = PositionVector(&centreOfMass);
        velocity = PositionVector();
    }
    else{
        // Calculation of semi major axis
        semiMajorAxis = meanDistance / (1 - std::pow(eccentricity, 2));

        // We generate random angles for true anomaly, argument of periapsis and longitude of ascending node
        trueAnomaly = (double)rand() / RAND_MAX * (2*PI);
        argumentOfPeriapsis = (double)rand() / RAND_MAX * (2*PI);
        longitudeOfAscendingNode = (double)rand() / RAND_MAX * (2*PI);
        // printf("Generated random values for %s\n", name);

        // Calculation of eccentric anomaly
        eccentricAnomaly = 2 * std::atan(std::sqrt((1 - eccentricity)/(1 + eccentricity)) * std::tan(trueAnomaly / 2));

        // Calculating of true anomaly
        meanAnomaly = eccentricAnomaly - eccentricity * std::sin(eccentricAnomaly);

        // Initialise position with position of parent
        centreOfMass = parent->getCentreOfMass();

        // printf("Starting to place %s\n", name);

        // Calculate position from values
        double factor = semiMajorAxis * (1 - std::pow(eccentricity, 2)) / (1 + eccentricity * std::cos(trueAnomaly));
        centreOfMass.setX(centreOfMass.getX() + factor * (std::cos(trueAnomaly + argumentOfPeriapsis) * std::cos(longitudeOfAscendingNode) - std::sin(trueAnomaly + argumentOfPeriapsis) * std::sin(longitudeOfAscendingNode) * std::cos(inclination)));
        centreOfMass.setY(centreOfMass.getY() + factor * (std::cos(trueAnomaly + argumentOfPeriapsis) * std::sin(longitudeOfAscendingNode) + std::sin(trueAnomaly + argumentOfPeriapsis) * std::cos(longitudeOfAscendingNode) * std::cos(inclination)));
        centreOfMass.setZ(centreOfMass.getZ() + factor * (std::sin(trueAnomaly + argumentOfPeriapsis) * std::sin(inclination)));
        // printf("Placed center of mass of %s at %f, %f, %f\n", name, centreOfMass.getX(), centreOfMass.getY(), centreOfMass.getZ());


        // -------------------------------------------------------------------------- TODO --------------------------------------------------------------------------
        // Compute speed around centre of mass of system after children are placed

        // Speed should then be: 
        // v = sqrt(mu / p) * [-sin(v + w) * cos(O) - cos(v + w) * sin(O) * cos(i), -sin(v + w) * sin(O) + cos(v + w) * cos(O) * cos(i), cos(v + w) * sin(i)]
        if(semiMajorAxis!=0){
            double mu = G * parent->getMass();
            double p = semiMajorAxis * (1 - std::pow(eccentricity, 2));

            velocity.setX(std::sqrt(mu / p) * (-1 * std::sin(trueAnomaly + argumentOfPeriapsis) * std::cos(longitudeOfAscendingNode) - std::cos(trueAnomaly + argumentOfPeriapsis) * std::sin(longitudeOfAscendingNode) * std::cos(inclination)));
            velocity.setY(std::sqrt(mu / p) * (-1 * std::sin(trueAnomaly + argumentOfPeriapsis) * std::sin(longitudeOfAscendingNode) + std::cos(trueAnomaly + argumentOfPeriapsis) * std::cos(longitudeOfAscendingNode) * std::cos(inclination)));
            velocity.setZ(std::sqrt(mu / p) * (std::cos(trueAnomaly + argumentOfPeriapsis) * std::sin(inclination)));
        }
        // printf("Velocity of parent of %s is (%f, %f, %f)\n", getName(), parent->getVelocity().getX(), parent->getVelocity().getY(), parent->getVelocity().getZ());
        velocity += parent->getVelocity();

        // -------------------------------------------------------------------------- TODO --------------------------------------------------------------------
        // Go through all children and place their center of mass, calculate the center of mass of children
        PositionVector tempCentreOfMass = PositionVector();
        double childrenTotalMass = 0;
        // printf("Trying to place children of %s\n", name);
        for(StellarObject *child: children){
            child->place();
            child->calculateTotalMass();
            double childTotalMass = child->getTotalMass();
            // printf("Total mass of system %s is %f\n", child->getName(), child->getTotalMass());
            calculateCentreOfMass();
            tempCentreOfMass += child->getCentreOfMass() * childTotalMass;
            childrenTotalMass += childTotalMass;
        }
        if(childrenTotalMass!=0)
            tempCentreOfMass /= childrenTotalMass;

        // Place object, such that centre of mass is at correct place
        if(type == STARSYSTEM){
             position.setX(centreOfMass.getX());
             position.setY(centreOfMass.getY());
             position.setZ(centreOfMass.getZ());
        }
        else{
            position.setX((centreOfMass.getX() * totalMass - tempCentreOfMass.getX() * childrenTotalMass) / mass);
            position.setY((centreOfMass.getY() * totalMass - tempCentreOfMass.getY() * childrenTotalMass) / mass);
            position.setZ((centreOfMass.getZ() * totalMass - tempCentreOfMass.getZ() * childrenTotalMass) / mass);
        }
        // printf("Object: %s, childrenTotalMass: %f, mass: %f\n", name, childrenTotalMass, mass);
        // printf("Placed %s at %f, %f, %f\n", name, position.getX(), position.getY(), position.getZ());

    }
}

void StellarObject::initialiseVelocity(){
    // ------------------------------------TODO-----------------------------------------------------
    // Initialise velocity with correct orbit in mind
    if(type == 0){
        position = PositionVector();
    }
}

void StellarObject::updatePosition(double deltaT){
    // printf("%s - move amount: (%f, %f, %f)\n", getName(), (velocity * deltaT).getX(), (velocity * deltaT).getY(), (velocity * deltaT).getZ());
    position += velocity * deltaT;
}

void StellarObject::updateVelocity(double deltaT){
    velocity += (stellarAcceleration + localAcceleration) * deltaT;
}

void StellarObject::updateStellarAcceleration(Tree *tree){
    // ------------------------------------TODO-----------------------------------------------------
    // Calculate acceleration from star(-system) to other star(-system)s. If type is not starsystem, reuse acceleration from motherstar
}

void StellarObject::updateLocalAcceleration(Tree *tree){
    // Calculate acceleration from local stellar objects - within system or nearby
    localAcceleration = tree->getRoot()->calculateAcceleration(this);
}

void StellarObject::addChild(StellarObject *child){
    // printf("Adding %s to %s\n", child->getName(), getName());
    children.push_back(child);
    child->setParent(this);

    // printf("Between ifs\n");
    if(((type == STARSYSTEM) && children.size()>1) || ((type != GALACTIC_CORE) && (type != STARSYSTEM))) {
        // printf("Setting loneStar of %s to false\n", (static_cast<StellarObject*>(homeSystem))->getName());
        homeSystem->setLoneStar(false);
    }

    // printf("After if\n");
}

void StellarObject::calculateTotalMass(){
    // ------------------------------------------ Can be improved by using recursion correctly ------------------------------------------
    totalMass = 0;
    for(StellarObject *child: children){
        child->calculateTotalMass();
        totalMass += child->getTotalMass();
    }
    totalMass += mass;
}

void StellarObject::calculateCentreOfMass(){
    // ------------------------------------------------------------------------ TODO ------------------------------------------------------------------------ correct?
    // Calculate centre of mass of current system
    PositionVector tempCentreOfMass = PositionVector();
    double totalMass = 0;
    for(StellarObject *child: children){
        child->calculateTotalMass();
        double childTotalMass = child->getTotalMass();
        child->calculateCentreOfMass();
        tempCentreOfMass += child->getCentreOfMass() * childTotalMass;
        totalMass += childTotalMass;
    }
    tempCentreOfMass += position * mass;
    totalMass += mass;
    tempCentreOfMass /= totalMass;

    // --------------------------------------------------------------------------- DAFUQ ---------------------------------------------------------------------------
    // This function does not do anything really
    // centreOfMass = tempCentreOfMass;
}

void StellarObject::setParent(StellarObject *parent){
    // printf("Set parent of %s\n", getName());
    this->parent = parent;
    // printf("Homesystem of %s is %p\n", parent->getName(), parent->getHomeSystem());
    if(type != STARSYSTEM){
        this->homeSystem = parent->getHomeSystem();
        // printf("Homesystem of %s is now %s with pointer %p\n", getName(), (static_cast<StellarObject*>(homeSystem))->getName(), homeSystem);
    }
    // printf("Parent of %s is now %s\n", name, parent->getName());
}

void StellarObject::setMass(double mass){
    this->mass = mass;
}

void StellarObject::setRadius(double radius){
    this->radius = radius;
}

void StellarObject::setMeanDistance(double meanDistance){
    this->meanDistance = meanDistance;
}

void StellarObject::setInclination(double inclination){
    this->inclination = inclination;
}

void StellarObject::setEccentricity(double eccentricity){
    this->eccentricity = eccentricity;
}

void StellarObject::setName(const char *name){
    // We first free the old string and then assign the new one, to not get memory leaks
    if(this->name != NULL){
        free(this->name);
    }
    std::string *temp = new std::string(name);
    this->name = temp;
}

void StellarObject::setColour(int colour){
    this->colour = colour;
}

void StellarObject::setHomeSystem(StarSystem *homeSystem){
    this->homeSystem = homeSystem;
}

void StellarObject::setLocalAcceleration(PositionVector localAcceleration){
    this->localAcceleration = localAcceleration;
}

double StellarObject::getX(){
    return position.getX();
}

double StellarObject::getY(){
    return position.getY();
}

double StellarObject::getZ(){
    return position.getZ();
}

PositionVector StellarObject::getPosition(){
    return position;
}

PositionVector StellarObject::getVelocity(){
    return velocity;
}

PositionVector StellarObject::getLocalAcceleration(){
    return localAcceleration;
}

PositionVector StellarObject::getCentreOfMass(){
    return centreOfMass;
}

PositionVector StellarObject::getUpdatedCentreOfMass(){
    updateCentreOfMass();
    return centreOfMass;
}

double StellarObject::getRadius(){
    return radius;
}

double StellarObject::getMass(){
    return mass;
}

double StellarObject::getTotalMass(){
    return totalMass;
}

double StellarObject::getEccentricity(){
    return eccentricity;
}

double StellarObject::getMeanDistance(){
    return meanDistance;
}

StellarObject *StellarObject::getParent(){
    return parent;
}

const char *StellarObject::getName(){
    return name->c_str();
}

std::vector<StellarObject*> *StellarObject::getChildren(){
    return &children;
}

int StellarObject::getType(){
    return type;
}

int StellarObject::getColour(){
    return colour;
}

StarSystem *StellarObject::getHomeSystem(){
    return homeSystem;
}

void StellarObject::freeObject(){
    for(StellarObject *child: children){
        child->freeObject();
        delete child;
    }
    if(name != NULL){
        delete name;
    }
}

void StellarObject::updateCentreOfMass(){
    double totalMass = mass;
    PositionVector adjustedCoM = position * mass;
    for(StellarObject *child: children){
        totalMass += child->getTotalMass();
        child->updateCentreOfMass();
        adjustedCoM += child->getCentreOfMass() * child->getTotalMass();
    }
    centreOfMass = adjustedCoM / totalMass;
}