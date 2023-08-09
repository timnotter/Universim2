#include <cstdio>
// #include <unistd.h>
#include <cmath>
#include <thread>
#include "treeCodeNode.hpp"
#include "tree.hpp"
#include "constants.hpp"

// TreeCodeNode::TreeCodeNode(long double x, long double y, long double z, long double length, long double width, long double height){
//     minCornerPosition = PositionVector(x, y, z);
//     this->length = length;
//     this->width = width;
//     this->height = height;
//     leaf = false;
// }

TreeCodeNode::TreeCodeNode(std::vector<StellarObject*> *objectsInNode){
    // printf("Constructor\n");
    minCornerPosition = PositionVector();
    leaf = false;
    type = LOCAL_TREE;
    if(objectsInNode->size() !=0 && (objectsInNode->at(0)->getType() == GALACTIC_CORE || objectsInNode->at(0)->getType() == STARSYSTEM)) type = STELLAR_TREE;
    for(StellarObject *stellarObject: *objectsInNode){
        this->objectsInNode.push_back(stellarObject);
    }
    // printf("Trying to calculate node value with node of size %lu\n", objectsInNode->size());
    calculateNodeValues();
    // printf("Size: %lu, length: %Lf\n", objectsInNode->size(), length);
    createSubNodes();
    // printf("Created node with %lu objects, position (%s), size (%Lf, %Lf, %Lf) and mass %Lf\n", objectsInNode->size(), centreOfMass.toString(), length, width, height, mass);
}

void TreeCodeNode::createSubNodes(){

    // ----------------------------------------------------------- Fix this: nodes are not splitted correctly -----------------------------------------------------------

    // printf("Create subnode with size: %lu and size (%Lf, %Lf, %Lf)\n", objectsInNode.size(), length, width, height);
    // usleep(100000);
    if(leaf || objectsInNode.size()==1) {
        // printf("Leaf\n");
        return;
    }
    // if(objectsInNode.size()<=3){
    //     for(StellarObject *stellarObject: objectsInNode){
    //         printf("%s: (%s)\n", stellarObject->getName(), getRelevantPositionOfTreetype(stellarObject).toString());
    //     }
    // }
    // printf("-------------------------------------------------------\n");
    // printf("Create subnodes with %lu objects\n", objectsInNode.size());
    // Create vectors to group objects into subnodes
    std::vector<std::vector<StellarObject*>> objectsInSubnodes;
    for(int i=0;i<8;i++){
        objectsInSubnodes.push_back(std::vector<StellarObject*>());
    }
    // Go through all objects and add them to corresponding subnode vector
    for(StellarObject *stellarObject: objectsInNode){
        // bool xCondition = stellarObject->getX() < minCornerPosition.getX()+length/2;
        // bool yCondition = stellarObject->getY() < minCornerPosition.getY()+width/2;
        // bool zCondition = stellarObject->getZ() < minCornerPosition.getZ()+height/2;
        bool xCondition = getRelevantXOfTreetype(stellarObject) < minCornerPosition.getX()+length/2;
        bool yCondition = getRelevantYOfTreetype(stellarObject) < minCornerPosition.getY()+width/2;
        bool zCondition = getRelevantZOfTreetype(stellarObject) < minCornerPosition.getZ()+height/2;

        // printf("%s: %d, %d, %d\n", stellarObject->getName(), xCondition, yCondition, zCondition);

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
            // printf("Inserting subnode\n");
            subNodes.push_back(new TreeCodeNode(&(objectsInSubnodes[i])));
        }
    }
}

