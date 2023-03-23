#include <cstdio>
#include <unistd.h>
#include <cmath>
#include "treeCodeNode.hpp"
#include "tree.hpp"
#include "constants.hpp"

// TreeCodeNode::TreeCodeNode(double x, double y, double z, double length, double width, double height){
//     minCornerPosition = PositionVector(x, y, z);
//     this->length = length;
//     this->width = width;
//     this->height = height;
//     leaf = false;
// }

TreeCodeNode::TreeCodeNode(std::vector<StellarObject*> *objectsInNode){
    minCornerPosition = PositionVector();
    leaf = false;
    for(StellarObject *stellarObject: *objectsInNode){
        this->objectsInNode.push_back(stellarObject);
    }
    calculateNodeValues();
    createSubNodes();
    // printf("Created node with %lu objects, position (%f, %f, %f), size (%f, %f, %f) and mass %f\n", objectsInNode->size(), centreOfMass.getX(), centreOfMass.getY(), centreOfMass.getZ(), length, width, height, mass);
}

void TreeCodeNode::createSubNodes(){
    if(objectsInNode.size()==1) {
        leaf = true;
        return;
    }
    // printf("Create subnodes with %lu objects\n", objectsInNode.size());
    // Create vectors to group objects into subnodes
    std::vector<std::vector<StellarObject*>> objectsInSubnodes;
    for(int i=0;i<8;i++){
        objectsInSubnodes.push_back(std::vector<StellarObject*>());
    }

    // Go through all objects and add them to corresponding subnode vector
    for(StellarObject *stellarObject: objectsInNode){
        bool xCondition = stellarObject->getX() < minCornerPosition.getX()+length/2;
        bool yCondition = stellarObject->getY() < minCornerPosition.getY()+width/2;
        bool zCondition = stellarObject->getZ() < minCornerPosition.getZ()+height/2;

        if(xCondition){
            if(yCondition){
                if(zCondition) objectsInSubnodes[0].push_back(stellarObject);
                else objectsInSubnodes[1].push_back(stellarObject);
            }
            else{
                if(zCondition) objectsInSubnodes[2].push_back(stellarObject);
                else objectsInSubnodes[3].push_back(stellarObject);
            }
        }
        else{
            if(yCondition){
                if(zCondition) objectsInSubnodes[4].push_back(stellarObject);
                else objectsInSubnodes[5].push_back(stellarObject);
            }
            else{
                if(zCondition) objectsInSubnodes[6].push_back(stellarObject);
                else objectsInSubnodes[7].push_back(stellarObject);
            }
        }
    }

    // Go through every vector and if there is at least one object, create the corresponding subnode
    for(int i=0;i<8;i++){
        if(objectsInSubnodes[i].size()!=0){
            subNodes.push_back(new TreeCodeNode(&(objectsInSubnodes[i])));
        }
    }
}

