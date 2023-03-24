#include <vector>
#include <mutex>
#include "renderer.hpp"
#include "stellarObject.hpp"
#include "date.hpp"

#define ADD_STARSYSTEM(a) galaxies->back()->addChild(a)
#define ADD_STAR(a) galaxies->back()->getChildren()->back()->addChild(a)
#define ADD_PLANET(a) galaxies->back()->getChildren()->back()->getChildren()->back()->addChild(a)
#define ADD_MOON(a) galaxies->back()->getChildren()->back()->getChildren()->back()->getChildren()->back()->addChild(a)
#define ADD_COMET(a) galaxies->back()->getChildren()->back()->getChildren()->back()->addChild(a)

// #define TIMESTEP_LOCAL 3600 // Timestep per update iteration in seconds
#define TIMESTEP_LOCAL 60 // Timestep per update iteration in seconds
// #define TIMESTEP_LOCAL 20 // Timestep per update iteration in seconds
// #define TIMESTEP_LOCAL 1 // Timestep per update iteration in seconds
// #define TIMESTEP_LOCAL 3600*24 // Timestep per update iteration in seconds

void localUpdate(std::mutex *currentlyUpdatingOrDrawingLock, std::vector<StellarObject*> *galaxies, bool *isRunning, bool *isPaused, Date *date, int *optimalTimeLocalUpdate, Renderer *renderer);
void initialiseStellarObjects(std::vector<StellarObject*> *galaxies, std::vector<StellarObject*> *allObjects);