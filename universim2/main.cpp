#include <iostream>
#include <unistd.h>
#include <thread>
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
	const int optimalTimeDrawing = 1000000/20;									//In microseconds
	int optimalTimeLocalUpdate = 1000000/10;									//In microseconds
	Renderer renderer(&myWindow, &galaxies, &allObjects, &date, &currentlyUpdatingOrDrawingLock, &optimalTimeLocalUpdate);

	// The window needs a little time to show up properly
	renderer.drawWaitingScreen();
	usleep(1000);
	renderer.drawWaitingScreen();
	printf("Waiting screen drawn\n");

	initialiseStellarObjects(&galaxies, &allObjects);
	printf("Initialised\n");
	renderer.initialiseReferenceObject();

	struct timespec prevTime;
	struct timespec currTime;
	int updateTime;
	// Start gravity calculations
	std::thread *updater = new std::thread(localUpdate, &currentlyUpdatingOrDrawingLock, &galaxies, &isRunning, &isPaused, &date, &optimalTimeLocalUpdate, &renderer);
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
		if((optimalTimeDrawing-updateTime) > 0) usleep(optimalTimeDrawing-updateTime);

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

void localUpdate(std::mutex *currentlyUpdatingOrDrawingLock, std::vector<StellarObject*> *galaxies, bool *isRunning, bool *isPaused, Date *date, int *optimalTimeLocalUpdate, Renderer *renderer){
	// Create vectors of all objects in the individual systems
	std::vector<std::vector<StellarObject*>> starSystemsToUpdate;
	// Go through all star systems, and if they are not lone stars, add a vector of all objects in the current star system to the starsystemToUpdate
	for(StellarObject *galacticCore: *galaxies){
		for(StellarObject *starSystem: *(galacticCore->getChildren())){
			if((static_cast<StarSystem*>(starSystem))->getLoneStar()) continue;
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
	// printf("Local update spit out %ld vectors\n", starSystemsToUpdate.size());

	// Run the simulation with the appropriate speed
	struct timespec prevTime;
	struct timespec currTime;
	// int optimalTime = 1000000/20;			//In microseconds
	int updateTime;
	while(*isRunning){
		// printf("running\n");
		if(*isPaused){
			usleep(1000000/24);
			continue;
		}
		// printf("Not Paused\n");
		clock_gettime(CLOCK_MONOTONIC, &prevTime);
		for(std::vector<StellarObject*> &objects: starSystemsToUpdate){
			Tree tree(&objects, currentlyUpdatingOrDrawingLock);
			// printf("Create tree\n");
			tree.buildTree();
			tree.update(TIMESTEP_LOCAL, renderer);
			tree.destroyTree();
			// printf("Destroyed tree\n");
		}
		date->incSecond(TIMESTEP_LOCAL);
		// usleep(1000000);


		clock_gettime(CLOCK_MONOTONIC, &currTime);
		updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
		// printf("Updating took %d mics, compared to wanted %d mics. Now sleeping %d\n", updateTime, *optimalTimeLocalUpdate, (*optimalTimeLocalUpdate-updateTime));
        // clock_gettime(CLOCK_MONOTONIC, &prevTime);
		if((*optimalTimeLocalUpdate-updateTime) > 0) usleep(*optimalTimeLocalUpdate-updateTime);
	}	
}

void initialiseStellarObjects(std::vector<StellarObject*> *galaxies, std::vector<StellarObject*> *allObjects){	
	// Initialise all objects
	galaxies->push_back(new GalacticCore("Sagittarius A*", 1, 1, 0, 0xFFFF00));
	// ADD_STARSYSTEM(new StarSystem("Solar System", 1, 0.07, 1.047)); 			// this inclinations is from the solar plan to the galactic plane, thus not of interest
	// ADD_STARSYSTEM(new StarSystem("Solar System", 1, 0.07, 0.1));
	// ADD_STAR(new Star("Sun", 1, 1, 0, 0, 0, 5770));
    // ADD_PLANET(new Planet("Mercury", 0.3829, 0.055, 0.387098, 0.205630, 7.005*PI/180));
    // ADD_PLANET(new Planet("Venus", 0.9499, 0.815, 0.723332, 0.006772, 3.39458*PI/180));

	ADD_STARSYSTEM(new StarSystem("Solar System", 1, 0, 0));
	ADD_STAR(new Star("Sun", 1, 1, 0, 0, 0, 5770));
    ADD_PLANET(new Planet("Earth", 1, 1, 1, 0, 0));
    ADD_MOON(new Moon("Moon", 1, 1, 1, 0, 0));

	// ADD_PLANET(new Planet("Earth", 1, 1, 1, 0.0167086, 0));
    // ADD_MOON(new Moon("Moon", 1, 1, 1, 0.0549, 5.145*PI/180));
    // ADD_PLANET(new Planet("Mars", 0.532, 0.107, 1.52368055, 0.0934, 1.85*PI/180));
    // ADD_MOON(new Moon("Phobos", 11266.7/lunarRadius, 1.0659e16/lunarMass, 9376000/distanceEarthMoon, 0.0151, 26.04*PI/180));
    // ADD_MOON(new Moon("Deimos", 6200/lunarRadius, 1.4762e15/lunarMass, 23463200/distanceEarthMoon, 0.00033, 27.58*PI/180));
    // ADD_PLANET(new Planet("Jupiter", 10.973, 317.8, 5.204, 0.0489, 1.303*PI/180));
	// // Read file of Jupiters moons
    // ADD_PLANET(new Planet("Saturn", 8.552, 95.159, 9.5826, 0.0565, 2.485*PI/180));
	// // Read file of Saturns moons
    // ADD_PLANET(new Planet("Uranus", 25362000/terranRadius, 14.536, 19.19126, 0.04717, 0.773*PI/180));
	// // Read file of Uranus' moons
    // ADD_PLANET(new Planet("Neptune", 24622000/terranRadius, 17.147, 30.07, 0.008678, 1.77*PI/180));
	// // Read file of Neptunes moons
    // ADD_PLANET(new Planet("Pluto", 0.1868, 0.00218, 39.482, 0.2488, 17.16*PI/180));
	// ADD_MOON(new Moon("Charon", 606000/lunarRadius, 1.586e21/lunarMass, 17181000/distanceEarthMoon, 0.0002, 112.783*PI/180));

	// Add random stars - currently orbit is still fixed
	for(int i=0;i<0;i++){
		ADD_STARSYSTEM(new StarSystem());
		// if(i%50000 == 0) printf("Created %d stars\n", i);
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
