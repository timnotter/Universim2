#include <stdio.h>
#include <string>
#include <thread>
#include "stellarObject.hpp"
#include "starSystem.hpp"
#include "constants.hpp"
#include "tree.hpp"
#include "timer.hpp"

StellarObject::StellarObject(const char *name, int type, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination){
    // this->name = name;
    this->name = new std::string(name);
    this->type = type;
    this->radius = radius;
    this->mass = mass;
    this->meanDistance = meanDistance;
    this->eccentricity = eccentricity;
    this->inclination = inclination;
    localAccelerationLifeTime = 0;
    // Initialised as -1, such that in the first iteration the localAccelerationLifeTime is smaller than the maxLifeTime
    localAccelerationMaxLifeTime = -1;
    isShining = false;
    
    for(int i=0;i<6;i++){
        renderFaces[i].initialise(this);
    }

    oldStellarAcceleration = PositionVector();

    // Correct all units in relation to respective reference units and create noise
    switch(type){
        case GALACTIC_CORE:
            this->radius *= sagittariusRadius;
            this->mass *= sagittariusMass;
            this->meanDistance *= lightyear;
            homeSystem = NULL;
            isShining = true;
            surfaceNoise = new SimplexNoise(0, 1, 2, 0.5);
            break;
        case STARSYSTEM:
            // If object is a star system, initialise homeSystem pointer to itself, such that children then can reuse it
            homeSystem = static_cast<StarSystem*>(this);
            // printf("Home system of %s has been set to %p\n", getName(), homeSystem);
            this->meanDistance *= distanceSagittariusSun;
            surfaceNoise = new SimplexNoise(1, 1, 2, 0.5);
            break;
        case STAR:
            this->radius *= solarRadius;
            this->mass *= solarMass;
            this->meanDistance *= astronomicalUnit;
            isShining = true;
            surfaceNoise = new SimplexNoise(0, 1, 2, 0.5);
            break;
        case PLANET:
            this->radius *= terranRadius;
            this->mass *= terranMass;
            this->meanDistance *= astronomicalUnit;
            surfaceNoise = new SimplexNoise(1, 1, 2, 0.5);
            break;
        case MOON:
            this->radius *= lunarRadius;
            this->mass *= lunarMass;
            this->meanDistance *= distanceEarthMoon;
            surfaceNoise = new SimplexNoise(1, 1, 2, 0.5);
            break;
        case COMET:
            this->radius *= 1;
            this->mass *= 1;
            this->meanDistance *= astronomicalUnit;
            surfaceNoise = new SimplexNoise(2, 2, 2, 0.5);
            break;
    }
    
    randomVector = PositionVector((long double)rand() / (RAND_MAX/25), (long double)rand() / (RAND_MAX/25), (long double)rand() / (RAND_MAX/25));
}

StellarObject::StellarObject(const char *name, int type, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination, int colour) : StellarObject(name, type, radius, mass, meanDistance, eccentricity, inclination){
    this->colour = colour;
}

