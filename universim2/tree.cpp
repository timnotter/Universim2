#include <cstdio>
#include <vector>
#include <unistd.h>
#include <thread>
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
    // printf("Tree builder\n");
    root = new TreeCodeNode(objectsInTree);
    // printf("Tree builder end\n");
}

void Tree::destroyTree(){
    root->freeNode();
    delete root;
}

void updateStellarAccelerationTreeMultiThread(Tree *tree, int begin, int end){
    StellarObject *stellarObject = nullptr;
    std::vector<StellarObject*> *objectsInTree = tree->getObjectsInTree();
    for(int i=begin;i<end;i++){
        stellarObject = objectsInTree->at(i);
        // stellarObject->updateStellarAcceleration(tree);

        PositionVector acceleration = PositionVector();
        for(StellarObject *object: *objectsInTree){
            long double distance = (object->getFuturePosition()-stellarObject->getFuturePosition()).getLength();
            if(distance!=0){
                // acceleration += (object->getPosition() - stellarObject->getPosition()) * (G * object->getMass() / std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3));
                acceleration += (object->getFuturePosition() - stellarObject->getFuturePosition()).normalise() * G * object->getMass() / std::pow(distance, 2);
            }
        }
        // printf("Acceleration on %s is (%s)\n", stellarObject->getName(), acceleration.toString());
        stellarObject->setFutureStellarAcceleration(acceleration);
    }
}

void Tree::update(long double timestep, Renderer *renderer){
    bool stellarTreeMultiThread = true;
    bool treeApproach = true;
    switch(treeType){
        // -------------------------------------------------------------------- TODO --------------------------------------------------------------------
        // Leapfrog
        case STELLAR_TREE:
            // for(StellarObject *stellarObject: *objectsInTree){
            //     stellarObject->updateFutureVelocity(timestep);
            //     stellarObject->updateFuturePosition(timestep);
            // }
            if(stellarTreeMultiThread){
                int threadNumber = 16;
                int amount = objectsInTree->size()/threadNumber;
                std::vector<std::thread> threads;
                for(int i=0;i<threadNumber-1;i++){
                    threads.push_back(std::thread (updateStellarAccelerationTreeMultiThread, this, i * amount, i * amount + amount));
                }
                threads.push_back(std::thread (updateStellarAccelerationTreeMultiThread, this, (threadNumber-1)*amount, objectsInTree->size()));
                for(int i=0;i<threadNumber;i++){
                    threads.at(i).join();
                }
                // This works
                for(StellarObject *stellarObject: *objectsInTree){
                    if(stellarObject->getType() == GALACTIC_CORE){
                        stellarObject->setStellarAcceleration(stellarObject->getFutureStellarAcceleration());
                        stellarObject->updateVelocity(timestep);
                        stellarObject->updatePosition(timestep);
                    }
                    else{
                        stellarObject->getChildren()->at(0)->updateVelocity(timestep/2);
                        stellarObject->getChildren()->at(0)->setStellarAcceleration(stellarObject->getFutureStellarAcceleration());
                        stellarObject->getChildren()->at(0)->updateVelocity(timestep/2);
                        stellarObject->getChildren()->at(0)->updatePosition(timestep);
                        stellarObject->setFuturePosition(stellarObject->getChildren()->at(0)->getPosition());
                    }
                }
            }
            else{
                for(StellarObject *stellarObject: *objectsInTree){
                    if(treeApproach){
                        stellarObject->updateStellarAcceleration(this);
                    }
                    else{
                        PositionVector acceleration = PositionVector();
                        for(StellarObject *object: *objectsInTree){
                            long double distance = (object->getFuturePosition()-stellarObject->getFuturePosition()).getLength();
                            if(distance!=0){
                                // acceleration += (object->getPosition() - stellarObject->getPosition()) * (G * object->getMass() / std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3));
                                acceleration += (object->getFuturePosition() - stellarObject->getFuturePosition()).normalise() * G * object->getMass() / std::pow(distance, 2);
                            }
                        }
                        // printf("Acceleration on %s is (%s)\n", stellarObject->getName(), acceleration.toString());
                        stellarObject->setFutureStellarAcceleration(acceleration);
                    }
                }
            }
            break;
        case LOCAL_TREE:
            for(StellarObject *stellarObject: *objectsInTree){
                // Tree approach - Gets seg fault :(
                stellarObject->updateLocalAcceleration(this);

                // Direct summation
                // PositionVector acceleration = PositionVector();
                // for(StellarObject *object: *objectsInTree){
                //     long double distance = (object->getPosition()-stellarObject->getPosition()).getLength();
                //     if(distance!=0){
                //         // acceleration += (object->getPosition() - stellarObject->getPosition()) * (G * object->getMass() / std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3));
                //         acceleration += (object->getPosition() - stellarObject->getPosition()).normalise() * G * object->getMass() / std::pow(distance, 2);
                //     }
                // }
                // stellarObject->setLocalAcceleration(acceleration);
            }
            for(StellarObject *stellarObject: *objectsInTree){
                stellarObject->updateVelocity(timestep);
                currentlyUpdatingOrDrawingLock->lock();
                stellarObject->updatePosition(timestep);
                currentlyUpdatingOrDrawingLock->unlock();
                
                // Debugging purposes
                // if(stellarObject->getType()==MOON){
                //     long double distance = (stellarObject->getPosition()-stellarObject->getParent()->getPosition()).getLength();
                //     distance = (stellarObject->getPosition()-stellarObject->getParent()->getUpdatedCentreOfMass()).getLength();
                //     // printf("%s - Current distance: %Lf, initial periapsis: %Lf, initial apoapsis: %Lf\n", stellarObject->getName(), distance, (1 - stellarObject->getEccentricity()) * stellarObject->getMeanDistance(), (1 + stellarObject->getEccentricity()) * stellarObject->getMeanDistance());
                //     // printf("Moon - Speed: (%s), X-Speed: %.12f\n", stellarObject->getVelocity().toString(), stellarObject->getVelocity().getX());
                //     if(counter++ % 100 == 0){
                //         distance = (stellarObject->getPosition()-stellarObject->getParent()->getPosition()).getLength();
                //         // renderer->dataPoints.push_back(distance);
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

std::vector<StellarObject*> *Tree::getObjectsInTree(){
    return objectsInTree;
}