#ifndef TREE_CODE_NODE
#define TREE_CODE_NODE
#include <vector>
#include "stellarObjects/stellarObject.hpp"
#include "helpers/positionVector.hpp"
#include "helpers/matrix3d.hpp"

#define MAX_ANGLE 0.5       // Theta = maximum angle, such that node is used for gravitational computation
// #define EPSILON 0.0001      // Gravitational Softening
#define EPSILON 0.0000000000001      // Gravitational Softening


#define STELLAR_TREE true
#define LOCAL_TREE false

class StellarObject;
class Tree;

class TreeCodeNode{
private:
    long double mass;
    // Position of center of mass
    PositionVector centreOfMass;
    // Quadropole
    Matrix3d quadropole;
    // Position of "minimal" corner of node box
    PositionVector minCornerPosition;
    // Vector of all 8 corners
    std::vector<PositionVector> corners;
    // Size of node box
    long double length;
    long double width;
    long double height;
    // All objects in current node
    std::vector<StellarObject*> objectsInNode;
    // Subnodes
    std::vector<TreeCodeNode*> subNodes;
    // Flag if node contains one object
    bool leaf;
    // Node id - currently not used
    int id;
    // Determines the type of the tree the node is part of
    bool type;

public:
    TreeCodeNode(std::vector<StellarObject*> *objectsInNode);

    void createSubNodes();
    // Calculate all needed values
    void calculateNodeValues();
    void calculateLeafValues();
    bool isLeaf();
    // Free the node and its children
    void freeNode();
    // ID currently not used
    int getID();
    PositionVector calculateAcceleration(StellarObject *stellarObject);
    PositionVector calculateStellarAcceleration(StellarObject *stellarObject);
    long double calculateAngle(StellarObject *stellarObject);
    void updateCorners();
    // For debugging purposes
    int countNodes();
    PositionVector getCentreOfMassPosition();

    PositionVector getRelevantPositionOfTreetype(StellarObject *stellarObject);
    long double getRelevantXOfTreetype(StellarObject *stellarObject);
    long double getRelevantYOfTreetype(StellarObject *stellarObject);
    long double getRelevantZOfTreetype(StellarObject *stellarObject);
};

#endif
