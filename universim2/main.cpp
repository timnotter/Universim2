#include <iostream>
#include <unistd.h>
#include <thread>
#include <fstream>
#include "main.hpp"
#include "window.hpp"
// #include "renderer.hpp"
#include "galacticCore.hpp"
#include "starSystem.hpp"
#include "star.hpp"
#include "planet.hpp"
#include "moon.hpp"
#include "comet.hpp"
#include "constants.hpp"
#include "date.hpp"
#include "tree.hpp"

// ------------------------------------------------------------------- SUGGESTION -------------------------------------------------------------------
// Maybe store store position of substellar objects in relation to their star system, such that we have smaller numbers

int main(int argc, char **argv){
	// Initialise base things
    MyWindow myWindow;
    bool isRunning = true;
    bool isPaused = true;
	Date date(1, 1, 2023);
	std::vector<StellarObject*> galaxies = std::vector<StellarObject*>();
	std::vector<StellarObject*> allObjects = std::vector<StellarObject*>();
	std::mutex currentlyUpdatingOrDrawingLock;
	std::mutex localUpdateIsReady;
	std::mutex stellarUpdateIsReady;
	const int optimalTimeDrawing = 1000000/20;									//In microseconds
	int optimalTimeLocalUpdate = 1000000/10;									//In microseconds
	Renderer renderer(&myWindow, &galaxies, &allObjects, &date, &currentlyUpdatingOrDrawingLock, &optimalTimeLocalUpdate);

	// Some test
	// renderer.cameraPlainVector1 = PositionVector(1, 0, 0);
	// renderer.cameraPlainVector2 = PositionVector(0, 1, 0);
	// PositionVector crossProduct = renderer.cameraPlainVector1.crossProduct(renderer.cameraPlainVector2);
	// renderer.cameraDirection = PositionVector(0, 0, 1);
	// renderer.cameraPlainEquationParameter4 = 0;

	// PositionVector pointA(5, 0, 2);
	// PositionVector pointB(0, 1, -2);

	// printf("Trying to find point, where the vector from (%s) to (%s) intersects the plane %Lf * x + %Lf * y + %Lf * z = %Lf\n", renderer.cameraPlainVector1.toString(), renderer.cameraPlainVector2.toString(), crossProduct.getX(), crossProduct.getY(), crossProduct.getZ(), renderer.cameraPlainEquationParameter4);
	// pointA = renderer.findIntersectionWithCameraPlane(pointA, pointB);
	// printf("Found intersection (%s)\n", pointA.toString());
	
	// sleep(10);

	// The window needs a little time to show up properly
	renderer.drawWaitingScreen();
	usleep(1000);
	renderer.drawWaitingScreen();
	// printf("Waiting screen drawn\n");

	initialiseStellarObjects(&galaxies, &allObjects, &currentlyUpdatingOrDrawingLock);
	// printf("Initialised\n");
	renderer.initialiseReferenceObject();

	struct timespec prevTime;
	struct timespec currTime;
	int updateTime;
	// Start gravity calculations
	std::thread *localUpdater = new std::thread(localUpdate, &currentlyUpdatingOrDrawingLock, &galaxies, &allObjects, &isRunning, &isPaused, &date, &optimalTimeLocalUpdate, &renderer, &localUpdateIsReady, &stellarUpdateIsReady);
	std::thread *stellarUpdater = new std::thread(stellarUpdate, &currentlyUpdatingOrDrawingLock, &galaxies, &isRunning, &renderer, &allObjects, &localUpdateIsReady, &stellarUpdateIsReady);
	while(isRunning){
		// Handle events
        clock_gettime(CLOCK_MONOTONIC, &prevTime);
		while(XPending(myWindow.getDisplay())){
            XNextEvent(myWindow.getDisplay(), myWindow.getEvent());
            myWindow.handleEvent(*(myWindow.getEvent()), isRunning, isPaused);
        }
		// clock_gettime(CLOCK_MONOTONIC, &currTime);
		// updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
		// printf("Events took %d mics\n", updateTime);


		renderer.draw();
        clock_gettime(CLOCK_MONOTONIC, &currTime);
		updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
		// printf("Events and drawing took %d mics, compared to wanted %d mics. Now sleeping %d\n", updateTime, optimalTimeDrawing, (optimalTimeDrawing-updateTime));
        // clock_gettime(CLOCK_MONOTONIC, &prevTime);
		int difference = optimalTimeDrawing-updateTime;
		if(difference > 0) {
			if(difference > 15000){
				renderer.adjustThreadCount(DECREASE_THREAD_COUNT);
			}
			usleep(difference);
		}
		else{
			renderer.adjustThreadCount(INCREASE_THREAD_COUNT);
			// printf("Difference: %d mics\n", difference);
		}
		// printf("Difference: %d mics\n", difference);

        // clock_gettime(CLOCK_MONOTONIC, &currTime);
		// printf("Sleeptime: %dmics\n", ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000));
	}

	// Close/Free stuff
	myWindow.closeWindow();
	// Recursively free everything
	for(StellarObject *galacticCore: galaxies){
        galacticCore->freeObject();
        delete galacticCore;
    }

	return 0;
}	