void StellarObject::place(){
    // ------------------------------------------------------------------------ TODO ------------------------------------------------------------------------
    // First go through all children and find total mass
    calculateTotalMass();

    // Initialise position with correct orbit in mind

    // Initialise random object with current time
    struct timespec currTime;
    getTime(&currTime, 0);
    srand(currTime.tv_nsec);
    // printf("Current time: %ld + %ld\n", currTime.tv_sec, currTime.tv_nsec);
    // printf("3 random Numbers: %Lf, %Lf, %Lf\n", (long double)rand() / RAND_MAX * (2*PI), (long double)rand() / RAND_MAX * (2*PI), (long double)rand() / RAND_MAX * (2*PI));
    
    // First place center of mass of system at appropriate place, then place children at appropriate places,
    // lastly, adjust position of object, such that center of mass is correct
    
    // To start, we consider a central plane on which all orbits lay, when eccentricity is 0, namely the plane z = 0

    long double apoapsis;
    long double periapsis;
    long double distance;
    long double semiMajorAxis;
    long double trueAnomaly;
    long double argumentOfPeriapsis;
    long double longitudeOfAscendingNode;
    long double eccentricAnomaly;
    long double meanAnomaly;

    // If object is galactic core, place it anywhere with distance to (0/0/0) = meanDistance
    // printf("Trying to place %s\n", getName());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if(type == GALACTIC_CORE){
        tryAgain:
        // printf("Trying...\n");
        centreOfMass = PositionVector();
        position = PositionVector();
        velocity = PositionVector();
        if(meanDistance != 0){
            // ------------------------------------------------------------------------ TODO ------------------------------------------------------------------------
            // Place different galactic core at appropriate position, for example: take two random angles to take a position of the border of ball with radius meanDistance
            centreOfMass.setX(meanDistance);
            position = centreOfMass;
        }
        // printf("Centre of mass initialised to: (%s)\n", centreOfMass.toString());

        PositionVector tempCentreOfMass = PositionVector();
        PositionVector childrenMomentum = PositionVector();
        long double childrenTotalMass = 0;
        // printf("Trying to place children of %s\n", name);
        for(StellarObject *child: children){
            child->place();
            // printf("%s - position: (%s), velocity: (%s)\n", child->getName(), child->getPosition().toString(), child->getVelocity().toString());
            child->calculateTotalMass();
            long double childTotalMass = child->getTotalMass();
            tempCentreOfMass += child->getCentreOfMass() * childTotalMass;
            childrenMomentum += (child->getVelocity()) * childTotalMass;
            childrenTotalMass += childTotalMass;
        }
        if(childrenTotalMass!=0) tempCentreOfMass /= childrenTotalMass;
        calculateTotalMass();
        // printf("Centre of mass of children of %s is (%s). ChildrenTotalMass: %Lf, Mass: %Lf\n", getName(), tempCentreOfMass.toString(), childrenTotalMass, mass);
        position.setX((centreOfMass.getX() * totalMass - tempCentreOfMass.getX() * childrenTotalMass) / mass);
        position.setY((centreOfMass.getY() * totalMass - tempCentreOfMass.getY() * childrenTotalMass) / mass);
        position.setZ((centreOfMass.getZ() * totalMass - tempCentreOfMass.getZ() * childrenTotalMass) / mass);
        // printf("%s - position: (%s), velocity: (%s)\n", getName(), getPosition().toString(), getVelocity().toString());
        // printf("(%Lf - %Lf) / %Lf = %Lf\n", centreOfMass.getX() * totalMass, tempCentreOfMass.getX() * childrenTotalMass, mass, position.getX());
        // printf("Position: (%s)\n", position.toString());
        // printf("Calculated CoM: (%s)\n", ((position * mass + tempCentreOfMass * childrenTotalMass)/totalMass).toString());
        // printf("CoM: (%s)\n", getUpdatedCentreOfMass().toString());
        if(children.size()!=0){
            PositionVector before = velocity;
            // velocity += (childrenMomentum * -(1/mass)) - ((position / position.getLength()) * (G * totalMass / position.getLength()));
            // velocity += (childrenMomentum * -(1/mass));              // Changed to the following statement
            velocity = (childrenMomentum * -1 / mass);
            // velocity += (children.at(0)->getVelocity() * children.at(0)->getTotalMass() * (-1))/mass;
            // printf("childrenMomentum * -(1/mass): (%s), adjustement: (%s)\n", (childrenMomentum * -(1/mass)).toString(), ((position / position.getLength()) * (G * totalMass / position.getLength())).toString());
            // printf("Adjusted velocity of %s by (%s)\n", getName(), (velocity-before).toString());
        }
        PositionVector localMomentum = velocity * mass;
        printf("TotalMomentum: (%s), childrenMomentum: (%s), localMomentum: (%s)\n", (childrenMomentum + localMomentum).toString(), childrenMomentum.toString(), localMomentum.toString());
        if((childrenMomentum + localMomentum) != PositionVector()){
            printf("Total momentum is not 0, trying again. Momentum is: (%s)\n",(childrenMomentum + velocity * mass).toString());
            goto tryAgain;
        }
    }
    else{
        // Calculation of semi major axis
        semiMajorAxis = meanDistance / (1 - std::pow(eccentricity, 2));

        // We generate random angles for true anomaly, argument of periapsis and longitude of ascending node
        trueAnomaly = (long double)rand() / RAND_MAX * (2*PI);
        argumentOfPeriapsis = (long double)rand() / RAND_MAX * (2*PI);
        longitudeOfAscendingNode = (long double)rand() / RAND_MAX * (2*PI);

        // Calculation of eccentric anomaly
        eccentricAnomaly = 2 * std::atan(std::sqrt((1 - eccentricity)/(1 + eccentricity)) * std::tan(trueAnomaly / 2));

        // Calculating of true anomaly
        meanAnomaly = eccentricAnomaly - eccentricity * std::sin(eccentricAnomaly);

        // Initialise position with position of parent
        centreOfMass = parent->getCentreOfMass();

        // Calculate position from values
        long double factor = semiMajorAxis * (1 - std::pow(eccentricity, 2)) / (1 + eccentricity * std::cos(trueAnomaly));
        centreOfMass.setX(centreOfMass.getX() + factor * (std::cos(trueAnomaly + argumentOfPeriapsis) * std::cos(longitudeOfAscendingNode) - std::sin(trueAnomaly + argumentOfPeriapsis) * std::sin(longitudeOfAscendingNode) * std::cos(inclination)));
        centreOfMass.setY(centreOfMass.getY() + factor * (std::cos(trueAnomaly + argumentOfPeriapsis) * std::sin(longitudeOfAscendingNode) + std::sin(trueAnomaly + argumentOfPeriapsis) * std::cos(longitudeOfAscendingNode) * std::cos(inclination)));
        centreOfMass.setZ(centreOfMass.getZ() + factor * (std::sin(trueAnomaly + argumentOfPeriapsis) * std::sin(inclination)));

        // -------------------------------------------------------------------------- TODO --------------------------------------------------------------------------
        // Compute speed around centre of mass of system after children are placed

        // Speed should then be: 
        // v = sqrt(mu / p) * [-sin(v + w) * cos(O) - cos(v + w) * sin(O) * cos(i), -sin(v + w) * sin(O) + cos(v + w) * cos(O) * cos(i), cos(v + w) * sin(i)]
        if(semiMajorAxis!=0){
            long double mu = G * parent->getMass();
            long double p = semiMajorAxis * (1 - std::pow(eccentricity, 2));

            velocity.setX(std::sqrt(mu / p) * (-1 * std::sin(trueAnomaly + argumentOfPeriapsis) * std::cos(longitudeOfAscendingNode) - std::cos(trueAnomaly + argumentOfPeriapsis) * std::sin(longitudeOfAscendingNode) * std::cos(inclination)));
            velocity.setY(std::sqrt(mu / p) * (-1 * std::sin(trueAnomaly + argumentOfPeriapsis) * std::sin(longitudeOfAscendingNode) + std::cos(trueAnomaly + argumentOfPeriapsis) * std::cos(longitudeOfAscendingNode) * std::cos(inclination)));
            velocity.setZ(std::sqrt(mu / p) * (std::cos(trueAnomaly + argumentOfPeriapsis) * std::sin(inclination)));
        }
        // printf("Velocity of parent of %s is (%f, %f, %f)\n", getName(), parent->getVelocity().getX(), parent->getVelocity().getY(), parent->getVelocity().getZ());
        velocity += parent->getVelocity();

        // -------------------------------------------------------------------------- TODO --------------------------------------------------------------------
        // Go through all children and place their center of mass, calculate the center of mass of children
        PositionVector tempCentreOfMass = PositionVector();
        PositionVector childrenMomentum = PositionVector();
        long double childrenTotalMass = 0;
        // printf("Trying to place children of %s\n", name);
        for(StellarObject *child: children){
            child->place();
            child->calculateTotalMass();
            long double childTotalMass = child->getTotalMass();
            // printf("Total mass of system %s is %f\n", child->getName(), child->getTotalMass());
            calculateCentreOfMass();
            tempCentreOfMass += child->getCentreOfMass() * childTotalMass;
            childrenMomentum += (child->getVelocity()-velocity) * childTotalMass;
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

        // Finally we can adjust the velocity, such that the body orbits around the centre of mass of the system - Not sure if this is in fact correct
        // PositionVector before = velocity;
        if(children.size()!=0 && type!=STARSYSTEM){
            velocity += (childrenMomentum * -(1/mass)) - ((position / position.getLength()) * (G * totalMass / position.getLength()));
            // printf("Adjusted velocity of %s by (%s)\n", getName(), (velocity-before).toString());
        }
        setOldPosition(position);
        setOldVelocity(velocity);
        setFuturePosition(position);
        if(type == STARSYSTEM){
            mass = totalMass;
        }
    }
}

