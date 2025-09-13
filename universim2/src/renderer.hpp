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

#include "stellarObjects/stellarObject.hpp"
#include "stellarObjects/starSystem.hpp"

#include "graphicInterface/window.hpp"
#include "graphicInterface/stellarObjectRenderFace.hpp"
#include "graphicInterface/drawObject.hpp"
#include "graphicInterface/point2d.hpp"

#include "helpers/constants.hpp"
#include "helpers/date.hpp"
#include "helpers/positionVector.hpp"
#include "helpers/matrix3d.hpp"

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

// #define MAX_TRIANGULATION_RESOLUTION 50
#define MAX_TRIANGULATION_RESOLUTION 25
// #define MAX_TRIANGULATION_RESOLUTION 15
// #define MAX_TRIANGULATION_RESOLUTION 4

#define STANDARD_CAMERA_POSITION -1 * std::min(std::max(2 * centreObject->getRadius(), solarRadius/5), astronomicalUnit)

// An object that is to close to be drawn as a simple circle gets assigned a CloseObject
typedef struct CloseObject_s{
    StellarObject *object;
    PositionVector distanceNewBasis;
    int size;
} CloseObject;

// Functions for multithreading
// Starts threads to calculate new positions of all objects
void calculateObjectPositionsMultiThread(int start, int amount, Renderer *renderer, int id);
// Starts threads to update a snapshot of all positions. This is used before drawing the canvas
void updatePositionAtPointInTimeMultiThread(std::vector<StellarObject*> *allObjects, int start, int amount);
// Starts threads to calculate all objects rendered as CloseObjects
void calculateCloseObjectTrianglesMultiThread(Renderer *renderer, StellarObject *object, std::vector<RenderTriangle*> *triangles, DrawObject *drawObject, int start, int amount, std::mutex *drawObjectLock);

// Sorts a vector of DrawObject* by their distance to the camera
void quicksort(int start, int end, std::vector<DrawObject*> *vector, int usedThreads, int maxThreads);
// Quicksort helper function
int quicksortInsert(int start, int end, std::vector<DrawObject*> *vector);

class Renderer{
private:
    MyWindow *myWindow;
    Date *date;

    // Distance one button press moves the camera, when not centring on an object
    long double cameraMoveAmount = astronomicalUnit;

    // Vector of all galactic cores. Essentially acts as a tree structure via the children fields
    std::vector<StellarObject*> *galaxies;
    // Vector with all objects
    std::vector<StellarObject*> *allObjects;

    // Camera position is always relativ to current reference object
    PositionVector cameraPosition;
    PositionVector cameraDirection;
    // CameraPlaneVectors are used for matrix transformations to the camera basis
    PositionVector cameraPlaneVector1;
    PositionVector cameraPlaneVector2;

    // Camera is centred in on this object. Camera movement are made while keeping it in the centre
    StellarObject *centreObject;
    StellarObject *lastCenterObject;        // Currently unused. Can be used to cycle back to a previous centre
    // Variables for reference system, such that the camera keeps its position relative to this body, even while the object moves through space
    StellarObject *referenceObject;
    StellarObject *lastReferenceObject;     // Currently unused. Can be used to cycle back to a previous reference object

    // Used to calculate display position of objects
    Matrix3d transformationMatrixCameraBasis;
    Matrix3d inverseTransformationMatrixCameraBasis;

    // Vectors to store objects (any "normal" structures), dots (distant stars) and closeObjects (triangles of close objects) 
    // with their respective lock to allow multiple threads to add things
    std::vector<DrawObject*> objectsOnScreen;
    std::mutex objectsOnScreenLock;
    std::vector<DrawObject*> dotsOnScreen;
    std::mutex dotsOnScreenLock;
    std::vector<CloseObject*> closeObjects;

    // Lock to protect the scene from having different points in time in the same frame
    std::mutex *currentlyUpdatingOrDrawingLock;

    // Pointer to the var, which is stored outside this class
    int *optimalTimeLocalUpdate;

    // Determines how many threads the renderer currently is using
    int8_t rendererThreadCount;

public:
    // This can be toggled to display certain information
    bool test = 0;    
    
    Renderer(MyWindow *myWindow, std::vector<StellarObject*> *galaxies, std::vector<StellarObject*> *allObjects, Date *date, std::mutex *currentlyUpdatingOrDrawingLock, int *optimalTimeLocalUpdate);