void localUpdate(std::mutex *currentlyUpdatingOrDrawingLock, std::vector<StellarObject*> *galaxies, std::vector<StellarObject*> *allObjects, bool *isRunning, bool *isPaused, Date *date, int *optimalTimeLocalUpdate, Renderer *renderer, std::mutex *localUpdateIsReady, std::mutex *stellarUpdateIsReady){
	// Create vectors of all objects in the individual systems
	std::vector<std::vector<StellarObject*>> starSystemsToUpdate;
	std::vector<StellarObject*> loneStars;
	// Go through all star systems, and if they are not lone stars, add a vector of all objects in the current star system to the starsystemToUpdate
	
	for(StellarObject *galacticCore: *galaxies){
		for(StellarObject *starSystem: *(galacticCore->getChildren())){
			if((dynamic_cast<StarSystem*>(starSystem))->getLoneStar()) {
				loneStars.push_back(starSystem);
				continue;
			}
			starSystemsToUpdate.push_back(std::vector<StellarObject*>());

			std::vector<StellarObject*> queue;
			for(StellarObject *current: *(starSystem->getChildren())) queue.push_back(current);

			StellarObject *current;
			while(!queue.empty()){
				current = queue.back();
				queue.pop_back();
				starSystemsToUpdate.back().push_back(current);
				for(StellarObject *children: *(current->getChildren())) queue.push_back(children);
			}
		}
	}
	// Run the simulation with the appropriate speed
	struct timespec initialTime;
	struct timespec prevTime;
	struct timespec currTime;
	// int optimalTime = 1000000/20;			//In microseconds
	int updateTime;
	int localUpdatesPerStellarUpdate = (TIMESTEP_STELLAR) / (TIMESTEP_LOCAL);
	localUpdateIsReady->lock();
	int stellarUpdateCounter = 0;
	int loneStarUpdateCounter = 0;
	clock_gettime(CLOCK_MONOTONIC, &initialTime);
	while(*isRunning){
		// printf("running\n");
		if(*isPaused){
			usleep(1000000/24);
			continue;
		}

		// This portion is only for testing stellar force calculation
		if(localUpdatesPerStellarUpdate == 1){
			for(StellarObject *stellarObject: *allObjects){
				if(stellarObject->getType() == STARSYSTEM) {
					// stellarObject->updateVelocity(TIMESTEP_LOCAL);
					continue;
				}
				stellarObject->updateVelocity(TIMESTEP_LOCAL);
				stellarObject->updatePosition(TIMESTEP_LOCAL);
			}
			// printf("Current centre of mass of galaxy %s: (%s)\n", galaxies->at(0)->getName(), galaxies->at(0)->getUpdatedCentreOfMass().toString());
			localUpdateIsReady->unlock();
			stellarUpdateIsReady->lock();
			localUpdateIsReady->lock();
			stellarUpdateIsReady->unlock();
			date->incYear(100);
			// usleep(100);
			continue;
		}


		// Loop over all starSystems, build their tree and update them
		// clock_gettime(CLOCK_MONOTONIC, &prevTime);
		for(std::vector<StellarObject*> &objects: starSystemsToUpdate){
			Tree tree(&objects, currentlyUpdatingOrDrawingLock);
			tree.buildTree();
			tree.update(TIMESTEP_LOCAL, renderer);
			tree.destroyTree();
		}
		// clock_gettime(CLOCK_MONOTONIC, &currTime);
		// updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
		// printf("Building and updating tree took %d mics\n", updateTime);


		// Updating all lone stars is expensive (there are many) and not really significant, for their position changes are barely visible, even if close, since they have
		// no close objects around them -> we only update them every LONESTAR_BACKOFF_AMOUNT normal update.
		if(++loneStarUpdateCounter == LONESTAR_BACKOFF_AMOUNT){
			loneStarUpdateCounter = 0;
			clock_gettime(CLOCK_MONOTONIC, &prevTime);
			int threadNumber = LOCAL_UPDATE_THREAD_COUNT;
			int amount = loneStars.size()/threadNumber;
			std::vector<std::thread> threads;
			for(int i=0;i<threadNumber-1;i++){
				threads.push_back(std::thread(updateLoneStarStarsystemMultiThread, &loneStars, i * amount, i * amount + amount, (TIMESTEP_LOCAL * LONESTAR_BACKOFF_AMOUNT)));
			}
			threads.push_back(std::thread(updateLoneStarStarsystemMultiThread, &loneStars, (threadNumber-1)*amount, loneStars.size(), (TIMESTEP_LOCAL * LONESTAR_BACKOFF_AMOUNT)));
			for(int i=0;i<threadNumber;i++){
				threads.at(i).join();
			}
			clock_gettime(CLOCK_MONOTONIC, &currTime);
			updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
			// printf("Updating loneStars took %d mics\n", updateTime);

			for(StellarObject *galacticCore: *galaxies){
				galacticCore->updateVelocity(TIMESTEP_LOCAL * 10);
				galacticCore->updatePosition(TIMESTEP_LOCAL * 10);
			}
		}
		// Incrementing date
		date->incSecond(TIMESTEP_LOCAL);

		// If it is time, wait until the stellar force calculation has finished and insert it's values. From then on use the new stellar acceleration
		if(++stellarUpdateCounter == localUpdatesPerStellarUpdate){
			stellarUpdateCounter = 0;
			localUpdateIsReady->unlock();
			stellarUpdateIsReady->lock();
			localUpdateIsReady->lock();
			stellarUpdateIsReady->unlock();
		}

		// Time keeping and sleeping, such that desired speed is achieved
		clock_gettime(CLOCK_MONOTONIC, &currTime);
		updateTime = ((1000000000*(currTime.tv_sec-initialTime.tv_sec)+(currTime.tv_nsec-initialTime.tv_nsec))/1000);
		// printf("Updating took %d mics, compared to wanted %d mics. Now sleeping %d\n", updateTime, *optimalTimeLocalUpdate, (*optimalTimeLocalUpdate-updateTime));
		if((*optimalTimeLocalUpdate-updateTime) > 0) usleep(*optimalTimeLocalUpdate-updateTime);
        clock_gettime(CLOCK_MONOTONIC, &initialTime);
	}	
}

