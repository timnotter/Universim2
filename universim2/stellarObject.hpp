#ifndef STELLAROBJECT_HPP
#define STELLAROBJECT_HPP

#define GALACTIC_CORE 0
#define STARSYSTEM 1
#define STAR 2
#define PLANET 3
#define MOON 4
#define COMET 5
#define ASTEROID 6

#include <vector>
#include <string>
#include "positionVector.hpp"
#include "stellarObjectRenderFace.hpp"
#include "simplexNoise.hpp"

class StarSystem;
class Tree;

class StellarObject{
private:
    // const char *name;
    std::string *name;
    PositionVector position;
    // Position of centre of mass of system - currently not used
    PositionVector centreOfMass;
    PositionVector velocity;
    // Acceleration from different starsystems: reused for many times because of little change in them
    PositionVector stellarAcceleration;
    // Acceleration from bodies in own system and other near body: recalculated almost every tick
    PositionVector localAcceleration;
    // Counts the number of ticks that happened since the last calculation of the localAcceleration
    long localAccelerationLifeTime;
    // Stores the current maximum lifeTime of the localAcceleration. After this, the value definitely has to be recalculated
    long localAccelerationMaxLifeTime;
    // Mass of object - in case of starsystem mass of all stars combined
    long double mass;
    // Total mass contained in respective system
    long double totalMass;
    // Radius of object
    long double radius;
    // Initial mean distance of orbit around center of mass of parent, parent itself may be a bit further/closer. Used for initialisation
    long double meanDistance;
    // Eccentricity at start
    long double eccentricity;
    // Inclination at start - currently always in relation to plane z=0
    long double inclination;
    // Gets periodically updated to account for shifting - needed to store one whole orbit in lastPos array - in terran years - currently not in use
    long double period;
    // Determines type of body: 0: Galactic Center, 1: Starsystem, 2: Star, 3: Planet, 4: Moon/Asteroid, 5: Comet
    int type;
    // Colour of body
    int colour;
    // Determines wether object shines light or is only reflective
    bool isShining;
    // Pointer to parent object
    StellarObject *parent;
    // Pointer to home star system - Null in case of object being a galactic center
    StarSystem *homeSystem;
    // Vector with all children
    std::vector<StellarObject*> children;

    // Renderfaces
    StellarObjectRenderFace renderFaces[6];

    // For stellar gravitational computations
    PositionVector oldPosition;
    PositionVector oldVelocity;
    PositionVector oldStellarAcceleration;
    PositionVector futurePosition;
    PositionVector futureVelocity;
    PositionVector futureStellarAcceleration;

    // For renderer: before drawing he updates the current position, such that the picture drawn is from one single point in time
    PositionVector positionAtPointInTime;

    // Noise for the renderer
    SimplexNoise *surfaceNoise;
    // Random numbers for noise generation
    PositionVector randomVector;
    // Stores the form, meaning the relative length of each axis of the body
    PositionVector form;


public:
    // Initialisation units are all relativ to reference units: they are converted in constructor
    StellarObject(const char *name, int type, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination);
    StellarObject(const char *name, int type, long double radius, long double mass, long double meanDistance, long double eccentricity, long double inclination, int colour);

    void place();
    void updatePosition(long double deltaT);
    void updateVelocity(long double deltaT);
    void updateFuturePosition(long double deltaT);
    void updateFutureVelocity(long double deltaT);
    void updateStellarAcceleration(Tree *tree);
    void updateLocalAcceleration(Tree *tree);
    void updateNewStellarValues();
    void updateStellarAccelerationMaxLifeTime();
    void addChild(StellarObject *child);
    void calculateTotalMass();
    void calculateCentreOfMass();

    void setParent(StellarObject *parent);
    void setMass(long double mass);
    void setRadius(long double radius);
    void setMeanDistance(long double meanDistance);
    void setInclination(long double inclination);
    void setEccentricity(long double eccentricity);
    void setName(const char *name);
    void setColour(int colour);
    void setHomeSystem(StarSystem *homeSystem);
    void setLocalAcceleration(PositionVector localAcceleration);
    void setStellarAcceleration(PositionVector stellarAcceleration);
    void setFutureStellarAcceleration(PositionVector futureStellarAcceleration);
    void setFutureVelocity(PositionVector futureVelocity);
    void setFuturePosition(PositionVector futurePosition);
    void setOldStellarAcceleration(PositionVector oldStellarAcceleration);
    void setOldVelocity(PositionVector oldVelocity);
    void setOldPosition(PositionVector oldPosition);
    void setForm(PositionVector form);

    // Getter
    long double getX();
    long double getY();
    long double getZ();
    PositionVector getPosition();
    PositionVector getVelocity();
    PositionVector getLocalAcceleration();
    PositionVector getCentreOfMass();
    PositionVector getUpdatedCentreOfMass();
    long double getRadius();
    long double getMass();
    long double getTotalMass();
    long double getEccentricity();
    long double getMeanDistance();
    StellarObject *getParent();
    const char *getName();
    std::vector<StellarObject*> *getChildren();
    int getType();
    int getColour();
    StarSystem *getHomeSystem();
    PositionVector getFutureStellarAcceleration();
    PositionVector getFutureVelocity();
    PositionVector getFuturePosition();
    PositionVector getOldStellarAcceleration();
    PositionVector getOldVelocity();
    PositionVector getOldPosition();
    StellarObjectRenderFace *getRenderFaces();
    PositionVector getPositionAtPointInTime();
    SimplexNoise *getSurfaceNoise();
    PositionVector getRandomVector();
    long getLocalAccelerationMaxLifeTime();
    PositionVector getForm();
    
    void freeObject();
    void updateCentreOfMass();
    void updatePositionAtPointInTime();

    // Make object virtual, needed for dynamic casts into subclasses
    virtual ~StellarObject() = default;
};

#endif