void TreeCodeNode::calculateNodeValues(){
    if(objectsInNode.size() == 1){
        calculateLeafValues();
        return;
    }

    double minX = DOUBLE_MAX_VALUE;
    double minY = DOUBLE_MAX_VALUE;
    double minZ = DOUBLE_MAX_VALUE;
    double maxX = DOUBLE_MIN_VALUE;
    double maxY = DOUBLE_MIN_VALUE;
    double maxZ = DOUBLE_MIN_VALUE;
    double tempMass;
    double tempX;
    double tempY;
    double tempZ;
    for(StellarObject *stellarObject: objectsInNode){
        tempX = stellarObject->getX();
        tempY = stellarObject->getY();
        tempZ = stellarObject->getZ();
        tempMass = stellarObject->getMass();
        // Update total mass of node
        mass += tempMass;
        // Check to update position and size of node
        minX = std::min(minX, tempX);
        minY = std::min(minY, tempY);
        minZ = std::min(minZ, tempZ);
        maxX = std::max(maxX, tempX);
        maxY = std::max(maxY, tempY);
        maxZ = std::max(maxZ, tempZ);
        // Update center of mass
        centreOfMass += PositionVector(tempX, tempY, tempZ) * tempMass;
    }
    centreOfMass /= mass;
    // if(id%1000==0){
    // printf("Node %d has %lu nodes, position (%f, %f, %f) and center of mass (%f, %f, %f)\n", id, indices.size(), x, y, z, centerOfMassX, centerOfMassY, centerOfMassZ);
    // printf("Particles have positions: ");
    // for(int index: indices){
    //     printf("(%f, %f, %f), ", particles->at(index).getX(), particles->at(index).getY(), particles->at(index).getZ());
    // }
    // printf("\n");
    // }

    for(StellarObject *stellarObject: objectsInNode){
        tempX = centreOfMass.getX() - stellarObject->getX();
        tempY = centreOfMass.getY() - stellarObject->getY();
        tempZ = centreOfMass.getZ() - stellarObject->getZ();
        tempMass = stellarObject->getMass();
        // Calculate quadropole
        quadropole.setEntry(0, 0, quadropole.getEntry(0, 0) + tempMass * (3*(tempX)*(tempX)-(std::pow(tempX, 2)+std::pow(tempY, 2)+std::pow(tempZ, 2))));
        quadropole.setEntry(0, 1, quadropole.getEntry(0, 1) + tempMass * (3*(tempX)*(tempY)));
        quadropole.setEntry(0, 2, quadropole.getEntry(0, 2) + tempMass * (3*(tempX)*(tempZ)));
        quadropole.setEntry(1, 0, quadropole.getEntry(1, 0) + tempMass * (3*(tempY)*(tempX)));
        quadropole.setEntry(1, 1, quadropole.getEntry(1, 1) + tempMass * (3*(tempY)*(tempY)-(std::pow(tempX, 2)+std::pow(tempY, 2)+std::pow(tempZ, 2))));
        quadropole.setEntry(1, 2, quadropole.getEntry(1, 2) + tempMass * (3*(tempY)*(tempZ)));
        quadropole.setEntry(2, 0, quadropole.getEntry(2, 0) + tempMass * (3*(tempZ)*(tempX)));
        quadropole.setEntry(2, 1, quadropole.getEntry(2, 1) + tempMass * (3*(tempZ)*(tempY)));
        quadropole.setEntry(2, 2, quadropole.getEntry(2, 2) + tempMass * (3*(tempZ)*(tempZ)-(std::pow(tempX, 2)+std::pow(tempY, 2)+std::pow(tempZ, 2))));
    }
    // printf("Quadropole:\n%f %f %f\n%f %f %f\n %f %f %f\n", q00, q01, q02, q10, q11, q12, q20, q21, q22);
    

    // Adjust box around particles
    minCornerPosition.setX(minX);
    minCornerPosition.setY(minY);
    minCornerPosition.setZ(minZ);
    length = maxX - minX;
    width = maxY - minY;
    height = maxZ - minZ;

    updateCorners();
    // printf("Adjusted min-/maxValues for node %d are: (%f, %f, %f), (%f, %f, %f)\n", id, minX, minY, minZ, maxX, maxY, maxZ);
}

void TreeCodeNode::calculateLeafValues(){
    leaf = true;
    mass = objectsInNode[0]->getMass();
    minCornerPosition = PositionVector(objectsInNode[0]->getX(), objectsInNode[0]->getY(), objectsInNode[0]->getZ());
    centreOfMass = minCornerPosition;
    length = 0;
    width = 0;
    height = 0;
}

bool TreeCodeNode::isLeaf(){
    return leaf;
}

void TreeCodeNode::freeNode(){
    // printf("Trying to free node %d\n", id);
    for(TreeCodeNode *child: subNodes){
        child->freeNode();
        delete child;
    }
}

int TreeCodeNode::getID(){
    return id;
}

