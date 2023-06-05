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

void Tree::buildTree(long double timestep){
    // printf("Tree builder\n");
    struct timespec prevTime;
	struct timespec currTime;
	int updateTime;
    clock_gettime(CLOCK_MONOTONIC, &prevTime);
    if(treeType){
        for(StellarObject *stellarObject: *objectsInTree){
            stellarObject->updateFutureVelocity(timestep);
            stellarObject->updateFuturePosition(timestep);
        }
    }
    root = new TreeCodeNode(objectsInTree);
    clock_gettime(CLOCK_MONOTONIC, &currTime);
    updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
    printf("Building tree took %d mics\n", updateTime);
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
        if(TREE_APPROACH){
            stellarObject->updateStellarAcceleration(tree);
            continue;
        }

        PositionVector acceleration = PositionVector();
        for(StellarObject *object: *objectsInTree){
            long double distance = (object->getFuturePosition()-stellarObject->getFuturePosition()).getLength();
            if(distance!=0){
                acceleration += (object->getFuturePosition() - stellarObject->getFuturePosition()).normalise() * G * object->getMass() / std::pow(distance, 2);
            }
        }
        // printf("Acceleration on %s is (%s)\n", stellarObject->getName(), acceleration.toString());
        // printf("%s - treeApproach acceleration: (%s), ", stellarObject->getName(), stellarObject->getFutureStellarAcceleration().toString());
        stellarObject->setFutureStellarAcceleration(acceleration);
        // printf(" directSummation acceleration: (%s)\n", stellarObject->getFutureStellarAcceleration().toString());
        // if(stellarObject->getType() != GALACTIC_CORE)
        //     printf("Future acceleration of %s at position (%s) is (%s) with parent %s position (%s)\n", stellarObject->getName(), stellarObject->getOldPosition().toString(), acceleration.toString(), stellarObject->getParent()->getName(), stellarObject->getParent()->getPosition().toString());
    }
}

void updateLocalAccelerationTreeMultiThread(Tree *tree, int begin, int end){
    StellarObject *stellarObject = nullptr;
    std::vector<StellarObject*> *objectsInTree = tree->getObjectsInTree();
    for(int i=begin;i<end;i++){
        stellarObject = objectsInTree->at(i);
        if(TREE_APPROACH){
            stellarObject->updateLocalAcceleration(tree);
            // printf("%s - treeApproach acceleration: (%s), ", stellarObject->getName(), stellarObject->getLocalAcceleration().toString());
            continue;
        }

        PositionVector acceleration = PositionVector();
        for(StellarObject *object: *objectsInTree){
            long double distance = (object->getPosition()-stellarObject->getPosition()).getLength();
            if(distance!=0){
                // acceleration += (object->getPosition() - stellarObject->getPosition()) * (G * object->getMass() / std::pow(std::sqrt((std::pow(distance, 2) + std::pow(EPSILON, 2))), 3));
                acceleration += (object->getPosition() - stellarObject->getPosition()).normalise() * G * object->getMass() / std::pow(distance, 2);
            }
        }
        stellarObject->setLocalAcceleration(acceleration);
        // printf(" directSummation acceleration: (%s)\n", stellarObject->getLocalAcceleration().toString());
    }
}

void Tree::update(long double timestep, Renderer *renderer){
    struct timespec prevTime;
	struct timespec currTime;
	int updateTime;
    clock_gettime(CLOCK_MONOTONIC, &prevTime);
    
    int threadNumber;
    int amount;
    std::vector<std::thread> threads;
    switch(treeType){
        // -------------------------------------------------------------------- TODO --------------------------------------------------------------------
        // Leapfrog - for stellar acceleration probably not really feasible
        case STELLAR_TREE:
            // The position and velocity update is done before the initialisation, such that the nodes also have the new values 
            // for(StellarObject *stellarObject: *objectsInTree){
            //     stellarObject->updateFutureVelocity(timestep);
            //     stellarObject->updateFuturePosition(timestep);
            // }
            threadNumber = 1;
            amount = objectsInTree->size()/threadNumber;
            threads.clear();
            for(int i=0;i<threadNumber-1;i++){
                threads.push_back(std::thread (updateStellarAccelerationTreeMultiThread, this, i * amount, i * amount + amount));
            }
            threads.push_back(std::thread (updateStellarAccelerationTreeMultiThread, this, (threadNumber-1)*amount, objectsInTree->size()));
            for(int i=0;i<threadNumber;i++){
                threads.at(i).join();
            }
            // This kind works, but with outwards drift. Keep away from it: normal way should work better
            // for(StellarObject *stellarObject: *objectsInTree){
            //     if(stellarObject->getType() == GALACTIC_CORE){
            //         stellarObject->setStellarAcceleration(stellarObject->getFutureStellarAcceleration());
            //         stellarObject->updateVelocity(timestep);
            //         stellarObject->updatePosition(timestep);
            //     }
            //     else{
            //         stellarObject->getChildren()->at(0)->updateVelocity(timestep/2);
            //         stellarObject->getChildren()->at(0)->setStellarAcceleration(stellarObject->getFutureStellarAcceleration());
            //         stellarObject->getChildren()->at(0)->updateVelocity(timestep/2);
            //         stellarObject->getChildren()->at(0)->updatePosition(timestep);
            //         stellarObject->setFuturePosition(stellarObject->getChildren()->at(0)->getPosition());
            //     }
            // }
            break;
        case LOCAL_TREE:
            // ----------------------------------------------------------- TODO -----------------------------------------------------------
            // Leapfrog curiosly does not work here at all
            // for(StellarObject *stellarObject: *objectsInTree){
            //         stellarObject->updateVelocity(timestep/2);
            //         currentlyUpdatingOrDrawingLock->lock();
            //         stellarObject->updatePosition(timestep);
            //         currentlyUpdatingOrDrawingLock->unlock();
            // }

            threadNumber = LOCAL_UPDATE_THREAD_COUNT;
            amount = objectsInTree->size()/threadNumber;
            threads.clear();
            for(int i=0;i<threadNumber-1;i++){
                threads.push_back(std::thread (updateLocalAccelerationTreeMultiThread, this, i * amount, i * amount + amount));
            }
            threads.push_back(std::thread (updateLocalAccelerationTreeMultiThread, this, (threadNumber-1)*amount, objectsInTree->size()));
            for(int i=0;i<threadNumber;i++){
                threads.at(i).join();
            }

            for(StellarObject *stellarObject: *objectsInTree){
                // stellarObject->updateVelocity(timestep/2);
                
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
            clock_gettime(CLOCK_MONOTONIC, &currTime);
            updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
            printf("Updating local tree took %d mics\n", updateTime);
            break;
    }
}

TreeCodeNode *Tree::getRoot(){
    return root;
}

std::vector<StellarObject*> *Tree::getObjectsInTree(){
    return objectsInTree;
}