void StellarObject::updatePosition(long double deltaT){
    // std::string *nameComparison = new std::string("Sun");
    // std::string *nameComparison2 = new std::string("Sagittarius A*");
    // if(strcmp(getName(), nameComparison->c_str()) == 0 || strcmp(getName(), nameComparison2->c_str()) == 0){
    // printf("%s - old position: (%s), ", getName(), position.toString());
    // }
    position += velocity * deltaT;
    // std::string *nameComparison = new std::string("Sun");
    // if(strcmp(getName(), nameComparison->c_str()) == 0){
    //     printf("%s - Updated position with old position (?) and velocity (%s) to new position (%s)\n", getName(), velocity.toString(), position.toString());
    // }
    // delete nameComparison;
}

void StellarObject::updateVelocity(long double deltaT){
    velocity += (stellarAcceleration + localAcceleration) * deltaT;
    // std::string *nameComparison = new std::string("Sun");
    // std::string *nameComparison2 = new std::string("Sagittarius A*");
    // if(strcmp(getName(), nameComparison->c_str()) == 0 || strcmp(getName(), nameComparison2->c_str()) == 0){
    //     printf("%s - stellarAcceleration: (%s), localAcceleration: (%s), new velocity: (%s)\n", getName(), stellarAcceleration.toString(), localAcceleration.toString(), velocity.toString());
    // }
    // delete nameComparison;
}

