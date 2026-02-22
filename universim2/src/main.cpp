#include <chrono>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "graphicInterface/renderer.hpp"
#include "graphicInterface/window.hpp"

#include "stellarObjects/comet.hpp"
#include "stellarObjects/galacticCore.hpp"
#include "stellarObjects/moon.hpp"
#include "stellarObjects/planet.hpp"
#include "stellarObjects/star.hpp"
#include "stellarObjects/starSystem.hpp"
#include "stellarObjects/stellarObject.hpp"

#include "helpers/constants.hpp"
#include "helpers/date.hpp"
#include "helpers/threadUtils.hpp"
#include "helpers/timer.hpp"
#include "helpers/tree.hpp"

#define ADD_STARSYSTEM(a) galaxies->back()->addChild(a)
#define ADD_STAR(a) galaxies->back()->getChildren()->back()->addChild(a)
#define ADD_PLANET(a)                                                          \
    galaxies->back()->getChildren()->back()->getChildren()->back()->addChild(a)
#define ADD_MOON(a)                                                            \
    galaxies->back()                                                           \
        ->getChildren()                                                        \
        ->back()                                                               \
        ->getChildren()                                                        \
        ->back()                                                               \
        ->getChildren()                                                        \
        ->back()                                                               \
        ->addChild(a)
#define ADD_COMET(a)                                                           \
    galaxies->back()->getChildren()->back()->getChildren()->back()->addChild(a)

#define TIMESTEP_LOCAL 120 // Timestep per update iteration in seconds
// #define TIMESTEP_LOCAL 60 // Timestep per update iteration in seconds
// #define TIMESTEP_LOCAL 20 // Timestep per update iteration in seconds
// #define TIMESTEP_LOCAL 1 // Timestep per update iteration in seconds

// Enable this line for local and stellar updates to be handled the same, such
// that we can see stellar movement
// #define TIMESTEP_LOCAL 3600 * 24 * 365 * 100L * 100
#define TIMESTEP_STELLAR 3600 * 24 * 365 * 100L * 100

#define LONESTAR_BACKOFF_AMOUNT 720

#define MICROSECONDS_PER_FRAME 1000000 / 20

void localUpdate(std::mutex *currentlyUpdatingOrDrawingLock,
                 std::vector<StellarObject *> *galaxies,
                 std::vector<StellarObject *> *allObjects, bool *isRunning,
                 bool *isPaused, Date *date, int *optimalTimeLocalUpdate,
                 Renderer *renderer, std::mutex *localUpdateIsReady,
                 std::mutex *stellarUpdateIsReady);
void stellarUpdate(std::mutex *currentlyUpdatingOrDrawingLock,
                   std::vector<StellarObject *> *galaxies, bool *isRunning,
                   Renderer *renderer, std::vector<StellarObject *> *allObjects,
                   std::mutex *localUpdateIsReady,
                   std::mutex *stellarUpdateIsReady);
void initialiseStellarObjects(std::vector<StellarObject *> *galaxies,
                              std::vector<StellarObject *> *allObjects,
                              std::mutex *currentlyUpdatingOrDrawingLock);
void updateLoneStarStarsystemMultiThread(
    std::vector<StellarObject *> *loneStars, int begin, int end,
    long double timestep);
void spawnStarSystemsMultiThread(
    std::vector<StellarObject *> *globalGalaxies, int amount,
    std::mutex *currentlyUpdatingOrDrawingLock,
    std::function<long double(double)> densityFunction);
void readMoonFile(std::string fileLocation, StellarObject *parent);

// #define TOTAL_STARSYSTEMS 100000
#define TOTAL_STARSYSTEMS 50000

#define MOONS_OF_JUPITER_FILE_PATH                                             \
    "/home/tim/programming/cpp/Universim2/universim2/src/files/"               \
    "MoonsOfJupiterAdjusted.csv"
#define MOONS_OF_NEPTUNE_FILE_PATH                                             \
    "/home/tim/programming/cpp/Universim2/universim2/src/files/"               \
    "MoonsOfNeptuneAdjusted.csv"

