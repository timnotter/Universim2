#ifndef RENDERER_HPP
#define RENDERER_HPP
// #define CL_TARGET_OPENCL_VERSION 220
// #include <CL/cl.h>
// #include <X11/Xlib.h>
#include <vector>
#include <time.h>
#include <cstring>
#include <atomic>
#include <mutex>
#include "constants.hpp"
#include "window.hpp"
#include "stellarObject.hpp"
#include "starSystem.hpp"
#include "positionVector.hpp"
#include "matrix3d.hpp"
#include "drawObject.hpp"
#include "date.hpp"
#include "stellarObjectRenderFace.hpp"
#include "point2d.hpp"

#define RENDERER_MAX_THREAD_COUNT 8
#define INCREASE_THREAD_COUNT 1
#define DECREASE_THREAD_COUNT -1

#define SCREEN_HEIGHT 600           //Initial values
#define SCREEN_WIDTH 800
#define TOPBAR_HEIGHT 50

#define BACKGROUND_COL 0x111111
#define WHITE_COL 0xFFFFFF
#define RED_COL 0xFF0000
#define GREEN_COL 0x00FF00
#define BLUE_COL 0x0000FF
#define BLACK_COL 0x000000
#define YELLOW_COL 0xFFFF00
#define ORANGE_COL 0xFF7F00
#define GREY_COL 0x888888
#define DARK_GREY_COL 0x666666
#define TRANS_BLUE_COL 0x0000FF01
#define LASTPOS_INIT_NUM 1000000000000000000.0

#define MAX_TRIANGULATION_RESOLUTION 50

#define STANDARD_CAMERA_POSITION -1 * std::min(std::max(2 * centreObject->getRadius(), solarRadius/5), astronomicalUnit)

#define KEY_ESCAPE 9
#define KEY_SPACE 65
#define KEY_Q 24
#define KEY_W 25
#define KEY_E 26
#define KEY_R 27
#define KEY_T 28
#define KEY_Z 29
#define KEY_U 30
#define KEY_I 31
#define KEY_O 32
#define KEY_P 33
#define KEY_A 38
#define KEY_S 39
#define KEY_D 40
#define KEY_F 41
#define KEY_G 42
#define KEY_H 43
#define KEY_J 44
#define KEY_K 45
#define KEY_L 46
#define KEY_OE 47
#define KEY_AE 48
#define KEY_LEFT 113
#define KEY_UP 111
#define KEY_RIGHT 114
#define KEY_DOWN 116
#define KEY_PG_UP 112
#define KEY_PG_DOWN 117
#define KEY_1 10
#define KEY_2 11
#define KEY_3 12
#define KEY_4 13
#define KEY_5 14
#define KEY_6 15
#define KEY_7 16
#define KEY_8 17

static int debug = 0;

typedef struct CloseObject_s{
    StellarObject *object;
    PositionVector distanceNewBasis;
    int size;
} CloseObject;

// Functions for multithreading
void calculateObjectPositionsMultiThread(int start, int amount, Renderer *renderer, int id);
void updatePositionAtPointInTimeMultiThread(std::vector<StellarObject*> *allObjects, int start, int amount);
void updateRenderFaceMultiThread(StellarObjectRenderFace *face, short axis, short direction, short resolution);
void calculateCloseObjectTrianglesMultiThread(Renderer *renderer, StellarObject *object, std::vector<RenderTriangle*> *triangles, DrawObject *drawObject, int start, int amount, std::mutex *drawObjectLock);
void getRenderTrianglesMultiThread(StellarObjectRenderFace *face, std::vector<RenderTriangle*> *triangles, PositionVector absoluteCameraPosition, std::mutex *trianglesLock);

void quicksort(int start, int end, std::vector<DrawObject*> *vector, int usedThreads, int maxThreads);
int quicksortInsert(int start, int end, std::vector<DrawObject*> *vector);

class Renderer{
private:
    MyWindow *myWindow;
    Date *date;

    // Distance one button press moves the camera, when not centring on an object
    long double cameraMoveAmount = astronomicalUnit;

    // Vector of stellar objects
    std::vector<StellarObject*> *galaxies;
    std::vector<StellarObject*> *allObjects;

    // Camera position is always relativ to current reference object
    PositionVector cameraPosition;
    PositionVector cameraPlaneVector1;
    PositionVector cameraPlaneVector2;
    PositionVector cameraDirection;
    long double cameraPlaneEquationParameter4;

    // When an object is centre, camera movements forward cannot "overshoot" object, 
    // because the distance gets multiplied with factor and no flat addition is performed
    StellarObject *centreObject;
    StellarObject *lastCenterObject;        // Currently unused
    // Variables for reference system, such that the camera keeps its position relative to this body
    StellarObject *referenceObject;
    StellarObject *lastReferenceObject;     // Currently unused

    // Used to calculate display position of objects
    Matrix3d transformationMatrixCameraBasis;
    Matrix3d inverseTransformationMatrixCameraBasis;