void TreeCodeNode::calculateNodeValues(){       // Centre of mass position seems to be quite wrong!!
    // printf("Calculating node values for node of size %lu\n", objectsInNode.size());
    if(objectsInNode.size() == 1){
        calculateLeafValues();
        return;
    }
    mass = 0;
    long double minX = std::numeric_limits<long double>::max();
    long double minY = std::numeric_limits<long double>::max();
    long double minZ = std::numeric_limits<long double>::max();
    long double maxX = std::numeric_limits<long double>::lowest();
    long double maxY = std::numeric_limits<long double>::lowest();
    long double maxZ = std::numeric_limits<long double>::lowest();
    // __LDBL_MIN__ curiously doesn't work, while __LDBL_MAX does. Thus we use the function and not the macros
    // printf("Limits: %Lf - %Lf\n", minX, maxX);
    // printf("Real limits: %Lf - %Lf\n", std::numeric_limits<long double>::max(), std::numeric_limits<long double>::lowest());
    
    long double tempMass;
    long double tempX;
    long double tempY;
    long double tempZ;
    for(StellarObject *stellarObject: objectsInNode){
        tempX = getRelevantXOfTreetype(stellarObject);
        tempY = getRelevantYOfTreetype(stellarObject);
        tempZ = getRelevantZOfTreetype(stellarObject);
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
        // printf("ObjectX: %Lf, minX: %Lf, maxX: %Lf\n", tempX, minX, maxX);
        // Update center of mass
        centreOfMass += PositionVector(tempX, tempY, tempZ) * tempMass;
    }
    // printf("MinX: %Lf, MaxX: %Lf, objectsInNode.size(): %lu\n", minX, maxX, objectsInNode.size());
    // printf("Preadjusted centreOfMass (%s) and totalMass = %Lf\n", centreOfMass.toString(), mass);
    centreOfMass /= mass;
    // printf("Adjusted centreOfMass to final value of (%s)\n", centreOfMass.toString());

    for(StellarObject *stellarObject: objectsInNode){
        tempX = centreOfMass.getX() - getRelevantXOfTreetype(stellarObject);
        tempY = centreOfMass.getY() - getRelevantYOfTreetype(stellarObject);
        tempZ = centreOfMass.getZ() - getRelevantZOfTreetype(stellarObject);
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

    // Adjust box around particles
    minCornerPosition.setX(minX);
    minCornerPosition.setY(minY);
    minCornerPosition.setZ(minZ);
    length = maxX - minX;
    width = maxY - minY;
    height = maxZ - minZ;
    updateCorners();
    // printf("Values of new node:\n\tcentreOfMass: (%s)\n\tminCornerPosition: (%s)\n\tmaxCornerPosition: (%Lf, %Lf, %Lf)\n\tsize: (%Lf, %Lf, %Lf)\n", 
    // centreOfMass.toString(), minCornerPosition.toString(), maxX, maxY, maxZ, length, width, height);
    // if(objectsInNode.size()<=3){
    //     for(StellarObject *stellarObject: objectsInNode){
    //         printf("\t%s: (%s)\n", stellarObject->getName(), getRelevantPositionOfTreetype(stellarObject).toString());
    //     }
    // }

    // int counter = 0;
    // for(StellarObject *stellarObject: objectsInNode){
    //     if(!(getRelevantXOfTreetype(stellarObject) >= minX && getRelevantXOfTreetype(stellarObject) <= maxX) || 
    //     !(getRelevantYOfTreetype(stellarObject) >= minY && getRelevantYOfTreetype(stellarObject) <= maxY) ||
    //     !(getRelevantZOfTreetype(stellarObject) >= minZ && getRelevantZOfTreetype(stellarObject) <= maxZ)){
    //         // printf("Object %s not in Node itself!!!!!!!\n", stellarObject->getName());
    //         counter++;
    //     }
    // }
    // printf("From %lu total starsystems, %d are out of bounds\n", objectsInNode.size(), counter);
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // printf("Centre of mass of node: %s\n", centreOfMass.toString());
}

void TreeCodeNode::calculateLeafValues(){
    leaf = true;
    mass = objectsInNode[0]->getMass();
    minCornerPosition = PositionVector(getRelevantXOfTreetype(objectsInNode[0]), getRelevantYOfTreetype(objectsInNode[0]), getRelevantZOfTreetype(objectsInNode[0]));
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
    // printf("Calculating acceleration for %s with node with position (%s)\n", stellarObject->getName(), centreOfMass.toString());
    // printf("G = x?: %d\n", G == 6.67430 * pow(10, -11));

    // If node is leaf, calculate directly
    if(isLeaf()){
        // printf("Node is leaf\n");
        // If the two particles are at exactly the same spot, we set the force between them to 0    // TODO - crash together
        long double distance = (centreOfMass - getRelevantPositionOfTreetype(stellarObject)).getLength();
        if(distance!=0){
            // Possible small improvement: multiply with G in the end and not in here already
            // ---------------------------------------------------------------- MAYBE? ----------------------------------------------------------------
            // Multiply with -1?
            acceleration = (centreOfMass - getRelevantPositionOfTreetype(stellarObject)) * (G * mass / std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3));
            // acceleration = (centreOfMass - stellarObject->getPosition()).normalise() * (G * mass / std::pow(distance, 2));
            // printf("(centreOfMass - stellarObject->getPosition()): (%f, %f, %f)\n", (centreOfMass - stellarObject->getPosition()).getX(), (centreOfMass - stellarObject->getPosition()).getY(), (centreOfMass - stellarObject->getPosition()).getZ());
            // printf("Acceleration: (%.10f, %.10f, %.10f)\n", acceleration.getX(), acceleration.getY(), acceleration.getZ());
            // printf("Rest: %f, G: %f, Mass: %f, G*mass: %f, div: %f\n", (G * mass / std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3)), G, mass, G * mass, std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3));
        }
        
        // printf("StellarObject %s with position (%f, %f, %f) is influenced by leaf node with CoM position (%f, %f, %f) and distance %f\n", stellarObject->getName(), 
        // stellarObject->getX(), stellarObject->getY(), stellarObject->getZ(), centreOfMass.getX(), centreOfMass.getY(), centreOfMass.getZ(), distance);
        // printf("Direct calculation: (%f, %f, %f)\n", acceleration.getX(), acceleration.getY(), acceleration.getZ());
        // printf("Leaf acceleration of %s: (%s)\n", stellarObject->getName(), acceleration.toString());
        // std::string *nameComparison = new std::string("Sagittarius A*");
        // if(strcmp(stellarObject->getName(), nameComparison->c_str()) == 0){
        //     printf("Tree acceleration of %s on %s: (%s). Distance: %Lf\n", objectsInNode.at(0)->getName(), stellarObject->getName(), acceleration.toString(), distance);
        // }
        // delete nameComparison;
        return acceleration;
    }

    // If angle is larger than MAX_ANGLE, let the children calculate the acceleration and add it up
    long double angle = calculateAngle(stellarObject);
    if(angle > MAX_ANGLE){
        for(TreeCodeNode *child: subNodes){
            acceleration += child->calculateAcceleration(stellarObject);
        }
        return acceleration;
    }
    // printf("Angle is small enough for %s\n", stellarObject->getName());

    // If angle is small enough but node is no leaf, use multipole expansion to calculate the acceleration. As origin we take (0/0/0)
    // In the formula, position of object is r, and centreOfMass of node is s, so we are going to use that notation 
    PositionVector r = getRelevantPositionOfTreetype(stellarObject);
    PositionVector s = centreOfMass;
    PositionVector rS(0, 0, 0);     // Difference between the two
    rS = r - s;

    // Calculation according to paper
    PositionVector directionVector = rS.normalise();
    long double rQr = directionVector.rQr(quadropole);
    PositionVector Qr = directionVector.Qr(quadropole);
    long double distance = rS.getLength();
    long double MR2 = -mass/(std::pow(distance, 2));
    long double distancePneg4 = 1/std::pow(distance, 4);

    // Monopole
    acceleration = directionVector * MR2;
    // Quadropole
    acceleration +=  (Qr * distancePneg4) - ((directionVector * distancePneg4) * rQr) * 5/2;
    // Factor in gravitational constant
    acceleration *= G;

    // For reference calculations
    // if(0==0){
    //     PositionVector dSumAcc(0, 0, 0);
    //     long double localAcceleration = 0;
    //     long double distance = (getCentreOfMassPosition()-stellarObject->getPosition()).getLength();
    //     if(distance!=0){
    //         dSumAcc = (centreOfMass - stellarObject->getPosition()) * (G *  mass / std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3));
    //     }
    //     printf("StellarObject %s with position (%f, %f, %f) is influence by node with CoM position (%f, %f, %f). Distance is %f\n", stellarObject->getName(), 
    //     stellarObject->getX(), stellarObject->getY(), stellarObject->getZ(), centreOfMass.getX(), centreOfMass.getY(), centreOfMass.getZ(), (centreOfMass-stellarObject->getPosition()).getLength());
    //     printf("MultipoleAcceleration: (%.15f, %.15f, %.15f), direct calculation: (%.15f, %.15f, %.15f)\n", acceleration.getX(), acceleration.getY(), acceleration.getZ(), dSumAcc.getX(), dSumAcc.getY(), dSumAcc.getZ());
    // }
    // printf("Tree acceleration of %s: (%s). Angle is: %Lf\n", stellarObject->getName(), acceleration.toString(), angle);
    return acceleration;
}