void StellarObject::updateFuturePosition(long double deltaT){
    futurePosition = oldPosition + futureVelocity * deltaT;
    // std::string *nameComparison = new std::string("Solar System");
    // if(strcmp(getName(), nameComparison->c_str()) == 0){
    //     printf("%s - Updated future position with old position (%s) and velocity (%s) to new position (%s)\n", getName(), oldPosition.toString(), futureVelocity.toString(), futurePosition.toString());
    // }
    // delete nameComparison;
}

void StellarObject::updateFutureVelocity(long double deltaT){
    futureVelocity = oldVelocity + futureStellarAcceleration * deltaT; 
}

void StellarObject::updateStellarAcceleration(Tree *tree){
    // Calculate acceleration from star(-system) to other star(-system)s. If type is not starsystem, reuse acceleration from motherstar
    futureStellarAcceleration = tree->getRoot()->calculateAcceleration(this);
}

void StellarObject::updateLocalAcceleration(Tree *tree){
    // If the maxLifeTime is smaller than the current lifeTime, we recalculate
    if(localAccelerationMaxLifeTime <= localAccelerationLifeTime){
        // Calculate acceleration from local stellar objects - within system or nearby
        localAcceleration = tree->getRoot()->calculateAcceleration(this);
        localAccelerationLifeTime = 0;
        updateStellarAccelerationMaxLifeTime();
        if(children.size() >= 1){
            localAccelerationMaxLifeTime = std::min(localAccelerationMaxLifeTime, children[0]->getLocalAccelerationMaxLifeTime());
        }
    }
    localAccelerationLifeTime++;

    // Old simpler but more accurate way
    // localAcceleration = tree->getRoot()->calculateAcceleration(this);
}