    // Vectors to store objects (any "normal" structures), dots (distant stars) and closeObjects (triangles of close opjects) 
    // with their respective lock to allow multiple threads to add things
    std::vector<DrawObject*> objectsOnScreen;
    std::mutex objectsOnScreenLock;
    std::vector<DrawObject*> dotsOnScreen;
    std::mutex dotsOnScreenLock;
    std::vector<CloseObject*> closeObjects;

    // Lock to protect the scene from changing while being drawn
    std::mutex *currentlyUpdatingOrDrawingLock;

    int *optimalTimeLocalUpdate;

    int8_t rendererThreadCount;

public:
    Renderer(MyWindow *myWindow, std::vector<StellarObject*> *galaxies, std::vector<StellarObject*> *allObjects, Date *date, std::mutex *currentlyUpdatingOrDrawingLock, int *optimalTimeLocalUpdate);

    // Draws waiting screen
    void drawWaitingScreen();
    // Draws a snapshot of the current scene
    void draw();
    void calculateObjectPosition(StellarObject *object, std::vector<DrawObject*> *objectsToAddOnScreen, std::vector<DrawObject*> *dotsToAddOnScreen);
    void calculateCloseObject(StellarObject *object, PositionVector distanceNewBasis, int size);
    void calculateReferenceLinePositions(int x, int y, int size, StellarObject *object);
    // Blocks the updating procedure and stores current position of all objects by calling updatePositionAtPointInTimeMultiThread,
    // then unblocks updating procedure and calculates the positions of all objects on screen at the snapshottime by calling calculateObjectPositionsMultiThread
    // Objects that are close enough to by drawn with triangles are automatically sorted out and their triangles are now calculated.
    // Lastly, function calls drawdrawObjects, which draws everything
    void drawObjects();
    // Draws UI
    void drawUI();
    // Sorts objectsOnScreen and then draws everything in the following order: dotsOnScreen, objectsOnScreen and then closeObjects
    void drawDrawObjects();
    // Either rotates camera around the centred object or rotates camera normally while not centring anything and recalculates transformation matrices
    void rotateCamera(long double angle, short axis);
    // Either moves camera proportionally to distance to centred object or by fixed amount while not centring anything
    void moveCamera(short direction, short axis);
    // Increases fixed camera move amount while not centring anything
    void increaseCameraMoveAmount();
    // Decreases fixed camera move amount while not centring anything
    void decreaseCameraMoveAmount();
    void increaseSimulationSpeed();
    void decreaseSimulationSpeed();
    // Jumps to reference object, reset camera orientation to standard value and recalculates transformation matrices
    void resetCameraOrientation();

    void centreParent();
    void centreChild();
    // Centring next child of parent
    void centreNext();
    void centrePrevious();
    void centreNextStarSystem();
    void centrePreviousStarSystem();
    void centreNearest();
    // Sets centre object to to reference object while not centring anything or enter "free roam mode" and unset centre object
    void toggleCentre();

    bool visibleOnScreen(int x, int y);
    // Locks and adds single objects to objectsOnScreen vector
    // AVOID due to performance
    void addObjectOnScreen(DrawObject *drawObjects);
    // Locks and adds single objects to dotsOnScreen vector
    // AVOID due to performance
    void addDotOnScreen(DrawObject *drawObjects);
    // Locks and adds all objects to objectsOnScreen vector
    void addObjectsOnScreen(std::vector<DrawObject*> *drawObjects);
    // Locks and adds all objects to dotsOnScreen vector
    void addDotsOnScreen(std::vector<DrawObject*> *drawObjects);

    // Initialises first reference object
    // SHOULD be called by main function after initialising all objects
    void initialiseReferenceObject();
    // Adjusts thread count of the renderer by (adjustment) if threadCount has not reached 1 or RENDERER_MAX_THREAD_COUNT
    void adjustThreadCount(int8_t adjustment);

    // Polls all waiting events and resolves them by calling handleEvent onto them
    void handleEvents(bool &running, bool &isPaused);
    // Calls the underlying windwow function to get the event number and then resolves them
    void handleEvent(void *eventptr, bool &running, bool &isPaused);
    
    // Getter
    int getWindowWidth();
    int getWindowHeight();
    MyWindow *getMyWindow();
    Matrix3d getInverseTransformationMatrixCameraBasis();
    PositionVector getCameraPosition();
    StellarObject *getReferenceObject();
    std::vector<DrawObject*> *getObjectsOnScreen();
    std::vector<StellarObject*> *getAllObjects();
    int8_t getRendererThreadCount();

    // Can be used for debugging
    std::vector<int> dataPoints;
    std::vector<int> dataPoints2;
    std::vector<int> dataPoints3;
};

#endif

// Name comparison template
// std::string *nameComparison = new std::string("Sagittarius A*");
// if(strcmp(object->getName(), nameComparison->c_str()) == 0){
//     printf("%s exited via visibility constraint\n", object->getName());
// }
// delete nameComparison;