PositionVector TreeCodeNode::calculateAcceleration(StellarObject *stellarObject){                   // TODO
    PositionVector acceleration(0.0, 0.0, 0.0);
    // printf("G = x?: %d\n", G == 6.67430 * pow(10, -11));

    // If node is leaf, calculate directly
    if(isLeaf()){
        // printf("Node is leaf\n");
        // If the two particles are at exactly the same spot, we set the force between them to 0    // TODO - crash together
        double distance = (centreOfMass - stellarObject->getPosition()).getLength();
        if(distance!=0){
            // Possible small improvement: multiply with G in the end and not in here already
            // ---------------------------------------------------------------- MAYBE? ----------------------------------------------------------------
            // Multiply with -1?
            acceleration = (centreOfMass - stellarObject->getPosition()) * (G * mass / std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3));
            // acceleration = (centreOfMass - stellarObject->getPosition()).normalise() * (G * mass / std::pow(distance, 2));
            // printf("(centreOfMass - stellarObject->getPosition()): (%f, %f, %f)\n", (centreOfMass - stellarObject->getPosition()).getX(), (centreOfMass - stellarObject->getPosition()).getY(), (centreOfMass - stellarObject->getPosition()).getZ());
            // printf("Acceleration: (%.10f, %.10f, %.10f)\n", acceleration.getX(), acceleration.getY(), acceleration.getZ());
            // printf("Rest: %f, G: %f, Mass: %f, G*mass: %f, div: %f\n", (G * mass / std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3)), G, mass, G * mass, std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3));
        }
        
        // printf("StellarObject %s with position (%f, %f, %f) is influenced by leaf node with CoM position (%f, %f, %f) and distance %f\n", stellarObject->getName(), 
        // stellarObject->getX(), stellarObject->getY(), stellarObject->getZ(), centreOfMass.getX(), centreOfMass.getY(), centreOfMass.getZ(), distance);
        // printf("Direct calculation: (%f, %f, %f)\n", acceleration.getX(), acceleration.getY(), acceleration.getZ());
        return acceleration;
    }

    // If angle is larger than MAX_ANGLE, let the children calculate the acceleration and add it up
    double angle = calculateAngle(stellarObject);
    if(angle > MAX_ANGLE){
        for(TreeCodeNode *child: subNodes){
            acceleration += child->calculateAcceleration(stellarObject);
        }
        return acceleration;
    }

    // If angle is small enough but node is no leaf, use multipole expansion to calculate the acceleration. As origin we take (0/0/0)
    // In the formula, position of object is r, and centreOfMass of node is s, so we are going to use that notation 
    PositionVector r = stellarObject->getPosition();
    PositionVector s = centreOfMass;
    PositionVector rS(0, 0, 0);     // Difference between the two
    rS = r - s;

    // Calculation according to paper
    PositionVector directionVector = rS.normalise();
    double rQr = directionVector.rQr(quadropole);
    PositionVector Qr = directionVector.Qr(quadropole);
    double distance = rS.getLength();
    double MR2 = -mass/(std::pow(distance, 2));
    double distancePneg4 = 1/std::pow(distance, 4);

    // Monopole
    acceleration = directionVector * MR2;
    // Quadropole
    acceleration +=  (Qr * distancePneg4) - ((directionVector * distancePneg4) * rQr) * 5/2;
    // Factor in gravitational constant
    acceleration *= G;

    // For reference calculations
    // if(0==0){
    //     PositionVector dSumAcc(0, 0, 0);
    //     double localAcceleration = 0;
    //     double distance = (getCentreOfMassPosition()-stellarObject->getPosition()).getLength();
    //     if(distance!=0){
    //         dSumAcc = (centreOfMass - stellarObject->getPosition()) * (G *  mass / std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3));
    //     }
    //     printf("StellarObject %s with position (%f, %f, %f) is influence by node with CoM position (%f, %f, %f). Distance is %f\n", stellarObject->getName(), 
    //     stellarObject->getX(), stellarObject->getY(), stellarObject->getZ(), centreOfMass.getX(), centreOfMass.getY(), centreOfMass.getZ(), (centreOfMass-stellarObject->getPosition()).getLength());
    //     printf("MultipoleAcceleration: (%.15f, %.15f, %.15f), direct calculation: (%.15f, %.15f, %.15f)\n", acceleration.getX(), acceleration.getY(), acceleration.getZ(), dSumAcc.getX(), dSumAcc.getY(), dSumAcc.getZ());
    // }
    return acceleration;
}

double TreeCodeNode::calculateAngle(StellarObject *stellarObject){
    double angle;
    // Approximating angle with l/distance
    double distance = (stellarObject->getPosition() - getCentreOfMassPosition()).getLength();
    double l = PositionVector(length, width, height).getLength();
    angle = l/distance;

    return angle;
}

void TreeCodeNode::updateCorners(){
    double x = minCornerPosition.getX();
    double y = minCornerPosition.getY();
    double z = minCornerPosition.getZ();
    corners.clear();
    corners.push_back(PositionVector(x, y, z));
    corners.push_back(PositionVector(x, y, z+height));
    corners.push_back(PositionVector(x, y+width, z));
    corners.push_back(PositionVector(x, y+width, z+height));
    corners.push_back(PositionVector(x+length, y, z));
    corners.push_back(PositionVector(x+length, y, z+height));
    corners.push_back(PositionVector(x+length, y+width, z));
    corners.push_back(PositionVector(x+length, y+width, z+height));
}

PositionVector TreeCodeNode::getCentreOfMassPosition(){
    return centreOfMass;
}

int TreeCodeNode::countNodes(){
    printf("Count nodes\n");
    int count = 1;
    for(TreeCodeNode *child: subNodes){
        count+=child->countNodes();
    }
    return count;
}