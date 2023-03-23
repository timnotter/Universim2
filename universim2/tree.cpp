#include <cstdio>
#include <vector>
#include "tree.hpp"
#include "constants.hpp"

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
void Tree::update(double timestep){
    switch(treeType){
        case STELLAR_TREE:
            // for(StellarObject *stellarObject: *objectsInTree){
                
            // }
            break;
        case LOCAL_TREE:
            // ----------------------------------------------- Doesn't work :( -----------------------------------------------
            // for(StellarObject *stellarObject: *objectsInTree){
            //     stellarObject->updateLocalAcceleration(this);
            // }
            // for(StellarObject *stellarObject: *objectsInTree){
            //     stellarObject->updateVelocity(timestep);
            //     currentlyUpdatingOrDrawingLock->lock();
            //     stellarObject->updatePosition(timestep);
            //     currentlyUpdatingOrDrawingLock->unlock();
                
            //     if(stellarObject->getType()==MOON){
            //         double distance = (stellarObject->getPosition()-stellarObject->getParent()->getPosition()).getLength();
            //         printf("Moon - Current distance: %f, initial periapsis: %f, initial apoapsis: %f\n", distance, (1 - stellarObject->getEccentricity()) * stellarObject->getMeanDistance(), (1 + stellarObject->getEccentricity()) * stellarObject->getMeanDistance());
            //     }
            // }
            for(StellarObject *stellarObject: *objectsInTree){
                PositionVector acceleration = PositionVector();
                for(StellarObject *object: *objectsInTree){
                    double distance = (object->getPosition()-stellarObject->getPosition()).getLength();
                    if(distance!=0){
                        acceleration += (object->getPosition() - stellarObject->getPosition()) * (G * object->getMass() / std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3));
                    }
                }
                stellarObject->setLocalAcceleration(acceleration);
            }
            for(StellarObject *stellarObject: *objectsInTree){
                stellarObject->updateVelocity(timestep);
                currentlyUpdatingOrDrawingLock->lock();
                stellarObject->updatePosition(timestep);
                currentlyUpdatingOrDrawingLock->unlock();
                
                if(stellarObject->getType()==MOON){
                    double distance = (stellarObject->getPosition()-stellarObject->getParent()->getPosition()).getLength();
                    printf("Moon - Current distance: %f, distance to CoM: %f, initial periapsis: %f, initial apoapsis: %f\n", distance, (stellarObject->getPosition() - stellarObject->getParent()->getUpdatedCentreOfMass()).getLength(), (1 - stellarObject->getEccentricity()) * stellarObject->getMeanDistance(), (1 + stellarObject->getEccentricity()) * stellarObject->getMeanDistance());
                }
            }
            break;
    }
}

TreeCodeNode *Tree::getRoot(){
    return root;
}