void updateLoneStarStarsystemMultiThread(std::vector<StellarObject*> *loneStars, int begin, int end, long double timestep){
	for(int i=begin;i<end;i++){
		StellarObject *loneStar = loneStars->at(i);
		loneStar->getChildren()->at(0)->updateVelocity(timestep);
		loneStar->getChildren()->at(0)->updatePosition(timestep);
	}
}

void stellarUpdate(std::mutex *currentlyUpdatingOrDrawingLock, std::vector<StellarObject*> *galaxies, bool *isRunning, Renderer *renderer, std::vector<StellarObject*> *allObjects, std::mutex *localUpdateIsReady, std::mutex *stellarUpdateIsReady){
	// Create vectors of all objects in the current galaxy					- Possibly add support for multiple galaxies with 
	std::vector<std::vector<StellarObject*>> galaxiesToUpdate;
	// Go through all galaxies and add starsystems to respective vector
	for(StellarObject *galacticCore: *galaxies){
		if(galacticCore->getChildren()->size()==0) continue;
		galaxiesToUpdate.push_back(std::vector<StellarObject*>());
		galaxiesToUpdate.back().push_back(galacticCore);
		for(StellarObject *starSystem: *(galacticCore->getChildren())){
			galaxiesToUpdate.back().push_back(starSystem);
		}
	}
	// Store galacitc acceleration inside starsystems, which do not get updated otherwise, then poll them for the local update

	// Run the simulation with the appropriate speed
	struct timespec prevTime;
	struct timespec currTime;
	stellarUpdateIsReady->lock();
	while(*isRunning){
		// --------------------------------------------------------- TODO --------------------------------------------------------- Acceleration doesn't work
		clock_gettime(CLOCK_MONOTONIC, &prevTime);
		for(std::vector<StellarObject*> &galaxy: galaxiesToUpdate){
			Tree tree(&galaxy, currentlyUpdatingOrDrawingLock);
			tree.buildTree(TIMESTEP_STELLAR);
			tree.update(TIMESTEP_STELLAR, renderer);
			tree.destroyTree();
		}
		clock_gettime(CLOCK_MONOTONIC, &currTime);
		int updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
		printf("Stellar force calculation took %d mics\n", updateTime);

		// Wait until local updates have caught up, swap in new acceleration values
		localUpdateIsReady->lock();
		for(StellarObject *stellarObject: *allObjects){
			stellarObject->updateNewStellarValues();
		}
		stellarUpdateIsReady->unlock();
		localUpdateIsReady->unlock();
		// Make sure the local update thread has picked up the localUpdateIsReady lock
		while(localUpdateIsReady->try_lock()){
			localUpdateIsReady->unlock();
			usleep(10);
		}
		stellarUpdateIsReady->lock();
		printf("Full update of future values has been done\n");
	}
}

