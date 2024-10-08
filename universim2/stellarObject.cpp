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
    this->form = PositionVector(1, 1, 1);
    // HomeSystem is initialised as a nullptr. It has to be set by the addChild of the future parent
    this->homeSystem = nullptr;
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
        case ASTEROID:
            this->radius *= 1;
            this->mass *= 1;
            this->meanDistance *= 1;
            surfaceNoise = new SimplexNoise(2, 2, 2, 0.5);
            break;
    }
    
    randomVector = PositionVector((long double)rand() / (RAND_MAX/25), (long double)rand() / (RAND_MAX/25), (long double)rand() / (RAND_MAX/25));
}

StellarObject::~StellarObject(){
    for(StellarObject *child: children){
        delete child;
    }
    if(name != NULL){
        delete surfaceNoise;
        delete name;
    }
}

StellarObject::StellarObject(const char *name, int type, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination, int colour) : StellarObject(name, type, radius, mass, meanDistance, eccentricity, inclination){
    this->colour = colour;
}

void StellarObject::place(){
    // Initialise position with correct orbit in mind

    // Initialise random object with current time
    struct timespec currTime;
    getTime(&currTime, 0);
    srand(currTime.tv_nsec);
    // printf("Current time: %ld + %ld\n", currTime.tv_sec, currTime.tv_nsec);
    // printf("3 random Numbers: %Lf, %Lf, %Lf\n", (long double)rand() / RAND_MAX * (2*PI), (long double)rand() / RAND_MAX * (2*PI), (long double)rand() / RAND_MAX * (2*PI));
    
    // First place center of mass of system at appropriate place, then place children at appropriate places.
    // Lastly, adjust position of object, such that centre of mass stays at the initially decided place
    
    // To start, we consider a central plane on which all orbits lay when eccentricity is 0, namely the plane z = 0
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
    if(type == GALACTIC_CORE){
        // First go through all children and find total mass. This function recursively goes over every object in the "parent tree"
        calculateTotalMass();
        tryAgain:
        // printf("Trying...\n");
        centreOfMass = PositionVector();
        position = PositionVector();
        velocity = PositionVector();
        if(meanDistance != 0){
            // ------------------------------------------------------------------------ TODO ------------------------------------------------------------------------
            // Place different galactic core at appropriate position, for example: take two random angles to take a position of the border of ball with radius meanDistance
            centreOfMass.setX(meanDistance);
            // This assignement ought to be meaningless, since we adjust the position after placing the children. Position of parent ought not to be used
            position = centreOfMass;
        }
        // printf("Centre of mass initialised to: (%s)\n", centreOfMass.toString());

        // printf("Trying to place children of %s\n", name);

        // We multithread children placement. However, if MULTITHREAD_PLACEMENT is set to 0, the placement will stay singlethreaded
        startPlacementOfChildren(&children, MULTITHREAD_PLACEMENT_MAX_THREAD_COUNT);

        if(children.size()!=0){
            // Here we ensure that the centre of mass of the system is where it was initially placed. 
            // Furthermore, we ensure that the momentum in the system is 0
            PositionVector before = velocity;

            PositionVector childrenMomentum = PositionVector();
            PositionVector childrenCentreOfMass = PositionVector();
            long double childrenTotalMass = totalMass - mass;
            for(StellarObject *child: children){
                long double childTotalMass = child->getTotalMass();
                childrenMomentum += (child->getVelocity()-velocity) * childTotalMass;
                childrenCentreOfMass += child->getCentreOfMass() * childTotalMass;
            }
            childrenCentreOfMass /= childrenTotalMass;

            // printf("Centre of mass of children of %s is (%s). ChildrenTotalMass: %Lf, Mass: %Lf\n", getName(), tempCentreOfMass.toString(), childrenTotalMass, mass);
            position.setX((centreOfMass.getX() * totalMass - childrenCentreOfMass.getX() * childrenTotalMass) / mass);
            position.setY((centreOfMass.getY() * totalMass - childrenCentreOfMass.getY() * childrenTotalMass) / mass);
            position.setZ((centreOfMass.getZ() * totalMass - childrenCentreOfMass.getZ() * childrenTotalMass) / mass);

            // This equation has been used before. I am not sure were the last term comes from, thus it was omitted 
            // velocity += (childrenMomentum * -(1/mass)) - ((position / position.getLength()) * (G * totalMass / position.getLength()));
            
            velocity += (childrenMomentum * -1 / mass);
            
            // printf("childrenMomentum * -(1/mass): (%s), adjustement: (%s)\n", (childrenMomentum * -(1/mass)).toString(), ((position / position.getLength()) * (G * totalMass / position.getLength())).toString());
            // printf("Adjusted velocity of %s by (%s)\n", getName(), (velocity-before).toString());


            // Here we check if the total momentum is 0
            // If this is the case, we have numerical instability. This happens because we divide childrenMomentum by mass to calculate
            // the velocity of the parent, but to check we multiply with mass again.
            // This should (hopefully) have no impact on the correctness of the system, and if it has, we cannot really alleviate it, without 
            // increasing precision of used datatype further, which we do not want to.
            // I don't really know if for the real momentum in the system to be 0, the totalMomentum variable has to be 0 here, or if
            // the imprecision just outs itself in this variable
            // I tend to the latter, because the line "velocity += (childrenMomentum * -1 / mass);" seems correct. Thus we omit the following code

            // PositionVector localMomentum = velocity * mass;
            // PositionVector totalMomentum = childrenMomentum + localMomentum;
            // // printf("TotalMomentum: (%s),\nchildrenMomentum: (%s),\nlocalMomentum: (%s)\n", (childrenMomentum + localMomentum).toString(), childrenMomentum.toString(), localMomentum.toString());
            // printf("TotalMomentum: (%Lf, %Lf, %Lf),\nchildrenMomentum: (%Lf, %Lf, %Lf),\nlocalMomentum: (%Lf, %Lf, %Lf)\n", 
            // totalMomentum.getX(), totalMomentum.getY(), totalMomentum.getZ(),
            // childrenMomentum.getX(), childrenMomentum.getY(), childrenMomentum.getZ(),
            // localMomentum.getX(), localMomentum.getY(), localMomentum.getZ());

            // if((childrenMomentum + localMomentum) != PositionVector()){
            //     printf("Total momentum is not 0, trying again. Momentum is: (%s)\n",(childrenMomentum + velocity * mass).toString());
            //     PositionVector temp1 = childrenMomentum * -1;
            //     PositionVector temp2 = (childrenMomentum * -1 / mass) * mass;
            //     printf("childrenMomentum clean: (%Lf, %Lf, %Lf),\nchildrenMomentum mult: (%Lf, %Lf, %Lf)\n",
            //     temp1.getX(), temp1.getY(), temp1.getZ(),
            //     temp2.getX(), temp2.getY(), temp2.getZ());
            //     goto tryAgain;
            // }
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

        // Initialise centreOfMass with that of parent
        centreOfMass = parent->getCentreOfMass();

        // Calculate position from orbital paramteres and store it into centreOfMass
        long double factor = semiMajorAxis * (1 - std::pow(eccentricity, 2)) / (1 + eccentricity * std::cos(trueAnomaly));
        centreOfMass.setX(centreOfMass.getX() + factor * (std::cos(trueAnomaly + argumentOfPeriapsis) * std::cos(longitudeOfAscendingNode) - std::sin(trueAnomaly + argumentOfPeriapsis) * std::sin(longitudeOfAscendingNode) * std::cos(inclination)));
        centreOfMass.setY(centreOfMass.getY() + factor * (std::cos(trueAnomaly + argumentOfPeriapsis) * std::sin(longitudeOfAscendingNode) + std::sin(trueAnomaly + argumentOfPeriapsis) * std::cos(longitudeOfAscendingNode) * std::cos(inclination)));
        centreOfMass.setZ(centreOfMass.getZ() + factor * (std::sin(trueAnomaly + argumentOfPeriapsis) * std::sin(inclination)));

        // Here we compute the theoretical speed the object with its trabants has to have with these orbital parameters
        // We adjust the speed after the children are placed, such that the object orbits the new centre of mass

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
        
        // We add the velocity of the parent (which is at this stage effectively the velocity of the CoM, not of the parent itself)
        // to this object. The parents velocity gets adjusted after placing it further below. This is to ensure that total momentum is 0
        // This adjustement, however, has no impact on this object, which means this adjustment here is sound
        velocity += parent->getVelocity();

        // We multithread children placement. However, if MULTITHREAD_PLACEMENT is set to 0, the placement will stay singlethreaded
        startPlacementOfChildren(&children, MULTITHREAD_PLACEMENT_MAX_THREAD_COUNT);

        // Finally we can adjust the velocity, such that this object orbits around the centre of mass of the system
        // PositionVector before = velocity;
        // We adjust the velocity such that total momentum of the system is 0
        if(children.size()!=0 && type!=STARSYSTEM){
            // We calculate the momentum of all children and their CoM 
            PositionVector childrenMomentum = PositionVector();
            PositionVector childrenCentreOfMass = PositionVector();
            long double childrenTotalMass = totalMass - mass;
            for(StellarObject *child: children){
                long double childTotalMass = child->getTotalMass();
                childrenMomentum += (child->getVelocity()-velocity) * childTotalMass;
                childrenCentreOfMass += child->getCentreOfMass() * childTotalMass;
            }
            childrenCentreOfMass /= childrenTotalMass;

            // We place the object such that the centre of mass stays where we placed it at the beginning 
            position.setX((centreOfMass.getX() * totalMass - childrenCentreOfMass.getX() * childrenTotalMass) / mass);
            position.setY((centreOfMass.getY() * totalMass - childrenCentreOfMass.getY() * childrenTotalMass) / mass);
            position.setZ((centreOfMass.getZ() * totalMass - childrenCentreOfMass.getZ() * childrenTotalMass) / mass);

            // if(getType() == PLANET){
            //     printf("%s - CoM: (%Lf, %Lf, %Lf), CCoM: (%Lf, %Lf, %Lf), Pos: (%Lf, %Lf, %Lf)\n", getName(), centreOfMass.getX(), centreOfMass.getY(), centreOfMass.getZ(), childrenCentreOfMass.getX(), childrenCentreOfMass.getY(), childrenCentreOfMass.getZ(), position.getX(), position.getY(), position.getZ());
            // }

            // I don't know where the second term came from
            // velocity += (childrenMomentum * -(1/mass)) - ((position / position.getLength()) * (G * totalMass / position.getLength()));
            velocity += (childrenMomentum * -1 / mass);
            // printf("Adjusted velocity of %s by (%s)\n", getName(), (velocity-before).toString());
        }
        else{
            // A starsystem or an object without children is always placed exactly where the CoM is
             position.setX(centreOfMass.getX());
             position.setY(centreOfMass.getY());
             position.setZ(centreOfMass.getZ());
        }

        // if(getType() == PLANET){
        //     printf("Placed %s at (%Lf, %Lf, %Lf) with velocity (%Lf, %Lf, %Lf)\n", getName(), position.getX(), position.getY(), position.getZ(), velocity.getX(), velocity.getY(), velocity.getZ());
        // }

        // These are values used by the force calculations
        setOldPosition(position);
        setOldVelocity(velocity);
        setFuturePosition(position);
    }
}

void StellarObject::updatePosition(long double deltaT){
    position += velocity * deltaT;
}

void StellarObject::updateVelocity(long double deltaT){
    velocity += (stellarAcceleration + localAcceleration) * deltaT;
}

void StellarObject::updateFuturePosition(long double deltaT){
    futurePosition = oldPosition + futureVelocity * deltaT;
}

void StellarObject::updateFutureVelocity(long double deltaT){
    futureVelocity = oldVelocity + futureStellarAcceleration * deltaT; 
}

void StellarObject::updateStellarAcceleration(Tree *tree){
    // Calculate acceleration from star(-system) to other star(-system)s. If type is not starsystem, reuse acceleration from motherstar
    futureStellarAcceleration = tree->getRoot()->calculateAcceleration(this);
}

void StellarObject::updateLocalAcceleration(Tree *tree){
    // If the current local acceleration is older than it is allowed to be, we recalculate.
    // This means we can store accelerations for reuse, to trade accuracy for efficiency
    // This maxLifeTime is individual for every object and gets updated after every computation
    if(localAccelerationMaxLifeTime <= localAccelerationLifeTime){
        // Calculate acceleration from local stellar objects - within system or nearby
        localAcceleration = tree->getRoot()->calculateAcceleration(this);
        localAccelerationLifeTime = 0;
        updateLocalAccelerationMaxLifeTime();
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

void StellarObject::updateLocalAccelerationMaxLifeTime(){
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
    // printf("Added %s to %s\n", child->getName(), getName());

    // If this is a second trabant added to a starsystem or an object added to a star, we unset loneStar (activate gravity calculations)

    // if(((type == STARSYSTEM) && children.size()>1) || ((type != GALACTIC_CORE) && (type != STARSYSTEM))) {
    if(((type == STARSYSTEM) && children.size()>1) || (type == STAR && children.size()>=1)) {
        // printf("Setting loneStar of %s to false\n", (static_cast<StellarObject*>(homeSystem))->getName());
        // printf("Set LoneStar = false for %s\n", homeSystem->getName());
        homeSystem->setLoneStar(false);
    }

    // printf("After if\n");
}

void StellarObject::calculateTotalMass(){
    totalMass = 0;
    for(StellarObject *child: children){
        child->calculateTotalMass();
        totalMass += child->getTotalMass();
    }
    if(type == STARSYSTEM) mass = totalMass;
    else totalMass += mass;
    // printf("Total mass of %s is %Lf\n", getName(), totalMass);
}

void StellarObject::setParent(StellarObject *parent){
    // printf("Set parent of %s\n", getName());
    this->parent = parent;
    // printf("Homesystem of %s is %p\n", parent->getName(), parent->getHomeSystem());
    if(type != STARSYSTEM){
        // We have to further check if the parents homeSystem has been set yet. If it hasn't, i.e. it's still nullptr, we skip this step
        StellarObject* hS = parent->getHomeSystem();
        if(hS != nullptr){
            setHomeSystem(parent->getHomeSystem());
        }
        // else{
        //     printf("Found nullptr in homeSystem of %s\n", parent->getName());
        // }
        // ATTENTION!!! We cannot static cast homeSystem to StellarObject*, since it could also be null at this time!!
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
    // printf("Setting homesystem of %s to %s\n", getName(), homeSystem->getName());
    // Here we have to loop over all children and set their homeSystem to this aswell
    this->homeSystem = homeSystem;
    for(StellarObject* child: children){
        child->setHomeSystem(homeSystem);
    }
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

void StellarObject::setForm(PositionVector form){
    this->form = form;
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
    // Attention!!! This could potentially be null
    // printf("HomeSystem of %s is %p\n", getName(), homeSystem);
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

PositionVector StellarObject::getForm(){
    return form;
}

void StellarObject::updateCentreOfMass(){
    PositionVector adjustedCoM = PositionVector();
    if(type != STARSYSTEM){
        adjustedCoM = position * mass;
    }
    for(StellarObject *child: children){
        child->updateCentreOfMass();
        adjustedCoM += child->getCentreOfMass() * child->getTotalMass();
    }
    centreOfMass = adjustedCoM / getTotalMass();
}

void StellarObject::updatePositionAtPointInTime(){
    positionAtPointInTime = position;
}

void StellarObject::startPlacementOfChildren(std::vector<StellarObject*> *children, int maxThreadCount){
    // Multithreading this does not actually decrease the needed time, even for large amounts of children,
    // For 1'000'000 stars with a minimum of 100 objects per thread, the time
    // spent does neither decrease consistently nor considerably, which is quite curious


    struct timespec prevTime;
	struct timespec currTime;
	clock_gettime(CLOCK_MONOTONIC, &prevTime);

    // If we have only one thread available or equal or less than 100 children, we do it all in one thread
    if(maxThreadCount == 1 || children->size() <= 100 || MULTITHREAD_PLACEMENT == 0){
        for(StellarObject *child: (*children)){
            child->place();
        }
        if(children->size() > 100){
            clock_gettime(CLOCK_MONOTONIC, &currTime);
            printf("Placing all children of %s with one thread took: %ld mics\n", getName(), ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000));
        }
        return;
    }
    int threadsUsed;
    int amountOfChildren = children->size();
    // We want at least 100 children per thread with a min of 1 and a max of maxThreadCount
    threadsUsed = std::max(std::min(amountOfChildren / 100, maxThreadCount), 1);
    int childrenPerThread = amountOfChildren/threadsUsed;

    printf("%s uses %d threads for %ld children\n", getName(), threadsUsed, children->size());

    std::vector<std::thread> threads;
    for(int i=0;i<threadsUsed-1;i++){
        // printf("Starting thread %d with %d children from parent %s\n", i, childrenPerThread, getName());
        threads.push_back(std::thread(placeChildrenMultiThread, children, i*childrenPerThread, childrenPerThread));
    }
    // printf("Starting thread %d with %d children from parent %s\n", threadsUsed-1, amountOfChildren - ((threadsUsed-1)*childrenPerThread), getName());
    threads.push_back(std::thread(placeChildrenMultiThread, children, (threadsUsed-1)*childrenPerThread, amountOfChildren - ((threadsUsed-1)*childrenPerThread)));
    // printf("Started all threads\n");
    
    for(int i=threadsUsed-1;i>=0;i--){
        threads.at(i).join();
        // printf("Thread %d ended\n", i);
    }
    threads.clear();
    clock_gettime(CLOCK_MONOTONIC, &currTime);
    printf("Placing all children of %s with multiple threads took: %ld mics\n", getName(), ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000));
}

void placeChildrenMultiThread(std::vector<StellarObject*> *children, int start, int amount){
    struct timespec prevTime;
	struct timespec currTime;
	clock_gettime(CLOCK_MONOTONIC, &prevTime);

    std::thread::id this_id = std::this_thread::get_id();
    std::stringstream ss;
    ss << this_id;
    std::string thread_id_str = ss.str();

    for(int i=start;i<start+amount;i++){
        children->at(i)->place();
    }
    
    clock_gettime(CLOCK_MONOTONIC, &currTime);
    printf("Thread %s with %d children and start %d took: %ld mics\n", thread_id_str.c_str(), amount, start, ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000));

}