void StellarObject::updateNewStellarValues(){
    // ------------------------------------------------------------------ TODO ------------------------------------------------------------------ Values don't coincide
    // Old position is only relevant for starsystems and galactic cores, where this makes sense
    oldPosition = getUpdatedCentreOfMass();
    oldVelocity = velocity;
    // std::string *nameComparison = new std::string("Solar System");
    // if(strcmp(getName(), nameComparison->c_str()) == 0){
    //     printf("%s - Approx pos: (%s), actual pos: (%s), difference: (%s)\n \t approx velocity: (%s), actual velocity: (%s), difference: (%s)\n", 
    //     getName(), oldPosition.toString(), futurePosition.toString(), (oldPosition-futurePosition).toString(), 
    //     velocity.toString(), futureVelocity.toString(), (velocity-futureVelocity).toString());
    // }
    // delete nameComparison;
    oldStellarAcceleration = futureStellarAcceleration;
    if(type == GALACTIC_CORE){
        stellarAcceleration = oldStellarAcceleration;
    }
    else{
        stellarAcceleration = homeSystem->getOldStellarAcceleration();
    }
    // if(type == STARSYSTEM){
    //     oldPosition = getUpdatedCentreOfMass();
    // }
}

void StellarObject::updateStellarAccelerationMaxLifeTime(){
    // Updates the localAccelerationMaxLifeTime by assuming a circular orbit with the current distance and speed 
    // and taking sqrt of the period of the orbit as the new maxLifeTime
    if(type == STAR){
        localAccelerationMaxLifeTime = 100;
        return;
    }
    long double approximatedPeriodInDays = 2 * (position.distance(parent->getPosition())) / (velocity.distance(parent->getVelocity())) / (60*60*24) * PI;
    // localAccelerationMaxLifeTime = std::ceil(std::sqrt(approximatedPeriodInDays));
    if(approximatedPeriodInDays<=1)
        localAccelerationMaxLifeTime = 1;
    else
        localAccelerationMaxLifeTime = std::ceil(approximatedPeriodInDays / (std::log2(approximatedPeriodInDays) + 1));
    // printf("%s: localAccelerationMaxLifeTime = %ld. Period: %Lf\n", getName(), localAccelerationMaxLifeTime, approximatedPeriodInDays);
}

