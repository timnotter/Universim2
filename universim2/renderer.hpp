#ifndef RENDERER_HPP
#define RENDERER_HPP
#include <vector>
#include <time.h>
#include <cstring>
#include <atomic>
#include <mutex>
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>
#include "constants.hpp"
#include "window.hpp"
#include "stellarObject.hpp"
#include "positionVector.hpp"
#include "matrix3d.hpp"
#include "drawObject.hpp"
#include "date.hpp"

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


#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2
#define FORWARDS 1
#define BACKWARDS -1

static int debug = 0;


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

    double cameraMoveAmount = astronomicalUnit;

    // Vector of stellar objects
    std::vector<StellarObject*> *galaxies;
    std::vector<StellarObject*> *allObjects;
    // Camera
    // Camera position is always relativ to current reference object
    PositionVector cameraPosition;
    PositionVector cameraPlainVector1;
    PositionVector cameraPlainVector2;
    PositionVector cameraDirection;

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

    std::mutex *currentlyUpdatingOrDrawingLock;

    int *optimalTimeLocalUpdate;

public:
    Renderer(MyWindow *myWindow, std::vector<StellarObject*> *galaxies, std::vector<StellarObject*> *allObjects, Date *date, std::mutex *currentlyUpdatingOrDrawingLock, int *optimalTimeLocalUpdate);

    void drawWaitingScreen();
    void draw();
    void calculateObjectPosition(StellarObject *object, std::vector<DrawObject*> *objectsToAddOnScreen, std::vector<DrawObject*> *dotsToAddOnScreen);
    void calculateReferenceLinePositions(int x, int y, int size, StellarObject *object);
    void drawObjects();
    void drawObjectMultiThread(int start, int end);
    void drawUI();
    void drawDrawObjects();
    int drawPoint(unsigned int col, int x, int y);
    int drawLine(unsigned int col, int x1, int y1, int x2, int y2);
    int drawRect(unsigned int col, int x, int y, int width, int height);
    int drawCircle(unsigned int col, int x, int y, int diam);
    int drawString(unsigned int col, int x, int y, const char *stringToBe);
    void rotateCamera(double angle, short axis);
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
    void sortObjectsOnScreen();
    void quicksort(int start, int end);
    int quicksortInsert(int start, int end);
    void initialiseReferenceObject();
};

#endif