long double TreeCodeNode::calculateAngle(StellarObject *stellarObject){
    long double angle;
    // Approximating angle with l/distance
    long double distance = (getRelevantPositionOfTreetype(stellarObject) - getCentreOfMassPosition()).getLength();
    // printf("Distance between node and %s: %Lf. position: %s, nodeposition: %s, position - nodeposition: (%s)\n", stellarObject->getName(), distance, stellarObject->getPosition().toString(), getCentreOfMassPosition().toString(), (stellarObject->getPosition() - getCentreOfMassPosition()).toString());
    long double l = PositionVector(length, width, height).getLength();
    angle = l/distance;

    return angle;
}

void TreeCodeNode::updateCorners(){
    long double x = minCornerPosition.getX();
    long double y = minCornerPosition.getY();
    long double z = minCornerPosition.getZ();
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

PositionVector TreeCodeNode::getRelevantPositionOfTreetype(StellarObject *stellarObject){
    if(type == LOCAL_TREE){
        return stellarObject->getPosition();
    }
    return stellarObject->getFuturePosition();
}

long double TreeCodeNode::getRelevantXOfTreetype(StellarObject *stellarObject){
    if(type == LOCAL_TREE){
        return stellarObject->getX();
    }
    return stellarObject->getFuturePosition().getX();
}

long double TreeCodeNode::getRelevantYOfTreetype(StellarObject *stellarObject){
    if(type == LOCAL_TREE){
        return stellarObject->getY();
    }
    return stellarObject->getFuturePosition().getY();
}

long double TreeCodeNode::getRelevantZOfTreetype(StellarObject *stellarObject){
    if(type == LOCAL_TREE){
        return stellarObject->getZ();
    }
    return stellarObject->getFuturePosition().getZ();
}