    // Draws waiting screen
    void drawWaitingScreen();
    // Draws a snapshot of the current scene
    void draw();
    // Calculates new positions of all objects
    void calculateObjectPosition(StellarObject *object, std::vector<DrawObject*> *objectsToAddOnScreen, std::vector<DrawObject*> *dotsToAddOnScreen);
    // calculates all objects rendered as CloseObjects
    void calculateCloseObject(StellarObject *object, PositionVector distanceNewBasis, int size);
    // Draws coloured lines on the axes of an object
    void calculateReferenceLinePositions(int x, int y, int size, StellarObject *object);
    // Blocks the updating procedure and stores current position of all objects by calling updatePositionAtPointInTimeMultiThread,
    // then unblocks updating procedure and calculates the positions of all objects on screen at the snapshottime by calling calculateObjectPositionsMultiThread
    // Objects that are close enough to by drawn with triangles are automatically sorted out and their triangles are now calculated.
    // Lastly, function calls drawdrawObjects, which draws everything
    void drawObjects();
    // Draws UI
    void drawUI();
    // Draws point, if it is visible
    // The colour should be a value between 0 and 0xFFFFFF. Returns -1 upon failure, 1 if nothing has to be drawn and 0 upon success
    int drawPoint(unsigned int col, int x, int y);
    // Changes line, such that it fits on the screen and then calls the draw function of the window to display it
    // The colour should be a value between 0 and 0xFFFFFF. Returns -1 upon failure, 1 if nothing has to be drawn and 0 upon success
    int drawLine(unsigned int col, int x1, int y1, int x2, int y2);
    // Changes rect, such that it fits on the screen and then calls the draw function of the window to display it
    // The colour should be a value between 0 and 0xFFFFFF. Returns -1 upon failure, 1 if nothing has to be drawn and 0 upon success
    // Checks are NOT implemented-------------------------------------------TODO-------------------------------------------
    int drawRect(unsigned int col, int x, int y, int width, int height);
    // Calls the draw function of the window to display it
    // The colour should be a value between 0 and 0xFFFFFF. Returns -1 upon failure, 1 if nothing has to be drawn and 0 upon success
    // Checks are NOT implemented-------------------------------------------TODO-------------------------------------------
    int drawCircle(unsigned int col, int x, int y, int diam);
    // Displays string if it would be visible
    // The colour should be a value between 0 and 0xFFFFFF. Returns -1 upon failure, 1 if nothing has to be drawn and 0 upon success
    // Checks are NOT implemented-------------------------------------------TODO-------------------------------------------
    int drawString(unsigned int col, int x, int y, const char *stringToBe);
    // Changes triangle, such that it fits on the screen and then calls the draw function of the window to display it
    // The colour should be a value between 0 and 0xFFFFFF. Returns -1 upon failure, 1 if nothing has to be drawn and 0 upon success
    int drawTriangle(unsigned int col, unsigned int colourP1, unsigned int colourP2, unsigned int colourP3, int x1, int y1, int x2, int y2, int x3, int y3);
    // No checks have been implemented. Always draws.
    int drawPolygon(unsigned int col, short count, Point2d *points);
    // Computes Phong shading for polygons, based upon a triangle that is not completely on the canvas.
    // Inputs are points array, size of arrays, original triangle and original triangle colours. Last input is the colour of the triangle if it where to be drawn normally
    int drawPhongPolygonOfTriangle(Point2d *points, int count, Point2d *originalTrianglePoints, unsigned int *colours, unsigned int col);
    // Takes as input two points. The first hast to be off the canvas, the second can be on it. Returns the first input point, moved to the closest canvas intersection.
    // If the intersection does not lie withing the connecting line between p1 and p2, it returns -1/-1
    Point2d calculateEdgePointWithPoint1NotVisible(int x1, int y1, int x2, int y2);

    // Assumes that the the points in the point array are convex and the next is always to the right of the previous
    int drawTriangleOneNotVisible(unsigned int col, Point2d *points, short indexNotVisible, Point2d *originalTrianglePoints, unsigned int *colours);
    // Assumes that the the points in the point array are convex and the next is always to the right of the previous
    int drawTriangleTwoNotVisible(unsigned int col, Point2d *points, short indexVisible, Point2d *originalTrianglePoints, unsigned int *colours);
    // Assumes that the the points in the point array are convex and the next is always to the right of the previous
    int drawTriangleAllNotVisible(unsigned int col, Point2d *points, Point2d *originalTrianglePoints, unsigned int *colours);
    // Checks if coordinates are on the canvas
    bool visibleOnScreen(int x, int y);

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

    // Setting camera to centre the parent of current object
    void centreParent();
    // Setting camera to centre the first child of current object
    void centreChild();
    // Setting camera to centre next child of parent
    void centreNext();
    // Setting camera to centre previous child of parent
    void centrePrevious();
    // Setting camera to centre next starsystem in list
    void centreNextStarSystem();
    // Setting camera to centre previous starsystem in list
    void centrePreviousStarSystem();
    // Setting camera to centre the nearest object
    void centreNearest();
    // Sets centre object to the current reference object while not centring anything or enter "free roam mode" and unset centre object
    void toggleCentre();

    // Locks and adds single objects to objectsOnScreen vector
    // AVOID using this repeatedly for multiple objects due to performance
    void addObjectOnScreen(DrawObject *drawObjects);
    // Locks and adds single objects to dotsOnScreen vector
    // AVOID using this repeatedly for multiple objects due to performance
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

    // Polls all waiting events and resolves them
    void handleEvents(bool &running, bool &isPaused);
    
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
