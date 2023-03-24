#ifndef STELLAROBJECT_HPP
#define STELLAROBJECT_HPP

#define GALACTIC_CORE 0
#define STARSYSTEM 1
#define STAR 2
#define PLANET 3
#define MOON 4
#define COMET 5

#include <vector>
#include <string>
#include "positionVector.hpp"
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
    // Acceleration from bodies in own system and other near body: recalculated every tick
    PositionVector localAcceleration;
    // Mass of object - in case of starsystem mass of all stars combined
    double mass;
    // Total mass contained in respective system
    double totalMass;
    // Radius of object
    double radius;
    // Initial mean distance of orbit around center of mass of parent, parent itself may be a bit further/closer. Used for initialisation
    double meanDistance;
    // Eccentricity at start
    double eccentricity;
    // Inclination at start - currently always in relation to plane z=0
    double inclination;
    // Gets periodically updated to account for shifting - needed to store one whole orbit in lastPos array - in terran years - currently not in use
    double period;
    // Determines type of body: 0: Galactic Center, 1: Starsystem, 2: Star, 3: Planet, 4: Moon/Asteroid, 5: Comet
    int type;
    // Colour of body
    int colour;
    // Pointer to parent object
    StellarObject *parent;
    // Pointer to home star system - Null in case of object being a galactic center
    StarSystem *homeSystem;
    // Vector with all children
    std::vector<StellarObject*> children;

public:
    // Initialisation units are all relativ to reference units: they are converted in constructor
    StellarObject(const char *name, int type, double radius, double mass, double meanDistance, double eccentricity, double inclination);
    StellarObject(const char *name, int type, double radius, double mass, double meanDistance, double eccentricity, double inclination, int colour);

    void place();
    void initialiseVelocity();
    void updatePosition(double deltaT);
    void updateVelocity(double deltaT);
    void updateStellarAcceleration(Tree *tree);
    void updateLocalAcceleration(Tree *tree);
    void addChild(StellarObject *child);
    void calculateTotalMass();
    void calculateCentreOfMass();

    void setParent(StellarObject *parent);
    void setMass(double mass);
    void setRadius(double radius);
    void setMeanDistance(double meanDistance);
    void setInclination(double inclination);
    void setEccentricity(double eccentricity);
    void setName(const char *name);
    void setColour(int colour);
    void setHomeSystem(StarSystem *homeSystem);
    void setLocalAcceleration(PositionVector localAcceleration);

    // Getter
    double getX();
    double getY();
    double getZ();
    PositionVector getPosition();
    PositionVector getVelocity();
    PositionVector getLocalAcceleration();
    PositionVector getCentreOfMass();
    PositionVector getUpdatedCentreOfMass();
    double getRadius();
    double getMass();
    double getTotalMass();
    double getEccentricity();
    double getMeanDistance();
    StellarObject *getParent();
    const char *getName();
    std::vector<StellarObject*> *getChildren();
    int getType();
    int getColour();
    StarSystem *getHomeSystem();

    void freeObject();
    void updateCentreOfMass();

    // Make object virtual, needed for dynamic casts into subclasses
    virtual ~StellarObject() = default;
};

#endif