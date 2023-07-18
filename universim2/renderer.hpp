#ifndef RENDERER_HPP
#define RENDERER_HPP
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>
#include <X11/Xlib.h>
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
    // Not sure what this is used for - keep for keepings sake
    Window rootWindow;
    Date *date;

    unsigned int windowWidth;
    unsigned int windowHeight;
    unsigned int borderWidth;
    unsigned int depth;
    int tempX;
    int tempY;

    long double cameraMoveAmount = astronomicalUnit;

    // Vector of stellar objects
    std::vector<StellarObject*> *galaxies;
    std::vector<StellarObject*> *allObjects;
    // Camera
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

    Matrix3d transformationMatrixCameraBasis;
    Matrix3d inverseTransformationMatrixCameraBasis;

    std::vector<DrawObject*> objectsOnScreen;
    std::mutex objectsOnScreenLock;
    std::vector<DrawObject*> dotsOnScreen;
    std::mutex dotsOnScreenLock;
    std::vector<CloseObject*> closeObjects;

    std::mutex *currentlyUpdatingOrDrawingLock;

    int *optimalTimeLocalUpdate;

    u_int8_t rendererThreadCount;

public:
    Renderer(MyWindow *myWindow, std::vector<StellarObject*> *galaxies, std::vector<StellarObject*> *allObjects, Date *date, std::mutex *currentlyUpdatingOrDrawingLock, int *optimalTimeLocalUpdate);

    void drawWaitingScreen();
    void draw();
    void calculateObjectPosition(StellarObject *object, std::vector<DrawObject*> *objectsToAddOnScreen, std::vector<DrawObject*> *dotsToAddOnScreen);
    void calculateCloseObject(StellarObject *object, PositionVector distanceNewBasis, int size);
    void calculateReferenceLinePositions(int x, int y, int size, StellarObject *object);
    void drawObjects();
    void drawUI();
    void drawDrawObjects();
    int drawPoint(unsigned int col, int x, int y);
    int drawLine(unsigned int col, int x1, int y1, int x2, int y2);
    // First point is the one being changed, second point stays the same
    Point2d calculateEdgePointWithOneVisible(int x1, int y1, int x2, int y2);
    // First point is the one being changed, second point stays the same. Returns -1/-1 if line between the points does not intersect the canvas
    Point2d calculateEdgePointWithNoneVisible(int x1, int y1, int x2, int y2);
    int drawRect(unsigned int col, int x, int y, int width, int height);
    int drawCircle(unsigned int col, int x, int y, int diam);
    int drawString(unsigned int col, int x, int y, const char *stringToBe);
    int drawTriangle(unsigned int col, int x1, int y1, int x2, int y2, int x3, int y3);
    int drawPolygon(unsigned int col, short count, Point2d *points, bool checks = false);
    int drawTriangleAllNotVisible(unsigned int col, Point2d *points);
    int drawTriangleTwoNotVisible(unsigned int col, Point2d *points, short indexVisible);
    int drawTriangleOneNotVisible(unsigned int col, Point2d *points, short indexNotVisible);
    void rotateCamera(long double angle, short axis);
    void moveCamera(short direction, short axis);
    void increaseCameraMoveAmount();
    void decreaseCameraMoveAmount();
    void increaseSimulationSpeed();
    void decreaseSimulationSpeed();
    void resetCameraOrientation();

    void centreParent();
    void centreChild();
    void centreNext();
    void centrePrevious();
    void centreNextStarSystem();
    void centrePreviousStarSystem();
    void centreNearest();
    void toggleCentre();

    bool visibleOnScreen(int x, int y);
    void addObjectOnScreen(DrawObject *drawObjects);
    void addDotOnScreen(DrawObject *drawObjects);
    void addObjectsOnScreen(std::vector<DrawObject*> *drawObjects);
    void addDotsOnScreen(std::vector<DrawObject*> *drawObjects);
    // Sorts all objects with index from start to end-1
    void initialiseReferenceObject();
    void adjustThreadCount(int8_t adjustment);
    int getWindowWidth();
    int getWindowHeight();
    Matrix3d getInverseTransformationMatrixCameraBasis();
    PositionVector getCameraPosition();
    StellarObject *getReferenceObject();
    std::vector<DrawObject*> *getObjectsOnScreen();
    std::vector<StellarObject*> *getAllObjects();
    u_int8_t getRendererThreadCount();

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