void initialiseStellarObjects(std::vector<StellarObject*> *galaxies, std::vector<StellarObject*> *allObjects, std::mutex *currentlyUpdatingOrDrawingLock){	
	// Initialise all objects
	galaxies->push_back(new GalacticCore("Sagittarius A*", 1, 1, 0, 0xFFFF00));
	// ADD_STARSYSTEM(new StarSystem("Solar System", 1, 0.07, 1.047)); 			// this inclinations is from the solar plan to the galactic plane, thus not of interest
	ADD_STARSYSTEM(new StarSystem("Solar System", 1, 0.07, 0.1));
	ADD_STAR(new Star("Sun", 1, 1, 0, 0, 0, 5770));
    ADD_PLANET(new Planet("Mercury", 0.3829, 0.055, 0.387098, 0.205630, 7.005*PI/180));
    ADD_PLANET(new Planet("Venus", 0.9499, 0.815, 0.723332, 0.006772, 3.39458*PI/180));
	ADD_PLANET(new Planet("Earth", 1, 1, 1, 0.0167086, 0));
    ADD_MOON(new Moon("Moon", 1, 1, 1, 0.0549, 5.145*PI/180));
    // ADD_PLANET(new Planet("Mars", 0.532, 0.107, 1.52368055, 0.0934, 1.85*PI/180));
    // ADD_MOON(new Moon("Phobos", 11266.7/lunarRadius, 1.0659e16/lunarMass, 9376000/distanceEarthMoon, 0.0151, 26.04*PI/180));
    ADD_PLANET(new Planet("Mars", 0.532, 0.107, 1.52368055, 0, 0));
    ADD_MOON(new Moon("Phobos", 11266.7/lunarRadius, 1.0659e16/lunarMass, 9376000/distanceEarthMoon, 0, 0));
    ADD_MOON(new Moon("Deimos", 6200/lunarRadius, 1.4762e15/lunarMass, 23463200/distanceEarthMoon, 0.00033, 27.58*PI/180));
    ADD_PLANET(new Planet("Jupiter", 10.973, 317.8, 5.204, 0.0489, 1.303*PI/180));
	readMoonFile("./files/MoonsOfJupiterAdjusted.csv", galaxies->back()->getChildren()->back()->getChildren()->back()->getChildren()->back());
	// Read file of Jupiters moons
    ADD_PLANET(new Planet("Saturn", 8.552, 95.159, 9.5826, 0.0565, 2.485*PI/180));
	// Read file of Saturns moons
    ADD_PLANET(new Planet("Uranus", 25362000/terranRadius, 14.536, 19.19126, 0.04717, 0.773*PI/180));
	// Read file of Uranus' moons
    ADD_PLANET(new Planet("Neptune", 24622000/terranRadius, 17.147, 30.07, 0.008678, 1.77*PI/180));
	readMoonFile("./files/MoonsOfNeptuneAdjusted.csv", galaxies->back()->getChildren()->back()->getChildren()->back()->getChildren()->back());
	// Read file of Neptunes moons
    ADD_PLANET(new Planet("Pluto", 0.1868, 0.00218, 39.482, 0.2488, 17.16*PI/180));
	ADD_MOON(new Moon("Charon", 606000/lunarRadius, 1.586e21/lunarMass, 17181000/distanceEarthMoon, 0.0002, 112.783*PI/180));

	// ADD_STARSYSTEM(new StarSystem("Solar System", 1, 0, 0));
	// ADD_STAR(new Star("Sun", 1, 1, 0, 0, 0, 5770));
    // ADD_PLANET(new Planet("Earth", 1, 1, 1, 0, 0));
    // ADD_MOON(new Moon("Moon", 1, 1, 1, 0, 0));

	// ADD_STARSYSTEM(new StarSystem("Solar System", 0.01, 0, 0));
	// ADD_STAR(new Star("Sun", 1, 1, 0, 0, 0, 5770));

	// Add random stars - currently orbit is still fixed
	int totalStarsystems = 10000;
	int threadNumber;
	int amount;
	long double characteristicScaleLength = 13000 * lightyear;
	// We use a function derived from the cumulative distribution function from hernquist
	auto densityFunction = [characteristicScaleLength](double u) -> long double {
		return characteristicScaleLength * (std::sqrt(u) + u) / (1-u);
	};

	// printf("densityFunction(0.25) = %.0Lf\n", densityFunction(0.25));
	// printf("a = %Lf, perc: %Lf\n", characteristicScaleLength, densityFunction(0.25)/characteristicScaleLength);
    std::vector<std::thread> threads;
	threadNumber = std::min((std::max(totalStarsystems/10, 16))/16, 16);
    amount = totalStarsystems/threadNumber;
    for(int i=0;i<threadNumber-1;i++){
        threads.push_back(std::thread (spawnStarSystemsMultiThread, galaxies, amount, currentlyUpdatingOrDrawingLock, densityFunction));
    }
    threads.push_back(std::thread (spawnStarSystemsMultiThread, galaxies, totalStarsystems - (threadNumber-1)*amount, currentlyUpdatingOrDrawingLock, densityFunction));
	for(int i=0;i<threadNumber;i++){
        threads.at(i).join();
    }

	for(StellarObject *galacticCore: *galaxies){
        for(StellarObject *starSystem: *(galacticCore->getChildren())){
            for(StellarObject *star: *(starSystem->getChildren())){
                for(StellarObject *planet: *(star->getChildren())){
                    for(StellarObject *moon: *(planet->getChildren())){
						allObjects->push_back(moon);
                    }
                    allObjects->push_back(planet);
                }
                allObjects->push_back(star);
            }
			allObjects->push_back(starSystem);
        }
        allObjects->push_back(galacticCore);
    }
	// printf("Created all stellar objects\n");

	// Place all objects in space and set correct speed
	for(StellarObject *galacticCore: *galaxies){
		galacticCore->place();
	}

	// printf("Distance of moon and earth: %f\n", (galaxies->at(0)->getChildren()->at(0)->getChildren()->at(0)->getChildren()->at(0)->getPosition() - galaxies->at(0)->getChildren()->at(0)->getChildren()->at(0)->getChildren()->at(0)->getChildren()->at(0)->getPosition()).getLength());
	// printf("Distance of moon and CoM earthsystem: %f\n", (galaxies->at(0)->getChildren()->at(0)->getChildren()->at(0)->getChildren()->at(0)->getCentreOfMass() - galaxies->at(0)->getChildren()->at(0)->getChildren()->at(0)->getChildren()->at(0)->getChildren()->at(0)->getPosition()).getLength());
	// printf("Placed all stellar objects\n");
}
void spawnStarSystemsMultiThread(std::vector<StellarObject*> *globalGalaxies, int amount, std::mutex *currentlyUpdatingOrDrawingLock, std::function<long double(double)> densityFunction){
	// globalGalaxies is called like that, because then we can use the name "galaxies" and don't have to change the define
	std::vector<StellarObject*> *galaxies = new std::vector<StellarObject*>();
	galaxies->push_back(new GalacticCore("", 0, 0, 0));
	for(int i=0;i<amount;i++){
		ADD_STARSYSTEM(new StarSystem(densityFunction, 1));
	}

	// We reuse the currentlyUpdatingOrDrawingLock
	currentlyUpdatingOrDrawingLock->lock();
	for(StellarObject *stellarObject: *(galaxies->at(0)->getChildren())){
		globalGalaxies->at(0)->addChild(stellarObject);
	}
	currentlyUpdatingOrDrawingLock->unlock();
	delete galaxies;
}

void readMoonFile(std::string fileLocation, StellarObject *parent){
	std::string name;
	long double mass;
	long double meanDistance;
	long double period;
	long double eccentricity;
	long double inclination;
	long double radius;
    std::ifstream moonFile;
    moonFile.open(fileLocation);
	// int counter = 0;
	while(!moonFile.eof()){
		moonFile >> name;
		moonFile >> mass;
		moonFile >> meanDistance;
		moonFile >> period;
		moonFile >> inclination;
		moonFile >> eccentricity;
		moonFile >> radius;
		// counter++;
		// If the period is to short, we ignore the moon, for it would be simulated badly anyways
		if(std::abs(period) < 1)
			continue;
		parent->addChild(new Moon(name.c_str(), radius/lunarRadius, mass/lunarMass, meanDistance/distanceEarthMoon, eccentricity, inclination));
		moonFile.peek();
		// printf("Read %s, radius: %Lf, mass: %Lf, meanDistance: %Lf, eccentricity: %Lf, inclination: %Lf, period: %Lf\n", name.c_str(), radius/lunarRadius, mass/lunarMass, meanDistance/distanceEarthMoon, eccentricity, inclination, period);
	}
    moonFile.close();
}