// #define DEBUG

// -------------------------------------------------------------------
// SUGGESTION
// ------------------------------------------------------------------- Maybe
// store store position of substellar objects in relation to their star system,
// such that we have smaller numbers

// TODO: For background, make array for every pixel and add up brightness?

// TODO: ---------------- INACCURACY ----------------
//       We use 2 different ways to adjust velocity with the momentum of
//       children 1: velocity += (childrenMomentum * -1 / mass); 2: velocity +=
//       (childrenMomentum * -(1/mass)) - ((position / position.getLength()) *
//       (G * totalMass / position.getLength())); Check and unify!!
//		 This has to be further investigated, but it is assumed that the
// first term suffices
//
//       ---------------- Unknown ----------------
//		 We assume every object in a system orbits the same point: the
// centre of mass. I don't actually know if this is universaly applicable

// TODO: For new moon/ssteroid shapes, improve collision detection of camera and
// code a random form generation for small moons and comets

// TODO: Improve lighting: especially if direct sunlight is blocked by a
// generated "mountain"

// TODO: Include multiple rings and improve the generation of them. MeanDistance
// is fixed for this time

// TODO: Points of rings are always rendered as objectsOnScreen. Maybe revert
// this

// TODO: Fading away of stars (maybe to some degree also other objects) is not
// correct

int main(int argc, char **argv) {
    // Initialise base things
    printf("Starting main\n");
    MyWindow myWindow;
    bool isRunning = true;
    bool isPaused = true;
    Date date(1, 1, 2023);
    std::vector<StellarObject *> galaxies = std::vector<StellarObject *>();
    std::vector<StellarObject *> allObjects = std::vector<StellarObject *>();
    std::mutex currentlyUpdatingOrDrawingLock;
    std::mutex localUpdateIsReady;
    std::mutex stellarUpdateIsReady;
    const int optimalTimeDrawing = MICROSECONDS_PER_FRAME; // In microseconds
    int optimalTimeLocalUpdate =
        MICROSECONDS_PER_FRAME *
        2; // In microseconds - I don't know why it has this value
    printf("Started init\n");

    // Start the renderer
    Renderer renderer(&myWindow, &galaxies, &allObjects, &date,
                      &currentlyUpdatingOrDrawingLock, &optimalTimeLocalUpdate);
    printf("Started renderer\n");

    // The window needs a little time to show up properly, don't know why
    renderer.drawWaitingScreen();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    renderer.drawWaitingScreen();
    printf("Drawn first screen\n");

    // Initialise all objects that ought to appear in the simulation
    initialiseStellarObjects(&galaxies, &allObjects,
                             &currentlyUpdatingOrDrawingLock);
    renderer.initialiseReferenceObject();
    printf("Initialised stellar objects\n");

    struct timespec prevTime;
    struct timespec currTime;
    int updateTime;

    // Start gravity calculations concurrently
    // Local updater calculates gravitation between objects within a solar
    // system Stellar updater calculates gravitation between interstellar
    // objects
    std::thread *localUpdater = new std::thread(
        localUpdate, &currentlyUpdatingOrDrawingLock, &galaxies, &allObjects,
        &isRunning, &isPaused, &date, &optimalTimeLocalUpdate, &renderer,
        &localUpdateIsReady, &stellarUpdateIsReady);
    std::thread *stellarUpdater = new std::thread(
        stellarUpdate, &currentlyUpdatingOrDrawingLock, &galaxies, &isRunning,
        &renderer, &allObjects, &localUpdateIsReady, &stellarUpdateIsReady);

    // Displaying and keyhandler loop
    while (isRunning) {
        // Handle events
        getTime(&prevTime, 0);
        renderer.handleEvents(isRunning, isPaused);
        // clock_gettime(CLOCK_MONOTONIC, &currTime);
        // updateTime =
        // ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
        // printf("Handling events took %d mics\n", updateTime);

        // Drawing the screen
        renderer.draw();
        renderer.drawOpenGL();
        // After drawing the screen we calculate the time it took. If it was
        // faster than the framrate permits, the thread goes to sleep for the
        // remaining time. If it was was way to fast, we decrease the number of
        // threads the renderer has available, if it was to slow, we increase it
        // up to a maximum
        getTime(&currTime, 0);
        updateTime = ((1000000000 * (currTime.tv_sec - prevTime.tv_sec) +
                       (currTime.tv_nsec - prevTime.tv_nsec)) /
                      1000);
        // printf("Handling events and drawing took %d mics, compared to wanted
        // %d mics. Now sleeping %d\n", updateTime, optimalTimeDrawing,
        // (optimalTimeDrawing-updateTime));
        // clock_gettime(CLOCK_MONOTONIC, &prevTime);
        int difference = optimalTimeDrawing - updateTime;
        if (difference > 0) {
            if (difference > 15000) {
                renderer.adjustThreadCount(DECREASE_THREAD_COUNT);
            }
            // printf("Trying to sleep for: %d mics\n", updateTime);
            std::this_thread::sleep_for(std::chrono::microseconds(difference));
            // printf("Slept\n");
        } else {
            renderer.adjustThreadCount(INCREASE_THREAD_COUNT);
            // printf("Else: Difference: %d mics\n", difference);
        }
        // printf("Post: Difference: %d mics\n", difference);

        // clock_gettime(CLOCK_MONOTONIC, &currTime);
        // printf("Sleeptime: %dmics\n",
        // ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000));
    }
    renderer.cleanupOpenGL();

    // Recursively free everything
    for (StellarObject *galacticCore : galaxies) {
        delete galacticCore;
    }

    return 0;
}