void StellarObject::addChild(StellarObject *child){
    // printf("Adding %s to %s\n", child->getName(), getName());
    children.push_back(child);
    child->setParent(this);

    // printf("Between ifs\n");
    if(((type == STARSYSTEM) && children.size()>1) || ((type != GALACTIC_CORE) && (type != STARSYSTEM))) {
        // printf("Setting loneStar of %s to false\n", (static_cast<StellarObject*>(homeSystem))->getName());
        // printf("Set LoneStar = false for %s\n", homeSystem->getName());
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
    if(type == STARSYSTEM) mass = totalMass;
    else totalMass += mass;
    // printf("Total mass of %s is %Lf\n", getName(), totalMass);
}

void StellarObject::calculateCentreOfMass(){
    // ------------------------------------------------------------------------ TODO ------------------------------------------------------------------------ correct?
    // Calculate centre of mass of current system
    PositionVector tempCentreOfMass = PositionVector();
    long double totalMass = 0;
    for(StellarObject *child: children){
        child->calculateTotalMass();
        long double childTotalMass = child->getTotalMass();
        child->calculateCentreOfMass();
        tempCentreOfMass += child->getCentreOfMass() * childTotalMass;
        totalMass += childTotalMass;
    }
    if(type != STARSYSTEM){
        tempCentreOfMass += position * mass;
        totalMass += mass;
    }
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

void StellarObject::setMass(long double mass){
    this->mass = mass;
}

void StellarObject::setRadius(long double radius){
    this->radius = radius;
}

void StellarObject::setMeanDistance(long double meanDistance){
    this->meanDistance = meanDistance;
}

void StellarObject::setInclination(long double inclination){
    this->inclination = inclination;
}

void StellarObject::setEccentricity(long double eccentricity){
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

void StellarObject::setStellarAcceleration(PositionVector stellarAcceleration){
    this->stellarAcceleration = stellarAcceleration;
}

void StellarObject::setFutureStellarAcceleration(PositionVector futureStellarAcceleration){
    this->futureStellarAcceleration = futureStellarAcceleration;
}

void StellarObject::setFutureVelocity(PositionVector futureVelocity){
    this->futureVelocity = futureVelocity;
}

void StellarObject::setFuturePosition(PositionVector futurePosition){
    this->futurePosition = futurePosition;
}

void StellarObject::setOldStellarAcceleration(PositionVector oldStellarAcceleration){
    this->oldStellarAcceleration = oldStellarAcceleration;
}

void StellarObject::setOldVelocity(PositionVector oldVelocity){
    this->oldVelocity = oldVelocity;
}

void StellarObject::setOldPosition(PositionVector oldPosition){
    this->oldPosition = oldPosition;
}

long double StellarObject::getX(){
    return position.getX();
}

long double StellarObject::getY(){
    return position.getY();
}

long double StellarObject::getZ(){
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

long double StellarObject::getRadius(){
    return radius;
}

long double StellarObject::getMass(){
    return mass;
}

long double StellarObject::getTotalMass(){
    return totalMass;
}

long double StellarObject::getEccentricity(){
    return eccentricity;
}

long double StellarObject::getMeanDistance(){
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

PositionVector StellarObject::getFutureStellarAcceleration(){
    return futureStellarAcceleration;
}

PositionVector StellarObject::getFutureVelocity(){
    return futureVelocity;
}

PositionVector StellarObject::getFuturePosition(){
    return futurePosition;
}

PositionVector StellarObject::getOldStellarAcceleration(){
    return oldStellarAcceleration;
}

PositionVector StellarObject::getOldVelocity(){
    return oldVelocity;
}

PositionVector StellarObject::getOldPosition(){
    return oldPosition;
}

StellarObjectRenderFace *StellarObject::getRenderFaces(){
    return renderFaces;
}

PositionVector StellarObject::getPositionAtPointInTime(){
    return positionAtPointInTime;
}

SimplexNoise *StellarObject::getSurfaceNoise(){
    return surfaceNoise;
}

PositionVector StellarObject::getRandomVector(){
    return randomVector;
}

long StellarObject::getLocalAccelerationMaxLifeTime(){
    return localAccelerationMaxLifeTime;
}

void StellarObject::freeObject(){
    for(StellarObject *child: children){
        child->freeObject();
        delete child;
    }
    if(name != NULL){
        delete surfaceNoise;
        delete name;
    }
}

void StellarObject::updateCentreOfMass(){
    long double totalMass = 0;
    PositionVector adjustedCoM = PositionVector();
    if(type != STARSYSTEM){
        totalMass = mass;
        adjustedCoM = position * mass;
    }
    for(StellarObject *child: children){
        totalMass += child->getTotalMass();
        child->updateCentreOfMass();
        adjustedCoM += child->getCentreOfMass() * child->getTotalMass();
    }
    centreOfMass = adjustedCoM / totalMass;
}

void StellarObject::updatePositionAtPointInTime(){
    positionAtPointInTime = position;
}