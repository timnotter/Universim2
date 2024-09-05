#include <vector>
#include <mutex>
#include <functional>
#include "renderer.hpp"
#include "stellarObject.hpp"
#include "date.hpp"

#define ADD_STARSYSTEM(a) galaxies->back()->addChild(a)
#define ADD_STAR(a) galaxies->back()->getChildren()->back()->addChild(a)
#define ADD_PLANET(a) galaxies->back()->getChildren()->back()->getChildren()->back()->addChild(a)
#define ADD_MOON(a) galaxies->back()->getChildren()->back()->getChildren()->back()->getChildren()->back()->addChild(a)
#define ADD_COMET(a) galaxies->back()->getChildren()->back()->getChildren()->back()->addChild(a)

#define TIMESTEP_LOCAL 120 // Timestep per update iteration in seconds
// #define TIMESTEP_LOCAL 60 // Timestep per update iteration in seconds
// #define TIMESTEP_LOCAL 20 // Timestep per update iteration in seconds
// #define TIMESTEP_LOCAL 1 // Timestep per update iteration in seconds

// #define TIMESTEP_LOCAL 3600 * 24 * 365 * 100L
#define TIMESTEP_STELLAR 3600 * 24 * 365 * 100L

#define LONESTAR_BACKOFF_AMOUNT 720

#define MICROSECONDS_PER_FRAME 1000000/20

void localUpdate(std::mutex *currentlyUpdatingOrDrawingLock, std::vector<StellarObject*> *galaxies, std::vector<StellarObject*> *allObjects, bool *isRunning, bool *isPaused, Date *date, int *optimalTimeLocalUpdate, Renderer *renderer, std::mutex *localUpdateIsReady, std::mutex *stellarUpdateIsReady);
void stellarUpdate(std::mutex *currentlyUpdatingOrDrawingLock, std::vector<StellarObject*> *galaxies, bool *isRunning, Renderer *renderer, std::vector<StellarObject*> *allObjects, std::mutex *localUpdateIsReady, std::mutex *stellarUpdateIsReady);
void initialiseStellarObjects(std::vector<StellarObject*> *galaxies, std::vector<StellarObject*> *allObjects, std::mutex *currentlyUpdatingOrDrawingLock);
void updateLoneStarStarsystemMultiThread(std::vector<StellarObject*> *loneStars, int begin, int end, long double timestep);
void spawnStarSystemsMultiThread(std::vector<StellarObject*> *globalGalaxies, int amount, std::mutex *currentlyUpdatingOrDrawingLock, std::function<long double(double)> densityFunction);
void readMoonFile(std::string fileLocation, StellarObject *parent);