// Updates the positions and velocites of objects within solar systems
void localUpdate(std::mutex *currentlyUpdatingOrDrawingLock,
                 std::vector<StellarObject *> *galaxies,
                 std::vector<StellarObject *> *allObjects, bool *isRunning,
                 bool *isPaused, Date *date, int *optimalTimeLocalUpdate,
                 Renderer *renderer, std::mutex *localUpdateIsReady,
                 std::mutex *stellarUpdateIsReady) {
    // Create vectors of all objects in the individual systems
    std::vector<std::vector<StellarObject *>> starSystemsToUpdate;
    std::vector<StellarObject *> loneStars;
    // Go through all star systems, and if they are not lone stars, add a vector
    // of all objects in the current star system to the starsystemToUpdate

    for (StellarObject *galacticCore : *galaxies) {
        for (StellarObject *starSystem : *(galacticCore->getChildren())) {
            if ((dynamic_cast<StarSystem *>(starSystem))->getLoneStar()) {
                loneStars.push_back(starSystem);
                continue;
            }
            starSystemsToUpdate.push_back(std::vector<StellarObject *>());

            std::vector<StellarObject *> queue;
            for (StellarObject *current : *(starSystem->getChildren()))
                queue.push_back(current);

            StellarObject *current;
            while (!queue.empty()) {
                current = queue.back();
                queue.pop_back();
                starSystemsToUpdate.back().push_back(current);
                for (StellarObject *children : *(current->getChildren()))
                    queue.push_back(children);
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
    getTime(&initialTime, 0);
    while (*isRunning) {
        // printf("running\n");
        if (*isPaused) {
            std::this_thread::sleep_for(
                std::chrono::microseconds(MICROSECONDS_PER_FRAME));
            continue;
        }

        // This portion is only for testing stellar force calculation
        if (localUpdatesPerStellarUpdate == 1) {
            for (StellarObject *stellarObject : *allObjects) {
                if (stellarObject->getType() == STARSYSTEM) {
                    // stellarObject->updateVelocity(TIMESTEP_LOCAL);
                    continue;
                }
                stellarObject->updateVelocity(TIMESTEP_LOCAL);
                stellarObject->updatePosition(TIMESTEP_LOCAL);
            }
            // printf("Current centre of mass of galaxy %s: (%s)\n",
            // galaxies->at(0)->getName(),
            // galaxies->at(0)->getUpdatedCentreOfMass().toString());
            localUpdateIsReady->unlock();
            stellarUpdateIsReady->lock();
            localUpdateIsReady->lock();
            stellarUpdateIsReady->unlock();
            // date->incYear(100);
            continue;
        }

        // Loop over all starSystems, build their tree and update them
        // clock_gettime(CLOCK_MONOTONIC, &prevTime);
        for (std::vector<StellarObject *> &objects : starSystemsToUpdate) {
            Tree tree(&objects, currentlyUpdatingOrDrawingLock);
            tree.buildTree();
            tree.update(TIMESTEP_LOCAL, renderer);
            tree.destroyTree();
        }
        // clock_gettime(CLOCK_MONOTONIC, &currTime);
        // updateTime =
        // ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
        // printf("Building and updating tree took %d mics\n", updateTime);

        // Updating all lone stars is expensive (there are many) and not really
        // significant, for their position changes are barely visible, even if
        // close, since they have no close objects around them -> we only update
        // them every LONESTAR_BACKOFF_AMOUNT normal update.
        if (loneStarUpdateCounter++ == LONESTAR_BACKOFF_AMOUNT) {
            loneStarUpdateCounter = 0;
            getTime(&prevTime, 0);
            int threadNumber = getThreadCountLocalUpdate();
            int amount = loneStars.size() / threadNumber;
            std::vector<std::thread> threads;
            for (int i = 0; i < threadNumber - 1; i++) {
                threads.push_back(
                    std::thread(updateLoneStarStarsystemMultiThread, &loneStars,
                                i * amount, i * amount + amount,
                                (TIMESTEP_LOCAL * LONESTAR_BACKOFF_AMOUNT)));
            }
            threads.push_back(
                std::thread(updateLoneStarStarsystemMultiThread, &loneStars,
                            (threadNumber - 1) * amount, loneStars.size(),
                            (TIMESTEP_LOCAL * LONESTAR_BACKOFF_AMOUNT)));
            for (int i = 0; i < threadNumber; i++) {
                threads.at(i).join();
            }
            getTime(&currTime, 0);
            updateTime = ((1000000000 * (currTime.tv_sec - prevTime.tv_sec) +
                           (currTime.tv_nsec - prevTime.tv_nsec)) /
                          1000);
            // printf("Updating loneStars took %d mics\n", updateTime);

            for (StellarObject *galacticCore : *galaxies) {
                galacticCore->updateVelocity(TIMESTEP_LOCAL *
                                             LONESTAR_BACKOFF_AMOUNT);
                galacticCore->updatePosition(TIMESTEP_LOCAL *
                                             LONESTAR_BACKOFF_AMOUNT);
            }
        }
        // Incrementing date
        date->incSecond(TIMESTEP_LOCAL);

        // If it is time, wait until the stellar force calculation has finished
        // and insert it's values. From then on use the new stellar acceleration
        if (++stellarUpdateCounter == localUpdatesPerStellarUpdate) {
            stellarUpdateCounter = 0;
            localUpdateIsReady->unlock();
            stellarUpdateIsReady->lock();
            localUpdateIsReady->lock();
            stellarUpdateIsReady->unlock();
        }

        // Time keeping and sleeping, such that desired speed is achieved
        getTime(&currTime, 0);
        updateTime = ((1000000000 * (currTime.tv_sec - initialTime.tv_sec) +
                       (currTime.tv_nsec - initialTime.tv_nsec)) /
                      1000);
        // printf("Updating took %d mics, compared to wanted %d mics. Now
        // sleeping %d\n", updateTime, *optimalTimeLocalUpdate,
        // (*optimalTimeLocalUpdate-updateTime));
        if ((*optimalTimeLocalUpdate - updateTime) > 0)
            std::this_thread::sleep_for(std::chrono::microseconds(
                *optimalTimeLocalUpdate - updateTime));
        getTime(&initialTime, 0);
    }
}

void updateLoneStarStarsystemMultiThread(
    std::vector<StellarObject *> *loneStars, int begin, int end,
    long double timestep) {
    for (int i = begin; i < end; i++) {
        StellarObject *loneStar = loneStars->at(i);
        loneStar->getChildren()->at(0)->updateVelocity(timestep);
        loneStar->getChildren()->at(0)->updatePosition(timestep);
    }
}

// Updates positions and velocities of interstellar objects
void stellarUpdate(std::mutex *currentlyUpdatingOrDrawingLock,
                   std::vector<StellarObject *> *galaxies, bool *isRunning,
                   Renderer *renderer, std::vector<StellarObject *> *allObjects,
                   std::mutex *localUpdateIsReady,
                   std::mutex *stellarUpdateIsReady) {
    // Create vectors of all objects in the current galaxy
    // - Possibly add support for multiple galaxies with
    std::vector<std::vector<StellarObject *>> galaxiesToUpdate;
    // Go through all galaxies and add starsystems to respective vector
    for (StellarObject *galacticCore : *galaxies) {
        if (galacticCore->getChildren()->size() == 0)
            continue;
        galaxiesToUpdate.push_back(std::vector<StellarObject *>());
        galaxiesToUpdate.back().push_back(galacticCore);
        for (StellarObject *starSystem : *(galacticCore->getChildren())) {
            galaxiesToUpdate.back().push_back(starSystem);
        }
    }
    // Store galacitc acceleration inside starsystems, which do not get updated
    // otherwise, then poll them for the local update

    // Run the simulation with the appropriate speed
    struct timespec prevTime;
    struct timespec currTime;
    stellarUpdateIsReady->lock();
    while (*isRunning) {
        getTime(&prevTime, 0);
        for (std::vector<StellarObject *> &galaxy : galaxiesToUpdate) {
            // printf("Updating galaxy\n");
            Tree tree(&galaxy, currentlyUpdatingOrDrawingLock);
            // printf("Init tree\n");
            tree.buildTree(TIMESTEP_STELLAR);
            // printf("Built tree\n");
            tree.update(TIMESTEP_STELLAR, renderer);
            // printf("Updated tree\n");
            tree.destroyTree();
            // printf("Destroyed tree");
        }
        getTime(&currTime, 0);
        int updateTime = ((1000000000 * (currTime.tv_sec - prevTime.tv_sec) +
                           (currTime.tv_nsec - prevTime.tv_nsec)) /
                          1000);
        printf("Stellar force calculation took %d mics\n", updateTime);

        // Wait until local updates have caught up, swap in new acceleration
        // values
        localUpdateIsReady->lock();
        for (StellarObject *stellarObject : *allObjects) {
            stellarObject->updateNewStellarValues();
        }
        stellarUpdateIsReady->unlock();
        localUpdateIsReady->unlock();

        // TODO: What the fuck were you thinking! What is this abomination
        // Make sure the local update thread has picked up the
        // localUpdateIsReady lock
        while (localUpdateIsReady->try_lock()) {
            localUpdateIsReady->unlock();
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
        stellarUpdateIsReady->lock();
        // printf("Full update of future values has been done\n");
    }
}

void initialiseStellarObjects(std::vector<StellarObject *> *galaxies,
                              std::vector<StellarObject *> *allObjects,
                              std::mutex *currentlyUpdatingOrDrawingLock) {
    printf("Entered initStellarObjects function\n");
    // Initialise all objects
    galaxies->push_back(new GalacticCore("Sagittarius A*", 1, 1, 0, 0xFFFF00));
    // ADD_STARSYSTEM(new StarSystem("Solar System", 1, 0.07, 1.047));
    // // this inclinations is from the solar plan to the galactic plane, thus
    // not of interest
    ADD_STARSYSTEM(new StarSystem("Solar System", 1, 0.07, 0.1));
    ADD_STAR(new Star("Sun", 1, 1, 0, 0, 0, 5770));
    ADD_PLANET(new Planet("Mercury", 0.3829, 0.055, 0.387098, 0.205630,
                          7.005 * PI / 180, 0));
    ADD_PLANET(new Planet("Venus", 0.9499, 0.815, 0.723332, 0.006772,
                          3.39458 * PI / 180, 0));
    ADD_PLANET(new Planet("Earth", 1, 1, 1, 0.0167086, 0, 0));
    ADD_MOON(new Moon("Moon", 1, 1, 1, 0.0549, 5.145 * PI / 180));
    ADD_PLANET(new Planet("Mars", 0.532, 0.107, 1.52368055, 0.0934,
                          1.85 * PI / 180, 0));
    ADD_MOON(new Moon("Phobos", 11266.7 / lunarRadius, 1.0659e16 / lunarMass,
                      9376000 / distanceEarthMoon, 0.0151, 26.04 * PI / 180));
    ADD_MOON(new Moon("Deimos", 6200 / lunarRadius, 1.4762e15 / lunarMass,
                      23463200 / distanceEarthMoon, 0.00033, 27.58 * PI / 180));
    ADD_PLANET(new Planet("Jupiter", 10.973, 317.8, 5.204, 0.0489,
                          1.303 * PI / 180, 0));
    // Read file of Jupiters moons
    readMoonFile(MOONS_OF_JUPITER_FILE_PATH, galaxies->back()
                                                 ->getChildren()
                                                 ->back()
                                                 ->getChildren()
                                                 ->back()
                                                 ->getChildren()
                                                 ->back());
    ADD_PLANET(new Planet("Saturn", 8.552, 95.159, 9.5826, 0.0565,
                          2.485 * PI / 180, 1));
    // Read file of Saturns moons
    ADD_PLANET(new Planet("Uranus", 25362000 / terranRadius, 14.536, 19.19126,
                          0.04717, 0.773 * PI / 180, 0));
    // Read file of Uranus' moons
    ADD_PLANET(new Planet("Neptune", 24622000 / terranRadius, 17.147, 30.07,
                          0.008678, 1.77 * PI / 180, 0));
    // readMoonFile("./files/MoonsOfNeptuneAdjusted.csv",
    // galaxies->back()->getChildren()->back()->getChildren()->back()->getChildren()->back());
    // Read file of Neptunes moons
    ADD_PLANET(new Planet("Pluto", 0.1868, 0.00218, 39.482, 0.2488,
                          17.16 * PI / 180, 0));
    ADD_MOON(new Moon("Charon", 606000 / lunarRadius, 1.586e21 / lunarMass,
                      17181000 / distanceEarthMoon, 0.0002,
                      112.783 * PI / 180));

    printf("Added the solar system\n");
    // ADD_STARSYSTEM(new StarSystem("Solar System", 1, 0, 0));
    // ADD_STAR(new Star("Sun", 1, 1, 0, 0, 0, 5770));
    // ADD_PLANET(new Planet("Earth", 1, 1, 1, 0, 0));
    // ADD_MOON(new Moon("Moon", 1, 1, 1, 0, 0));

    // ADD_STARSYSTEM(new StarSystem("Solar System", 0.01, 0, 0));
    // ADD_STAR(new Star("Sun", 1, 1, 0, 0, 0, 5770));

    // Add random stars - currently orbit is still fixed. Is it though? -.-
    int totalStarsystems = TOTAL_STARSYSTEMS;
    int threadNumber;
    int amount;
    long double characteristicScaleLength = 13000 * lightyear;
    // We use a function derived from the cumulative distribution function from
    // hernquist
    auto densityFunction =
        [characteristicScaleLength](double u) -> long double {
        return characteristicScaleLength * (std::sqrt(u) + u) / (1 - u);
    };
    // printf("Defined density functions\n");
    // printf("densityFunction(0.25) = %.0Lf\n", densityFunction(0.25));
    // printf("a = %Lf, perc: %Lf\n", characteristicScaleLength,
    // densityFunction(0.25)/characteristicScaleLength);
    std::vector<std::thread> threads;
    // min, max is macro in windows.h -> replace
    threadNumber = std::min((std::max(totalStarsystems / 10, 16)) / 16,
                            getThreadCountCreate());
    printf("Trying to start %d threads to create universe\n", threadNumber);
    amount = totalStarsystems / threadNumber;
    for (int i = 0; i < threadNumber - 1; i++) {
        // printf("Started thread number %d\n", i);
        threads.push_back(std::thread(spawnStarSystemsMultiThread, galaxies,
                                      amount, currentlyUpdatingOrDrawingLock,
                                      densityFunction));
    }
    threads.push_back(
        std::thread(spawnStarSystemsMultiThread, galaxies,
                    totalStarsystems - (threadNumber - 1) * amount,
                    currentlyUpdatingOrDrawingLock, densityFunction));
    printf("Starting last thread\n");
    for (int i = 0; i < threadNumber; i++) {
        threads.at(i).join();
    }
    printf("Threads completed their task\n");

    // We store every object in a single vector for improved access in certain
    // functions
    for (StellarObject *galacticCore : *galaxies) {
        for (StellarObject *starSystem : *(galacticCore->getChildren())) {
            for (StellarObject *star : *(starSystem->getChildren())) {
                for (StellarObject *planet : *(star->getChildren())) {
                    for (StellarObject *moon : *(planet->getChildren())) {
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
    for (StellarObject *galacticCore : *galaxies) {
        galacticCore->place();
    }
    // printf("Placed everything\n");

    // printf("Distance of moon and earth: %f\n",
    // (galaxies->at(0)->getChildren()->at(0)->getChildren()->at(0)->getChildren()->at(0)->getPosition()
    // -
    // galaxies->at(0)->getChildren()->at(0)->getChildren()->at(0)->getChildren()->at(0)->getChildren()->at(0)->getPosition()).getLength());
    // printf("Distance of moon and CoM earthsystem: %f\n",
    // (galaxies->at(0)->getChildren()->at(0)->getChildren()->at(0)->getChildren()->at(0)->getCentreOfMass()
    // -
    // galaxies->at(0)->getChildren()->at(0)->getChildren()->at(0)->getChildren()->at(0)->getChildren()->at(0)->getPosition()).getLength());
}
void spawnStarSystemsMultiThread(
    std::vector<StellarObject *> *globalGalaxies, int amount,
    std::mutex *currentlyUpdatingOrDrawingLock,
    std::function<long double(double)> densityFunction) {
    // globalGalaxies is called like that, because then we can use the name
    // "galaxies" and don't have to change the define
    std::vector<StellarObject *> *galaxies = new std::vector<StellarObject *>();
    galaxies->push_back(new GalacticCore("", 0, 0, 0));
    for (int i = 0; i < amount; i++) {
        ADD_STARSYSTEM(new StarSystem(densityFunction, 1));
    }

    // We reuse the currentlyUpdatingOrDrawingLock
    currentlyUpdatingOrDrawingLock->lock();
    for (StellarObject *stellarObject : *(galaxies->at(0)->getChildren())) {
        globalGalaxies->at(0)->addChild(stellarObject);
    }
    currentlyUpdatingOrDrawingLock->unlock();
    delete galaxies;
}

void readMoonFile(std::string fileLocation, StellarObject *parent) {
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
    while (!moonFile.eof()) {
        moonFile >> name;
        moonFile >> mass;
        moonFile >> meanDistance;
        moonFile >> period;
        moonFile >> inclination;
        moonFile >> eccentricity;
        moonFile >> radius;
        // counter++;
        // If the period is to short, we ignore the moon, for it would be
        // simulated badly anyways
        if (std::abs(period) < 1)
            continue;
        parent->addChild(new Moon(
            name.c_str(), radius / lunarRadius, mass / lunarMass,
            meanDistance / distanceEarthMoon, eccentricity, inclination));
        moonFile.peek();
        // printf("Read %s, radius: %Lf, mass: %Lf, meanDistance: %Lf,
        // eccentricity: %Lf, inclination: %Lf, period: %Lf\n", name.c_str(),
        // radius/lunarRadius, mass/lunarMass, meanDistance/distanceEarthMoon,
        // eccentricity, inclination, period);
    }
    moonFile.close();
}
