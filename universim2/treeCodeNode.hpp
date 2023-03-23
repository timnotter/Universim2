#ifndef TREE_CODE_NODE
#define TREE_CODE_NODE
#include <vector>
#include "stellarObject.hpp"
#include "positionVector.hpp"
#include "matrix3d.hpp"

#define MAX_ANGLE 0.1       // Theta = maximum angle, such that node is used for gravitational computation
// #define EPSILON 0.0001      // Gravitational Softening
#define EPSILON 0.0000000000001      // Gravitational Softening

class StellarObject;
class Tree;

class TreeCodeNode{
private:
    double mass;
    // Position of center of mass
    PositionVector centreOfMass;
    // Quadropole
    Matrix3d quadropole;
    // Position of "minimal" corner of node box
    PositionVector minCornerPosition;
    // Vector of all 8 corners
    std::vector<PositionVector> corners;
    // Size of node box
    double length;
    double width;
    double height;
    // Indices of particles inside node, initialisation works by inputing a vector of Particles and read the indices from them
    std::vector<int> indices;
    // All particles that exist - probably not needed
    // // std::vector<StellarObject*> *objectsInTree;
    // ALl particles in current node
    std::vector<StellarObject*> objectsInNode;
    // Subnodes
    std::vector<TreeCodeNode*> subNodes;
    // Flag if node contains one object
    bool leaf;
    // Node id - currently not used
    int id;

    // ----------------------------------------------------- TODO -----------------------------------------------------
    // Adjust these functions 
public:
    // TreeCodeNode(double x, double y, double z, double length, double width, double height);
    TreeCodeNode(std::vector<StellarObject*> *objectsInNode);


    // void initialiseNode(std::vector<Particle> *particles, std::vector<Particle> *indices, double x, double y, double z, double length, double width, double height);
    // void initialiseNode(std::vector<Particle> *particles, double x, double y, double z, double length, double width, double height);
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
    double calculateAngle(StellarObject *stellarObject);
    void updateCorners();
    // For debugging purposes
    int countNodes();
    PositionVector getCentreOfMassPosition();
};

#endif