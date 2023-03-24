#include <cstdio>
#include <vector>
#include <unistd.h>
#include "tree.hpp"
#include "constants.hpp"

static int counter = 0;


Tree::Tree(std::vector<StellarObject*> *objectsInTree, std::mutex *currentlyUpdatingOrDrawingLock){
    this->objectsInTree = objectsInTree;
    if(objectsInTree->size()==0){
        printf("Invalid vector of StellarObjects in tree constructor!\n");
        return;
    }
    treeType = (objectsInTree->at(0)->getType() == GALACTIC_CORE || objectsInTree->at(0)->getType() == STARSYSTEM);
    this->currentlyUpdatingOrDrawingLock = currentlyUpdatingOrDrawingLock;
}

void Tree::buildTree(){
    root = new TreeCodeNode(objectsInTree);
}

void Tree::destroyTree(){
    root->freeNode();
    delete root;
}
void Tree::update(double timestep, Renderer *renderer){
    switch(treeType){
        case STELLAR_TREE:
            // for(StellarObject *stellarObject: *objectsInTree){
                
            // }
            break;
        case LOCAL_TREE:
            // ----------------------------------------------- Doesn't work :( -----------------------------------------------
            for(StellarObject *stellarObject: *objectsInTree){ // None of the approaches currently works :/
                // Tree approach
                stellarObject->updateLocalAcceleration(this);

                // Direct summation
                // printf("Calculating gravity for %s. Used objects: \n", stellarObject->getName());
                // PositionVector acceleration = PositionVector();
                // for(StellarObject *object: *objectsInTree){
                //     // StellarObject *object = stellarObject->getParent();
                //     double distance = (object->getPosition()-stellarObject->getPosition()).getLength();
                //     if(distance!=0){
                //         // acceleration += (object->getPosition() - stellarObject->getPosition()) * (G * object->getMass() / std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3));
                //         acceleration += (object->getPosition() - stellarObject->getPosition()).normalise() * G * object->getMass() / std::pow(distance, 2);
                //         // printf("%s, \n", object->getName());
                //     }
                // }
                // stellarObject->setLocalAcceleration(acceleration);
                // printf("total acceleration of %s: (%s), velocity: (%s), position: (%s)\n", stellarObject->getName(), stellarObject->getLocalAcceleration().toString(), stellarObject->getVelocity().toString(), stellarObject->getPosition().toString());
            }
            for(StellarObject *stellarObject: *objectsInTree){
                stellarObject->updateVelocity(timestep);
                currentlyUpdatingOrDrawingLock->lock();
                stellarObject->updatePosition(timestep);
                currentlyUpdatingOrDrawingLock->unlock();
                
                // Debugging purposes
                // if(stellarObject->getType()==MOON){
                //     double distance = (stellarObject->getPosition()-stellarObject->getParent()->getPosition()).getLength();
                //     // printf("Moon - Current distance: %f, initial periapsis: %f, initial apoapsis: %f\n", distance, (1 - stellarObject->getEccentricity()) * stellarObject->getMeanDistance(), (1 + stellarObject->getEccentricity()) * stellarObject->getMeanDistance());
                //     // printf("Moon - Speed: (%s), X-Speed: %.12f\n", stellarObject->getVelocity().toString(), stellarObject->getVelocity().getX());
                //     if(counter++ % 1 == 0){
                //         distance = (stellarObject->getPosition()-stellarObject->getParent()->getPosition()).getLength();
                //         renderer->dataPoints.push_back(distance);
                //         renderer->dataPoints2.push_back((stellarObject->getPosition()-stellarObject->getParent()->getPosition()).getX());
                //         renderer->dataPoints3.push_back((stellarObject->getPosition()-stellarObject->getParent()->getPosition()).getY());
                //         counter = 1;
                //     }
                // }
            }
            // sleep(1);
            break;
    }
}

TreeCodeNode *Tree::getRoot(){
    return root;
}