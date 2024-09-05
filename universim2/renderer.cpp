// #include <X11/Xlib.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <thread>
#include "renderer.hpp"
#include "star.hpp"
#include "point2d.hpp"
#include "plane.hpp"

Renderer::Renderer(MyWindow *myWindow, std::vector<StellarObject*> *galaxies, std::vector<StellarObject*> *allObjects, Date *date, std::mutex *currentlyUpdatingOrDrawingLock, int *optimalTimeLocalUpdate){
    this->myWindow = myWindow;
    this->galaxies = galaxies;
    this->allObjects = allObjects;
    this->date = date;
    this->currentlyUpdatingOrDrawingLock = currentlyUpdatingOrDrawingLock;
    this->optimalTimeLocalUpdate = optimalTimeLocalUpdate;
    // Initiate camera position
    cameraPosition = PositionVector(10 * astronomicalUnit, 0, 0);
    cameraDirection = PositionVector(-1, 0, 0);
    cameraPlaneVector1 = PositionVector(0, 1, 0);
    cameraPlaneVector2 = PositionVector(0, 0, 1);
    centreObject = NULL;
    lastCenterObject = NULL;
    referenceObject = NULL;
    lastReferenceObject = NULL;

    transformationMatrixCameraBasis = Matrix3d(cameraPlaneVector1, cameraPlaneVector2, cameraDirection);
    inverseTransformationMatrixCameraBasis = transformationMatrixCameraBasis.transpose();
    // printf("Renderer constructor end\n");

    rendererThreadCount = 1;
}

void Renderer::drawWaitingScreen(){
    myWindow->drawBackground(BACKGROUND_COL);

    int windowWidth = myWindow->getWindowWidth();

    // Light gray top bar
    myWindow->drawRect(GREY_COL, 0, 0, windowWidth, TOPBAR_HEIGHT);
    // Dark grey corner top right
    myWindow->drawRect(DARK_GREY_COL, windowWidth-100, 0, 100, TOPBAR_HEIGHT);
    // Border between the top right corner and the top bar
    myWindow->drawLine(BLACK_COL, windowWidth-100, 0, windowWidth-100, TOPBAR_HEIGHT);
    // Dark grey corner top left
    myWindow->drawRect(DARK_GREY_COL, 0, 0, 200, TOPBAR_HEIGHT);
    // Border between the top left corner and the top bar
    myWindow->drawLine(BLACK_COL, 200, 0, 200, TOPBAR_HEIGHT);
    // Border between top bar and main display
    myWindow->drawLine(BLACK_COL, 0, TOPBAR_HEIGHT, windowWidth, TOPBAR_HEIGHT);

    myWindow->drawString(WHITE_COL, 12, 40, "Loading...");

    myWindow->endDrawing();
}

void Renderer::draw(){
    // printf("Trying to draw\n");

    // XGetGeometry(myWindow->getDisplay(), *(myWindow->getWindow()), &rootWindow, &tempX, &tempY, &windowWidth, &windowHeight, &borderWidth, &depth);
    // XSetWindowBackground(myWindow->getDisplay(), *(myWindow->getWindow()), 0x111111);
    myWindow->drawBackground(0x111111);

    drawObjects();
    drawUI();

    // This is a red square in the middle of the screen
    if(test){
        drawLine(0xFF0000, 390, 290, 390, 310);
        drawLine(0xFF0000, 390, 310, 410, 310);
        drawLine(0xFF0000, 410, 310, 410, 290);
        drawLine(0xFF0000, 410, 290, 390, 290);
    }

    // XClearWindow(myWindow->getDisplay(), *(myWindow->getWindow()));          // Not neccessary, probably because of the swap
    // XdbeSwapInfo swap_info = {*(myWindow->getWindow()), 1};
    // XdbeSwapBuffers(myWindow->getDisplay(), &swap_info, 1);
    myWindow->endDrawing();


    // printf("\n-------------------Finished drawing-------------------\n\n");
}

void Renderer::drawObjects(){
    // ------------------------------------------------------------ TODO ------------------------------------------------------------
    // This function should be executed by the GPU!!!!! - Use OpenCL
    // struct timespec prevTime;
	// struct timespec currTime;
	// clock_gettime(CLOCK_MONOTONIC, &prevTime);

    int threadNumber = rendererThreadCount;
    int amount = allObjects->size()/threadNumber;
    std::vector<std::thread> threads;

    // Go through all objects and update their positionAtPointInTime
    // We lock currentlyUpdatingOrDrawingLock, such that the update thread cannot do updates
    currentlyUpdatingOrDrawingLock->lock();
    for(int i=0;i<threadNumber-1;i++){
        threads.push_back(std::thread (updatePositionAtPointInTimeMultiThread, allObjects, i*amount, amount));
    }
    threads.push_back(std::thread (updatePositionAtPointInTimeMultiThread, allObjects, (threadNumber-1)*amount, allObjects->size()-(threadNumber-1)*amount));
    for(int i=0;i<threadNumber;i++){
        threads.at(i).join();
    }
    currentlyUpdatingOrDrawingLock->unlock();
    threads.clear();
    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // printf("Updating all positionAtPointInTime: %ld mics\n", ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000));
    // clock_gettime(CLOCK_MONOTONIC, &prevTime);

    // Then render all with the positionAtPointInTime
    for(int i=0;i<threadNumber-1;i++){
        threads.push_back(std::thread (calculateObjectPositionsMultiThread, i*amount, amount, this, i));
    }
    threads.push_back(std::thread (calculateObjectPositionsMultiThread, (threadNumber-1)*amount, allObjects->size()-(threadNumber-1)*amount, this, threadNumber-1));
    for(int i=0;i<threadNumber;i++){
        threads.at(i).join();
    }
    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // printf("Calculating Image took: %ld mics\n", ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000));
    // clock_gettime(CLOCK_MONOTONIC, &prevTime);

    // Then we render close objects seperately. We don't use multithreading on this level, 
    // we assume there are only few of them 
    // printf("Rendering %ld close objects\n", closeObjects.size());
    for(int i=0;i<closeObjects.size();i++){
        calculateCloseObject(closeObjects[i]->object, closeObjects[i]->distanceNewBasis, closeObjects[i]->size);
        delete closeObjects[i];
    }
    closeObjects.clear();
    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // printf("Calculating close Objects took: %ld mics\n", ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000));
    
    // clock_gettime(CLOCK_MONOTONIC, &prevTime);
    drawDrawObjects();
    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // printf("Sorting and drawing Image took: %ld mics\n", ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000));
}

// ------------------------------------------------------------ TODO FIX PROBLEM ------------------------------------------------------------
// It has to be some kind of overflow, memory leak, memory scarcity. For small number of stars, it doesn't happen
// Thank you for this exact problem discription :)

void Renderer::drawDrawObjects(){
    // Sorting dots takes a comparetively long time. Up to 25ms for 100000 stars.
    // But it fixes flickering of star concentrations

    // struct timespec prevTime;
	// struct timespec currTime;
	// clock_gettime(CLOCK_MONOTONIC, &prevTime);
    quicksort(0, dotsOnScreen.size(), &dotsOnScreen, 1, rendererThreadCount);
    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // printf("Sorting dots took: %ld mics\n", ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000));
    quicksort(0, objectsOnScreen.size(), &objectsOnScreen, 1, rendererThreadCount);
    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // printf("Sorting both vectors took: %ld mics\n", ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000));

    for(DrawObject *drawObject: dotsOnScreen){
        drawObject->draw(this);
    }
    for(DrawObject *drawObject: objectsOnScreen){
        drawObject->draw(this);
    }

    int dotsOnScreenSize = dotsOnScreen.size();
    int objectsOnScreenSize = objectsOnScreen.size();
    for(int i=dotsOnScreenSize-1;i>=0;i--){
        delete dotsOnScreen[i];
    }
    for(int i=objectsOnScreenSize-1;i>=0;i--){
        delete objectsOnScreen[i];
    }
    dotsOnScreen.clear();
    objectsOnScreen.clear();
}

void Renderer::drawUI(){
    int windowWidth = myWindow->getWindowWidth();

    // Light gray top bar
    myWindow->drawRect(GREY_COL, 0, 0, windowWidth, TOPBAR_HEIGHT);
    // Dark grey corner top right
    myWindow->drawRect(DARK_GREY_COL, windowWidth-100, 0, 100, TOPBAR_HEIGHT);
    // Border between the top right corner and the top bar
    myWindow->drawLine(BLACK_COL, windowWidth-100, 0, windowWidth-100, TOPBAR_HEIGHT);
    // Dark grey corner top left
    myWindow->drawRect(DARK_GREY_COL, 0, 0, 200, TOPBAR_HEIGHT);
    // Border between the top left corner and the top bar
    myWindow->drawLine(BLACK_COL, 200, 0, 200, TOPBAR_HEIGHT);
    // Border between top bar and main display
    myWindow->drawLine(BLACK_COL, 0, TOPBAR_HEIGHT, windowWidth, TOPBAR_HEIGHT);

    // Display center object
    std::string toDisplay;
    if(centreObject != NULL){
        toDisplay = std::string(centreObject->getName());
        toDisplay.insert(0, "Centre: ");
        // str().insert(0, "Centre object: ")
        myWindow->drawString(BLACK_COL, 15, 20, toDisplay.c_str());                 // Displays current centre object
    }

    // Display reference object
    if(referenceObject != NULL){
        toDisplay = std::string(referenceObject->getName());
        toDisplay.insert(0, "Reference: ");
        // str().insert(0, "Centre object: ")
        myWindow->drawString(BLACK_COL, 15, 40, toDisplay.c_str());                 // Displays current reference object
    }
    // Display date
    myWindow->drawString(BLACK_COL, windowWidth-75, 25, date->toString(true));
    myWindow->drawString(BLACK_COL, windowWidth-60, 40, date->timeToString());
    // printf("%s\n", date->toString(false));

}

int Renderer::drawPoint(unsigned int col, int x, int y){
    if(col<0 || col>0xFFFFFF) {
        printf("Tried to draw a point with invalid colour: %x\n", col);
        return -1;
    }
    if(x<0||x>myWindow->getWindowWidth()||y<0||y>myWindow->getWindowHeight())
        return 1;
    myWindow->drawPoint(col, x, y);
    return 0;
}

int Renderer::drawLine(unsigned int col, int x1, int y1, int x2, int y2){
    if(col<0 || col>0xFFFFFF) {
        printf("Tried to draw a line with invalid colour: %x\n", col);
        return -1;
    }

    // Make sure no fancy visual bugs get displayed
    int windowWidth = myWindow->getWindowWidth();
    int windowHeight = myWindow->getWindowHeight();
    
    bool p1Visible = visibleOnScreen(x1, y1);
    bool p2Visible = visibleOnScreen(x2, y2);
    Point2d edgePoint;
    // First handle case that both are not visible
    if(!p1Visible && !p2Visible){
        edgePoint = calculateEdgePointWithPoint1NotVisible(x1, y1, x2, y2);
        if(edgePoint.getX() == -1 && edgePoint.getY() == -1) return 1;
        x1 = edgePoint.getX();
        y1 = edgePoint.getY();
        p1Visible = true;
    }

    if(!p1Visible){
        edgePoint = calculateEdgePointWithPoint1NotVisible(x1, y1, x2, y2);
        x1 = edgePoint.getX();
        y1 = edgePoint.getY();
    }
    else if(!p2Visible){
        edgePoint = calculateEdgePointWithPoint1NotVisible(x2, y2, x1, y1);
        x2 = edgePoint.getX();
        y2 = edgePoint.getY();
    }

    myWindow->drawLine(col, x1, y1, x2, y2);
    return 0;
}

int Renderer::drawRect(unsigned int col, int x, int y, int width, int height){
    if(col<0 || col>0xFFFFFF) {
        printf("Tried to draw a rect with invalid colour: %x\n", col);
        return -1;
    }
    return myWindow->drawRect(col, x, y, width, height);
}

int Renderer::drawCircle(unsigned int col, int x, int y, int diam){
    if(col<0 || col>0xFFFFFF) {
        printf("Tried to draw a circle with invalid colour: %x\n", col);
        return -1;
    }
    return myWindow->drawCircle(col, x, y, diam);
}

int Renderer::drawString(unsigned int col, int x, int y, const char *stringToBe){
    if(col<0 || col>0xFFFFFF) {
        printf("Tried to draw a string with invalid colour: %x\n", col);
        return -1;
    }
    return myWindow->drawString(col, x, y, stringToBe);
}

int Renderer::drawTriangle(unsigned int col, unsigned int colourP1, unsigned int colourP2, unsigned int colourP3, int x1, int y1, int x2, int y2, int x3, int y3){
    if(col<0 || col>0xFFFFFF) {
        printf("Tried to draw a triangle with invalid colour: %x\n", col);
        return -1;
    }

    // printf("Trying to draw triangle with points: (%d, %d), (%d, %d), (%d, %d)\n", x1, y1, x2, y2, x3, y3);
    Point2d points[3];
    points[0] = Point2d(x1, y1);
    points[1] = Point2d(x2, y2);
    points[2] = Point2d(x3, y3);
    Point2d originalTrianglePoints[3];
    originalTrianglePoints[0] = Point2d(x1, y1);
    originalTrianglePoints[1] = Point2d(x2, y2);
    originalTrianglePoints[2] = Point2d(x3, y3);
    unsigned int colours[3];
    colours[0] = colourP1;
    colours[1] = colourP2;
    colours[2] = colourP3;
    // printf("Trying to draw triangle with pointsInXpoint: (%d, %d), (%d, %d), (%d, %d)\n", points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);

    // Here we make sure that the next point is always to the right of the previous. If we have this guarantee, drawing becomes way simpler
    // We transform the coordinate system to one with 0/0 on the bottom left.
    // We then check crossProduct of vector P1-P0 and P2-P0. If it is positive, P2-P0 is to the left. Thus we switch P1 and P2
    int windowHeight = myWindow->getWindowHeight();
    int p0Xt = points[0].getX();
    int p0Yt = windowHeight - points[0].getY();
    int p1Xt = points[1].getX();
    int p1Yt = windowHeight - points[1].getY();
    int p2Xt = points[2].getX();
    int p2Yt = windowHeight - points[2].getY();
    Point2d p0p1 = Point2d(p1Xt - p0Xt, p1Yt - p0Yt);
    Point2d p0p2 = Point2d(p2Xt - p0Xt, p2Yt - p0Yt);
    double crossProduct = p0p1.getX() * p0p2.getY() - p0p1.getY() * p0p2.getX();
    if(crossProduct > 0){
        // If crossProduct is negative, we flip the two points
        // printf("Positive crossproduct p0: (%d, %d), p1: (%d, %d), p2: (%d, %d). p0p1: (%d, %d), p0p2: (%d, %d). CrossProduct: %f\n", points[0].getX(), points[0].getY(), points[1].getX(), points[1].getY(), points[2].getX(), points[2].getY(), p0p1.getX(), p0p1.getY(), p0p2.getX(), p0p2.getY(), crossProduct);
        // printf("p0X: %d, p1X: %d, p0p1X: %d, p0p2X: %d\n", points[0].getX(), points[1].getX(), p0p1.getX(), p0p2.getX());
        // printf("p0Y: %d, p1Y: %d, p0p1Y: %d, p0p2Y: %d\n", points[0].getY(), points[1].getY(), p0p1.getY(), p0p2.getY());
        Point2d tempPoint = points[1];
        points[1] = points[2];
        points[2] = tempPoint;

        tempPoint = originalTrianglePoints[1];
        originalTrianglePoints[1] = originalTrianglePoints[2];
        originalTrianglePoints[2] = tempPoint;

        unsigned int tempColour = colours[1];
        colours[1] = colours[2];
        colours[2] = tempColour;
    }
    // printf("Reordered and now drawing triangle: (%d, %d), (%d, %d), (%d, %d)\n", 
    // points[0].getX(), points[0].getY(), points[1].getX(), points[1].getY(), points[2].getX(), points[2].getY());

    bool p1Visible = visibleOnScreen(points[0].getX(), points[0].getY());
    bool p2Visible = visibleOnScreen(points[1].getX(), points[1].getY());
    bool p3Visible = visibleOnScreen(points[2].getX(), points[2].getY());
    if(!p1Visible && !p2Visible && !p3Visible){
        return drawTriangleAllNotVisible(col, points, originalTrianglePoints, colours);
        // return 1;
    }
    if(!p1Visible && !p2Visible){
        return drawTriangleTwoNotVisible(col, points, 2, originalTrianglePoints, colours);
        // return 1;
    }
    if(!p1Visible && !p3Visible){
        return drawTriangleTwoNotVisible(col, points, 1, originalTrianglePoints, colours);
        // return 1;
    }
    if(!p3Visible && !p2Visible){
        return drawTriangleTwoNotVisible(col, points, 0, originalTrianglePoints, colours);
        // return 1;
    }
    if(!p1Visible){
        // printf("Trying to draw triangle with points: (%d, %d), (%d, %d), (%d, %d)\n", x1, y1, x2, y2, x3, y3);
        return drawTriangleOneNotVisible(col, points, 0, originalTrianglePoints, colours);
        // return 1;
    }
    if(!p2Visible){
        // printf("Trying to draw triangle with points: (%d, %d), (%d, %d), (%d, %d)\n", x1, y1, x2, y2, x3, y3);
        return drawTriangleOneNotVisible(col, points, 1, originalTrianglePoints, colours);
        // return 1;
    }
    if(!p3Visible){
        // printf("Trying to draw triangle with points: (%d, %d), (%d, %d), (%d, %d)\n", x1, y1, x2, y2, x3, y3);
        return drawTriangleOneNotVisible(col, points, 2, originalTrianglePoints, colours);
        // return 1;
    }
    return drawPhongPolygonOfTriangle(points, 3, points, colours, col);
    // return myWindow->drawTriangle(col, points[0].getX(), points[0].getY(), points[1].getX(), points[1].getY(), points[2].getX(), points[2].getY());
    // return 1;
}

int Renderer::drawPolygon(unsigned int col, short count, Point2d *points){
    return myWindow->drawPolygon(col, count, points);
}

int Renderer::drawPhongPolygonOfTriangle(Point2d *points, int count, Point2d *originalTrianglePoints, unsigned int *colours, unsigned int col){
    // printf("Drawing triangles with colours: (%x, %x, %x)\n", colours[0], colours[1], colours[2]);

    if(count<=2){
        printf("drawPhongPolygonOfTriangle: trying to draw polygon with only %d points\n", count);
        return -1;
    }

    // First we go through all colours. If they are the same, we just draw the polygon with one colour instead of every pixel
    unsigned int firstColour = *(colours);
    for(int i=1;i<count;i++){
        if(firstColour != *(colours+i)){
            goto normal;
        }
    }
    // printf("ALl colours of phong polygon are the same: %d\n", firstColour);
    return myWindow->drawPolygon(firstColour, count, points);

    normal:

    // We go through a box and check every point if it is inside our triangle.
    // We do this by calculating the barycentric coordinates. If all of them lie between 0 and 1, the point is inside.
    // We then use the barycentric coordinates to calculate the colour of every pixel
    int minX = 100000;
    int maxX = 0;
    int minY = 100000;
    int maxY = 0;
    for(int i=0;i<count;i++){
        minX = std::min(minX, points[i].getX());
        maxX = std::max(maxX, points[i].getX());
        minY = std::min(minY, points[i].getY());
        maxY = std::max(maxY, points[i].getY());
    }
    long double lambda1;
    long double lambda2;
    long double lambda3;
    int x1 = originalTrianglePoints[0].getX();
    int y1 = originalTrianglePoints[0].getY();
    int x2 = originalTrianglePoints[1].getX();
    int y2 = originalTrianglePoints[1].getY();
    int x3 = originalTrianglePoints[2].getX();
    int y3 = originalTrianglePoints[2].getY();
    for(int x=minX;x<=maxX;x++){
        for(int y=minY;y<=maxY;y++){
            long double devider = ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3)) * 1.0;
            lambda1 = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / devider;
            lambda2 = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / devider;
            lambda3 = 1 - lambda1 - lambda2;
            double epsilon = 0.000001;
            if(lambda1 >= 0 - epsilon && lambda1 <= 1 + epsilon && lambda2 >= 0 - epsilon && lambda2 <= 1 + epsilon && lambda3 >= 0 - epsilon && lambda3 <= 1 + epsilon){
                if(test == 1 && (lambda1 >= 1 - epsilon || lambda2 >= 1 - epsilon || lambda3 >= 1 - epsilon)){
                    myWindow->drawPoint(0x00FF00, x, y);
                    continue;
                }
                int redP1 = colours[0] >> 16;
                int greenP1 = (colours[0] >> 8) & (0b11111111);
                int blueP1 = colours[0] & (0b11111111);
                int redP2 = colours[1] >> 16;
                int greenP2 = (colours[1] >> 8) & (0b11111111);
                int blueP2 = colours[1] & (0b11111111);
                int redP3 = colours[2] >> 16;
                int greenP3 = (colours[2] >> 8) & (0b11111111);
                int blueP3 = colours[2] & (0b11111111);
                // Make sure that the colour is at least the background colour
                int colour = ((int)((redP1 * lambda1 + redP2 * lambda2 + redP3 * lambda3)) << 16)
                + ((int)((greenP1 * lambda1 + greenP2 * lambda2 + greenP3 * lambda3)) << 8)
                + (int)((blueP1 * lambda1 + blueP2 * lambda2 + blueP3 * lambda3));
                // if(x==400 && y==300 && (lambda1 == 1 || lambda2 == 1 || lambda3 == 1)){
                //     printf("point (%d, %d) has some lambda = 1. Colour: %x\n", x, y, colour);
                // }
                myWindow->drawPoint(colour, x, y);
                // if(x == 400 && y == 300){
                //     printf("For triangle (%d, %d), (%d, %d), (%d, %d) and point (%d, %d), the lambdas are %Lf, %Lf, %Lf.\nWith initial colours: %x, %x, %x, this leads to colour %x\n", x1, y1, x2, y2, x3, y3, x, y, lambda1, lambda2, lambda3, colours[0], colours[1], colours[2], colour);
                // }
            }
            else continue;
        }
    }
    return 0;
}

Point2d Renderer::calculateEdgePointWithPoint1NotVisible(int x1, int y1, int x2, int y2){
    // printf("Calculating edge point with p1 visible: (%d, %d), (%d, %d)\n", x1, y1, x2, y2);
    int windowWidth = myWindow->getWindowWidth();
    int windowHeight = myWindow->getWindowHeight();

    Point2d output(-1, -1);
    if(x1 == x2 && y1 == y2){
        return output;
    }
    // We initialise output as -1/-1. If we return that, we know that the function has failed, meaning that the line does not intersect the canvas

    // First we find out which edge is crossed first. We build the line equation and fill in the appropriate values, with which we hit one of the edges
    // x = x1 + s * (x2 - x1), y = y1 + s * (y2 - y1)
    // If the line intersects the respective edge with an s larger than 1, we know the intersection does not lie inbetween the two points, and is therefore discarded
    double edgesS[4];
    if(x2 - x1 != 0){
        edgesS[0] = (0 - x1) / (x2 - x1 + 0.0);                 // X=0 edge
        edgesS[1] = (windowWidth - x1) / (x2 - x1 + 0.0);       // X=windowWidth edge
    }
    else{
        edgesS[0] = 2;
        edgesS[1] = 2;
    }
    if(y2 - y1 != 0){
        edgesS[2] = (0 - y1) / (y2 - y1 + 0.0);                 // Y=0 edge
        edgesS[3] = (windowHeight - y1) / (y2 - y1 + 0.0);      // Y=windowHeight edge
    }
    else{
        edgesS[2] = 2;
        edgesS[3] = 2;
    }
    // The closest of these points, with s being positive, is the new point
    if(edgesS[0]<0) edgesS[0] = 2;
    if(edgesS[1]<0) edgesS[1] = 2;
    if(edgesS[2]<0) edgesS[2] = 2;
    if(edgesS[3]<0) edgesS[3] = 2;
    double smallestS = std::min(std::min(std::min(edgesS[0], edgesS[1]), edgesS[2]), edgesS[3]);
    // If this closest s is larger than 1, we don't have to draw anything
    if(smallestS>1){
        // printf("------------------------------------------------------\n");
        // printf("Searching for intersection between (%d, %d) and (%d, %d)\n", x1, y1, x2, y2);
        // printf("(0 - y1) / (y2 - y1 + 0.0): %f\n", (0 - y1) / (y2 - y1 + 0.0));
        // printf("Returning, smallestS > 1. Ses are: %f, %f, %f, %f\n", edgesS[0], edgesS[1], edgesS[2], edgesS[3]);
        // printf("------------------------------------------------------\n");
        return output;
    }
    // Else select the smallest s than leads to a visible point
    int newX = -1;
    int newY = -1;
    double currentS = 2;
    for(int i=0;i<4;i++){
        if(edgesS[i]>1) continue;
        int tempX = x1 + edgesS[i] * (x2 - x1);
        int tempY = y1 + edgesS[i] * (y2 - y1);
        if(edgesS[i]<currentS && visibleOnScreen(tempX, tempY)){
            newX = tempX;
            newY = tempY;
            currentS = edgesS[i];
            // printf("Took edge number %d\n", i);
        }
    }
    // printf("Decided on final S: %f, new Point: (%d, %d)\n", currentS, newX, newY);
    if(currentS > 1){
        // printf("Returning, currentS > 1\n");
        return output;
    }
    // Now replace the point (x1,y1) with the newly found point, which means p1 is now visible
    output.setX(newX);
    output.setY(newY);
    // myWindow->drawLine(0xFF0000, newX, newY, x2, y2);
    // printf("Calculated edge point with none visible. Returning: (%d, %d)\n", output.getX(), output.getY());
    if(output.getX() == -1 && output.getY() == -1){
        printf("Output still (-1/-1). Ses are: %f, %f, %f, %f\n", edgesS[0], edgesS[1], edgesS[2], edgesS[3]);
    }
    return output;
}

int Renderer::drawTriangleAllNotVisible(unsigned int col, Point2d *points, Point2d *originalTrianglePoints, unsigned int *colours){
    // printf("Drawing all not visible with points: (%s), (%s), (%s)\n", points[0].toString(), points[1].toString(), points[2].toString());
    int windowWidth = myWindow->getWindowWidth();
    int windowHeight = myWindow->getWindowHeight();


    // Checks if all triangle is trivially not on canvas 
    if((points[0].getX()<0 && points[1].getX()<0 && points[2].getX()<0) ||
    (points[0].getY()<0 && points[1].getY()<0 && points[2].getY()<0) ||
    (points[0].getX()>windowWidth && points[1].getX()>windowWidth && points[2].getX()>windowWidth) ||
    (points[0].getY()>windowHeight && points[1].getY()>windowHeight && points[2].getY()>windowHeight)){
        return 1;
    }

    // Check if they are all on the same point
    // if(points[0] == points[1] && points[0] == points[2]){
    //     drawPoint(col, points[0].getX(), points[0].getY());
    //     return 0;
    // }
    
    // printf("Drawing all not visible with points: (%d, %d), (%d, %d), (%d, %d)\n", points[0].getX(), points[0].getY(), points[1].getX(), points[1].getY(), points[2].getX(), points[2].getY());

    std::vector<Point2d> drawPointsVector;
    Point2d drawPoints[7];
    Point2d edgePoint;
    Point2d edgePoint2;
    // Only relevant for case where only one line intersects canvas
    short indexOfPointNotUsed;
    // First we go through all three point pairs and check if the line between them intersects the canvas (twice)
    // If yes, we add both intersectionpoints to the drawPoints array
    for(int i=0;i<3;i++){
        // printf("Calling Function from AllNotVisible\n");
        edgePoint = calculateEdgePointWithPoint1NotVisible(points[i].getX(), points[i].getY(), points[(i+1)%3].getX(), points[(i+1)%3].getY());
        // printf("Finished\n");
        // printf("Edge point i = %d: (%d, %d)\n", i, edgePoint.x, edgePoint.y);
        if(edgePoint.getY() == -1 && edgePoint.getY() == -1) continue;
        // printf("Calling Function from AllNotVisible\n");
        edgePoint2 = calculateEdgePointWithPoint1NotVisible(points[(i+1)%3].getX(), points[(i+1)%3].getY(), edgePoint.getX(), edgePoint.getY());
        // printf("Finished\n");
        drawPointsVector.push_back(edgePoint);
        drawPointsVector.push_back(edgePoint2);
        indexOfPointNotUsed = (i+2)%3;
    }
    // If only one line intersected the canvas, we know that we have to add at least one canvas corner, so that we can draw a triangle
    // If we have no intersections, we don't draw the triangle, else we draw the polygon with the found points on the edges
    int drawPointsVectorSize = drawPointsVector.size();
    if(drawPointsVectorSize == 0) {
        // printf("No intersection line exists\n");
        return 1;
    }
    // else{
    //     printf("Intersection points generated. Points are: ");
    //     for(int i=0;i<drawPointsVector.size()-1;i++){
    //         printf("(%s), ", drawPointsVector[i].toString());
    //     }
    //     printf("(%s)\n", drawPointsVector[drawPointsVector.size()-1].toString());
    // }

    Point2d corners[4];
    corners[0] = Point2d(windowWidth, 0);
    corners[1] = Point2d(windowWidth, windowHeight);
    corners[2] = Point2d(0, windowHeight);
    corners[3] = Point2d(0, 0);

    // If the line from an intersection to a point is on another canvas edge than the next intersection point, we need to add all the corners inbetween

    int arrayIndex = 0;
    for(int pointIndex = 1;pointIndex<drawPointsVectorSize;pointIndex+=2){
        drawPoints[arrayIndex++] = drawPointsVector[pointIndex-1];
        drawPoints[arrayIndex++] = drawPointsVector[pointIndex];

        int p1X = drawPointsVector[pointIndex].getX();
        int p1Y = drawPointsVector[pointIndex].getY();
        int p2X = drawPointsVector[(pointIndex+1)%drawPointsVectorSize].getX();
        int p2Y = drawPointsVector[(pointIndex+1)%drawPointsVectorSize].getY();

        if((p1X == p2X && p2X == 0) || (p1X == p2X && p2X == windowWidth) || (p1Y == p2Y && p2Y == 0) || (p1Y == p2Y && p2Y == windowHeight)){
            // Here they are on the same edge, thus we add nothing
            continue;
        }
        else{
            // Here they are not on the same edge
            // printf("We want to draw a triangle with no visible points and we would have to add an edge!!!\n");
            // Because in the beginning we made sure that the next point is always to the right of the previous, we can just cycle through the corners clockwise
            short startIndex;
            if(p1Y == 0) startIndex = 0;
            else if(p1X == windowWidth) startIndex = 1;
            else if(p1Y == windowHeight) startIndex = 2;
            else startIndex = 3;
            short endIndex;
            if(p2Y == 0) endIndex = 0;
            else if(p2X == windowWidth) endIndex = 1;
            else if(p2Y == windowHeight) endIndex = 2;
            else endIndex = 3;

            for(int i = startIndex; i != endIndex; i=((i+1)%4)){
                drawPoints[arrayIndex++] = corners[i];
            }
        }
    }

    // printf("Converted all not visible to polygon. Points are: ");
    // for(int i=0;i<arrayIndex-1;i++){
    //     printf("(%s), ", drawPoints[i].toString());
    // }
    // printf("(%s). ArrayIndex = %d\n", drawPoints[arrayIndex-1].toString(), arrayIndex);

    // printf("Drawn all not visible with points: (%s), (%s), (%s)--------------------\n", points[0].toString(), points[1].toString(), points[2].toString());
    // printf("Left drawTriangleAllNotVisible\n");

    return drawPhongPolygonOfTriangle(drawPoints, arrayIndex, originalTrianglePoints, colours, col);
    // return myWindow->drawPolygon(col, arrayIndex, drawPoints);
}

int Renderer::drawTriangleTwoNotVisible(unsigned int col, Point2d *points, short indexVisible, Point2d *originalTrianglePoints, unsigned int *colours){
    // printf("Entered drawTriangleTwoNotVisible\n");
    // printf("Drawing two not visible with points: (%s), (%s), (%s)\n", points[0].toString(), points[1].toString(), points[2].toString());
    int windowWidth = myWindow->getWindowWidth();
    int windowHeight = myWindow->getWindowHeight();
    Point2d drawPoints[6];
    drawPoints[0] = points[(indexVisible)];
    // First we find the points which intersect
    // printf("Calling Function from TwoNotVisible\n");
    Point2d intersect1 = calculateEdgePointWithPoint1NotVisible(points[(indexVisible+1)%3].getX(), points[(indexVisible+1)%3].getY(), points[indexVisible].getX(), points[indexVisible].getY());
    // printf("Finished\n");
    // printf("Calling Function from TwoNotVisible\n");
    Point2d intersect2 = calculateEdgePointWithPoint1NotVisible(points[(indexVisible+2)%3].getX(), points[(indexVisible+2)%3].getY(), points[indexVisible].getX(), points[indexVisible].getY());
    // printf("Finished\n");

    if(intersect1.getX() == intersect2.getX() || intersect1.getY() == intersect2.getY()){
        // printf("Case two points not visible, two intersection points, which are on the same edge\n");
        // This means the intersection points are on the same edge
        drawPoints[0] = points[indexVisible];
        drawPoints[1] = intersect1;
        drawPoints[2] = intersect2;
        drawPhongPolygonOfTriangle(drawPoints, 3, originalTrianglePoints, colours, col);
        // myWindow->drawTriangle(col, points[(indexVisible)].getX(), points[(indexVisible)].getY(), intersect1.getX(), intersect1.getY(), intersect2.getX(), intersect2.getY());
        // myWindow->drawTriangle(col, points[(indexVisible)].getX(), points[(indexVisible)].getY(), intersect1.getX(), intersect1.getY(), intersect2.getX(), intersect2.getY());
        return 0;
    }
    else{
        // Intersections points are not on the same edge. Now we have to check if the line between the invisible points intersects the canvas
        // printf("Calling Function from TwoNotVisible again\n");
        Point2d intersect3 = calculateEdgePointWithPoint1NotVisible(points[(indexVisible+1)%3].getX(), points[(indexVisible+1)%3].getY(), points[(indexVisible+2)%3].getX(), points[(indexVisible+2)%3].getY());
        // printf("Finished\n");
        // printf("Intersectionpoint of invisible points: (%d, %d)\n", intersect3.getX(), intersect3.getY());
        if(intersect3.getX() != -1 && intersect3.getY() != -1){
            // printf("Case two points not visible, four intersection points, which are not on the same edge\n");
            // Case line intersects canvas
            // printf("Calling Function from TwoNotVisible again\n");
            Point2d intersect4 = calculateEdgePointWithPoint1NotVisible(points[(indexVisible+2)%3].getX(), points[(indexVisible+2)%3].getY(), intersect3.getX(), intersect3.getY());
            // printf("Finished\n");
            drawPoints[1] = intersect1;
            drawPoints[2] = intersect3;
            drawPoints[3] = intersect4;
            drawPoints[4] = intersect2;

            drawPhongPolygonOfTriangle(drawPoints, 5, originalTrianglePoints, colours, col);
            // myWindow->drawPolygon(col, 5, drawPoints);
            // printf("Drawn two not visible with points: (%d, %d), (%d, %d), (%d, %d)--------------------\n", points[0].getX(), points[0].getY(), points[1].getX(), points[1].getY(), points[2].getX(), points[2].getY());
            return 0;
        }
        else{
            // This would also work if points are on the same canvas edge
            Point2d corners[4];
            corners[0] = Point2d(windowWidth, 0);
            corners[1] = Point2d(windowWidth, windowHeight);
            corners[2] = Point2d(0, windowHeight);
            corners[3] = Point2d(0, 0);
            // printf("Corners: (%d, %d), (%d, %d), (%d, %d), (%d, %d)\n", corners[0].x, corners[0].y, corners[1].x, corners[1].y, corners[2].x, corners[2].y, corners[3].x, corners[3].y);

            short startIndex;
            if(intersect1.getY() == 0) startIndex = 0;
            else if(intersect1.getX() == windowWidth) startIndex = 1;
            else if(intersect1.getY() == windowHeight) startIndex = 2;
            else startIndex = 3;
            short endIndex;
            if(intersect2.getY() == 0) endIndex = 0;
            else if(intersect2.getX() == windowWidth) endIndex = 1;
            else if(intersect2.getY() == windowHeight) endIndex = 2;
            else endIndex = 3;

            drawPoints[1] = intersect1;

            int drawPointsIndex = 2;
            for(int i = startIndex; i != endIndex; i=((i+1)%4)){
                drawPoints[drawPointsIndex++] = corners[i];
            }
            drawPoints[drawPointsIndex++] = intersect2;
            
            drawPhongPolygonOfTriangle(drawPoints, drawPointsIndex, originalTrianglePoints, colours, col);
            // myWindow->drawPolygon(col, drawPointsIndex, drawPoints);
        }
        // printf("Drawn two not visible with points: (%s), (%s), (%s)--------------------\n", points[0].toString(), points[1].toString(), points[2].toString());
        return 0;
    }
}

int Renderer::drawTriangleOneNotVisible(unsigned int col, Point2d *points, short indexNotVisible, Point2d *originalTrianglePoints, unsigned int *colours){
    // printf("Drawing one not visible with points: (%s), (%s), (%s)\n", points[0].toString(), points[1].toString(), points[2].toString());
    int windowWidth = myWindow->getWindowWidth();
    int windowHeight = myWindow->getWindowHeight();
    Point2d drawPoints[5];
    drawPoints[0] = points[(indexNotVisible+1)%3];
    drawPoints[1] = points[(indexNotVisible+2)%3];
    // First we find the points which intersect
    // printf("Calling Function from OneNotVisible\n");
    Point2d intersect1 = calculateEdgePointWithPoint1NotVisible(points[indexNotVisible].getX(), points[indexNotVisible].getY(), points[(indexNotVisible+1)%3].getX(), points[(indexNotVisible+1)%3].getY());
    // printf("Finished\n");
    // printf("Calling Function from OneNotVisible\n");
    Point2d intersect2 = calculateEdgePointWithPoint1NotVisible(points[indexNotVisible].getX(), points[indexNotVisible].getY(), points[(indexNotVisible+2)%3].getX(), points[(indexNotVisible+2)%3].getY());
    // printf("Finished\n");
    // printf("One Point not visible intersection points: (%d, %d), (%d, %d)\n", intersect1.x, intersect1.y, intersect2.x, intersect2.y);
    if(intersect1.getX() == intersect2.getX() || intersect1.getY() == intersect2.getY()){
        // printf("Case one point not visible, two intersection points, which are on the same edge\n");
        // This means the intersection points are on the same edge
        drawPoints[2] = intersect2;
        drawPoints[3] = intersect1;
        // printf("Drawing polygon: (%d, %d), (%d, %d), (%d, %d), (%d, %d)\n", drawPoints[0].x, drawPoints[0].x, drawPoints[1].x, drawPoints[1].y, drawPoints[2].x, drawPoints[2].y, drawPoints[3].x, drawPoints[3].y);
        drawPhongPolygonOfTriangle(drawPoints, 4, originalTrianglePoints, colours, col);
        // myWindow->drawPolygon(col, 4, drawPoints);
        // printf("Drawn one not visible with points: (%d, %d), (%d, %d), (%d, %d)--------------------\n", points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
        return 0;
    }
    else if(intersect1.getX() == 0 || intersect1.getX() == windowWidth){
        drawPoints[2] = intersect2;
        Point2d corner(intersect1.getX(), intersect2.getY());
        drawPoints[3] = corner;
        drawPoints[4] = intersect1;
        // printf("Case one point not visible, two intersection points, which are not on the same edge, corner %d, %d included\n", corner.x, corner.y);
        // printf("Intersect points: (%d, %d), (%d, %d)\n", intersect1.x, intersect1.y, intersect2.x, intersect2.y);
    }
    else{
        if(intersect1.getX() == -1 || intersect2.getX() == -1){
            printf("Having problems trying to draw: (%d, %d), (%d, %d), (%d, %d). Index not visible: %d\nIntersections: (%d, %d), (%d, %d)\n---------------------------\n", 
            points[0].getX(), points[0].getY(), points[1].getX(), points[1].getY(), points[2].getX(), points[2].getY(), indexNotVisible, intersect1.getX(), intersect1.getY(), intersect2.getX(), intersect2.getY());
        }
        drawPoints[2] = intersect2;
        Point2d corner(intersect2.getX(), intersect1.getY());
        drawPoints[3] = corner;
        drawPoints[4] = intersect1;
        // printf("Case one point not visible, two intersection points, which are not on the same edge, corner %d, %d included\n", corner.x, corner.y);
    }
    // printf("Drawing polygon: (%d, %d), (%d, %d), (%d, %d), (%d, %d), (%d, %d)\n", drawPoints[0].x, drawPoints[0].x, drawPoints[1].x, drawPoints[1].y, drawPoints[2].x, drawPoints[2].y, drawPoints[3].x, drawPoints[3].y, drawPoints[4].x, drawPoints[4].y);
    drawPhongPolygonOfTriangle(drawPoints, 5, originalTrianglePoints, colours, col);
    // myWindow->drawPolygon(col, 5, drawPoints);
    // printf("Drawn one not visible with points: (%s), (%s), (%s)--------------------\n", points[0].toString(), points[1].toString(), points[2].toString());
    return 0;
}

void Renderer::calculateObjectPosition(StellarObject *object, std::vector<DrawObject*> *objectsToAddOnScreen, std::vector<DrawObject*> *dotsToAddOnScreen){
    int windowWidth = myWindow->getWindowWidth();
    int windowHeight = myWindow->getWindowHeight();
    
    
    // First we calculate the distancevector from camera to object
    // PositionVector distance = object->getPosition() - referenceObject->getPosition() - cameraPosition; // This would be convenient but is comparatively much to slow
    PositionVector distance = PositionVector(object->getPositionAtPointInTime().getX()-referenceObject->getPositionAtPointInTime().getX()-cameraPosition.getX(), object->getPositionAtPointInTime().getY()-referenceObject->getPositionAtPointInTime().getY()-cameraPosition.getY(), object->getPositionAtPointInTime().getZ()-referenceObject->getPositionAtPointInTime().getZ()-cameraPosition.getZ());

    // Calculate size in pixels on screen
    long double size = object->getRadius() / distance.getLength() * (windowWidth/2);
    double sizeCutOff = 1.0/1000000000000;
    if(size < sizeCutOff){
        // printf("Exited calculateObjectPosition\n");
        return;
    }
    // if(object->getType() != GALACTIC_CORE && object->getType() != STARSYSTEM && object->getType() != STAR){
    //     printf("Rendering %s\n", object->getName());
    // }

    // We then make a basistransformation with the cameraDirection and cameraPlaneVectors
    // PositionStandardBasis = TransformationMatrix * PositionNewBasis
    // Because the basis is orthonormal, the inverse is just the transpose
    // -> InverseTransformationMatrix * PositionStandardBasis = PositionNewBasis
    PositionVector distanceNewBasis = inverseTransformationMatrixCameraBasis * distance;

    // If the z-coordinate is negative, it is definitely out of sight, namely behind the camera
    if(distanceNewBasis.getZ()<-object->getRadius()){
        return;
    }

    // Calculate position of center on screen
    int x;
    int y;
    if(centreObject == object){
        x = (windowWidth/2);
        y = (windowHeight/2);
    }
    else{
        // ----------------- Distance calculation is numerically unstable, especially for referenceObject == object -----------------
        x = (windowWidth/2) + std::floor((windowWidth/2) * distanceNewBasis.getX() / distanceNewBasis.getZ());
        y = (windowHeight/2) + std::floor((windowWidth/2) * distanceNewBasis.getY() / distanceNewBasis.getZ());
    }

    

    // If no part of the object would be on screen, we dont draw it - only applicable for objects that are far away -> small
    // Cutoff of 5 chosesn arbitrarily
    if(size < 5 && (x - size > windowWidth || x + size < 0 || y - size > windowHeight || y + size < TOPBAR_HEIGHT)){
        return;
    }

    // if(object->getType() != GALACTIC_CORE && object->getType() != STARSYSTEM && object->getType() != STAR){
    //     printf("Displaying %s with size of %.13Lf\n", object->getName(), size);
    // }

    // ------------------------------------------------- TODO -------------------------------------------------
    int colour = object->getColour();
    float dotProductNormalVector;
    if(object->getType() != GALACTIC_CORE && object->getType() != STAR){
        colour = object->getHomeSystem()->getChildren()->at(0)->getColour();
        dotProductNormalVector = ((((object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - object->getPositionAtPointInTime()).normalise().dotProduct(((cameraPosition + referenceObject->getPositionAtPointInTime()) - object->getPositionAtPointInTime()).normalise()) + 1) / 2) + 1) / 2;
        // printf("DotProduct for %s is %f\n", object->getName(), dotProductNormalVector);
    }
    // Used such that the "plus" fades into a point
    int originalColour = colour;

    // ------------------------------------------------- Maybe use apparent brightness for realistic fading away -------------------------------------------------
    bool isPoint = false;
    int red = colour >> 16;
    int green = (colour >> 8) & (0b11111111);
    int blue = colour & (0b11111111);
    // Make sure that the colour is at least the background colour
    int backgroundRed = BACKGROUND_COL >> 16;
    int backgroundGreen = (BACKGROUND_COL >> 8) & (0b11111111);
    int backgroundBlue = BACKGROUND_COL & (0b11111111);
    // if(object->getType()==STAR) printf("Before - Colour: %d, r: %d, g: %d, b: %d, size: %f\n", colour, red, green, blue, size);
    
    // This determines from when on an object gets rendered as a close object, a plus or a 
    int plusCutOff = 1;
    double dotCutOffShining = 1.0/1000000;
    double dotCutOffNonShining = 1.0/100;
    if(object->getType() == GALACTIC_CORE || object->getType() == STAR){
        // Objects that shine by themselves fade away differentely
        if(size < dotCutOffShining){
            isPoint = true;
            // colour = ((int)(red*(1 + std::log(1000*size/2+0.5))) << 16) + ((int)(green*(1 + std::log(1000*size/2+0.5))) << 8) + (int)(blue*(1 + std::log(1000*size/2+0.5)));
            colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*(1 + std::log((1/dotCutOffShining)*size/2+0.0001)/log((1/dotCutOffShining))))) << 16)
            + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*(1 + std::log((1/dotCutOffShining)*size/2+0.0001)/log((1/dotCutOffShining))))) << 8)
            + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*(1 + std::log((1/dotCutOffShining)*size/2+0.0001)/log((1/dotCutOffShining)))));
            // if(object->getType()==STAR) printf("After - Colour: %d, r: %d, g: %d, b: %d, size: %f\n", colour, (int)(red*(1 + std::log(1000*size/2+0.5))), (int)(green*(1 + std::log(1000*size/2+0.5))), (int)(blue*(1 + std::log(1000*size/2+0.5))), size);
        }
        else if(size < plusCutOff){
            colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*(1 + std::log(size/(2*plusCutOff)+0.5)))) << 16)
            + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*(1 + std::log(size/(2*plusCutOff)+0.5)))) << 8)
            + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*(1 + std::log(size/(2*plusCutOff)+0.5))));
            // if(object->getType()==STAR) printf("After - Colour: %d, r: %d, g: %d, b: %d, size: %f\n", colour, (int)(red*(1 + std::log(size/2+0.5)))<<16, (int)(green*(1 + std::log(size/2+0.5)))<<8, (int)(blue*(1 + std::log(size/2+0.5))), size);
            originalColour = ((int)(backgroundRed + (std::max(0, red-backgroundRed))) << 16)
            + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen))) << 8)
            + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)));
        }
        else{
            // Object is displayed as a sphere. The colour is not used
        }
    }
    else{
        if(size < 1.0/10000){
            // Object is to small to be displayed
            // printf("Exited calculateObjectPosition\n");
            return;
        }
        // DotProductNormalForm does not work as intended
        else if(size < dotCutOffNonShining){
            // Object is displayed as point
            isPoint = true;
            colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*(1 + std::log((1/dotCutOffNonShining)*size/2+0.5))*dotProductNormalVector)) << 16)
            + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*(1 + std::log((1/dotCutOffNonShining)*size/2+0.5))*dotProductNormalVector)) << 8)
            + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*(1 + std::log((1/dotCutOffNonShining)*size/2+0.5))*dotProductNormalVector));
        }
        else if(size < plusCutOff){
            // Object is displayed as a plus. Colour is the colour of the plus itself, originalColour is the colour of the centre point
            // The centre point should have the same colour as the singular point, at the distance where they change
            
            // We want the originalColour (colour of the centre dot) to be inbetween it's own colour and the suns colour
            int sunColour = object->getHomeSystem()->getChildren()->at(0)->getColour();
            int sunRed = sunColour >> 16;
            int sunGreen = (sunColour >> 8) & (0b11111111);
            int sunBlue = sunColour & (0b11111111);
            sunRed = (std::max(0, sunRed-backgroundRed)*dotProductNormalVector);
            sunGreen = (std::max(0, sunGreen-backgroundGreen)*dotProductNormalVector);
            sunBlue = (std::max(0, sunBlue-backgroundBlue)*dotProductNormalVector);
            
            int ownColour = object->getColour();
            int ownRed = ownColour >> 16;
            int ownGreen = (ownColour >> 8) & (0b11111111);
            int ownBlue = ownColour & (0b11111111);
            ownRed = (std::max(0, ownRed-backgroundRed)*dotProductNormalVector);
            ownGreen = (std::max(0, ownGreen-backgroundGreen)*dotProductNormalVector);
            ownBlue = (std::max(0, ownBlue-backgroundBlue)*dotProductNormalVector);

            double factorOwnColour = ((((size) / (plusCutOff))));
            double factorSunColour = 1 - factorOwnColour;
            red = ((int)(backgroundRed + factorOwnColour * ownRed + factorSunColour * sunRed)) << 16;
            green = ((int)(backgroundGreen + factorOwnColour * ownGreen + factorSunColour * sunGreen)) << 8;
            blue = backgroundBlue + factorOwnColour * ownBlue + factorSunColour * sunBlue;
            // printf("FactorOwnColour: %f, FactorSunColour: %f\n", factorOwnColour, factorSunColour);
            // printf("Own colour: %x, %x, %x. Sun colour: %x, %x, %x. Res colour: %x, %x, %x\n", ownRed, ownGreen, ownBlue, sunRed, sunGreen, sunBlue, red, green, blue);

            originalColour = red + green + blue;

            //We fade the outer colour from its own colour to background
            colour = ((int)(backgroundRed + (std::max(0, ownRed-backgroundRed)*(1 + std::log(size/(2*plusCutOff)+0.5))*dotProductNormalVector)) << 16)
            + ((int)(backgroundGreen + (std::max(0, ownGreen-backgroundGreen)*(1 + std::log(size/(2*plusCutOff)+0.5))*dotProductNormalVector)) << 8)
            + (int)(backgroundBlue + (std::max(0, ownBlue-backgroundBlue)*(1 + std::log(size/(2*plusCutOff)+0.5))*dotProductNormalVector));


            // printf("%s - Size: %Lf. OwnColour: %x. SunColour: %x. factorOwnColour: %f. factorSunColour: %f. Original colour: %x\n", object->getName(), size, ownColour, sunColour, factorOwnColour, factorSunColour, originalColour);
        }
        else{
            // Object is displayed as a sphere. The colour is not used
            // colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*dotProductNormalVector)) << 16)
            // + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*dotProductNormalVector)) << 8)
            // + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*dotProductNormalVector));
        }
    }

    // if(object->getType() != GALACTIC_CORE && object->getType() != STARSYSTEM && object->getType() != STAR){
    //     printf("Determined colour of %s\n", object->getName());
    // }

    // printf("Colour and size of %s: %d, %f\n", object->getName(), colour, size);


    // For dots, lines and plus', we have to decrement the coordinates, else they don't coincide with the triangulation approach (idk why)
    DrawObject *drawObject;
    if(isPoint){
        // x--;y--;
        if(visibleOnScreen(x, y)) {
            drawObject = new DrawObject(colour, x, y, distanceNewBasis.getLength(), POINT);

            // -------------------------------------------------- EXPERIMENTAL --------------------------------------------------
            // We put asteroids to the normal objects, since they can be infront of planets
            if(object->getType() == 6){
                objectsToAddOnScreen->push_back(drawObject);
                return;
            }

            dotsToAddOnScreen->push_back(drawObject);

        }
        // printf("Exited calculateObjectPosition\n");
    }
    else if(size < plusCutOff){
        // x--;y--;
        // drawObject = new DrawObject(colour, x, y, distanceNewBasis.getLength(), PLUS);
        // dotsToAddOnScreen->push_back(drawObject);
        // drawObject = new DrawObject(originalColour, x, y, distanceNewBasis.getLength()-1, POINT);
        // dotsToAddOnScreen->push_back(drawObject);
        drawObject = new DrawObject(colour, x, y, distanceNewBasis.getLength(), PLUS);
        objectsToAddOnScreen->push_back(drawObject);
        drawObject = new DrawObject(originalColour, x, y, distanceNewBasis.getLength()-1, POINT);
        objectsToAddOnScreen->push_back(drawObject);
    }
    // else if(size < circleCutOff){
    //     x--;y--;
    //     drawObject = new DrawObject(colour, x, y, size, distanceNewBasis.getLength(), CIRCLE);
    //     objectsToAddOnScreen->push_back(drawObject);
    // }
    else{
        // Close objects are rendered seperately, to harness the full power of multithreading
        CloseObject *closeObject = new CloseObject();
        closeObject->object = object;
        closeObject->distanceNewBasis = distanceNewBasis;
        closeObject->size = size;
        closeObjects.push_back(closeObject);
        // calculateCloseObject(object, distanceNewBasis, size, objectsToAddOnScreen);
    }

    // Draw reference lines
    // if(referenceObject == object && size>3)
    //     calculateReferenceLinePositions(x, y, size, object);
    // printf("Exited calculateObjectPosition\n");
}

void Renderer::calculateCloseObject(StellarObject *object, PositionVector distanceNewBasis, int size){
    // struct timespec prevTime;
	// struct timespec currTime;
    // struct timespec beginTime;
    // int updateTime;
    // clock_gettime(CLOCK_MONOTONIC, &beginTime);

    int resolution = std::min(std::max((int)(size), 5), MAX_TRIANGULATION_RESOLUTION);
    // int resolution = std::min(std::max((int)(size/10), 5), 25);
    if(resolution % 2 == 0) resolution++;
    // resolution = 3;
    // printf("Resolution: %d\n", resolution);
    // First initialise the 6 different faces of the planet

    
    PositionVector absoluteCameraPosition = cameraPosition + referenceObject->getPositionAtPointInTime();

    StellarObjectRenderFace *faces = object->getRenderFaces();
    faces->updateRenderFaces(resolution);
    std::vector<RenderTriangle*> triangles;
    faces->getRenderTriangles(&triangles, absoluteCameraPosition, cameraDirection);

    // Initialise drawObject as group and store all triangles of the object in it. We sort it individualy
    DrawObject *drawObject = new DrawObject(distanceNewBasis.getLength(), GROUP);

    // Then loop over all triangles, calculate their position and add fitting draw objects
    // printf("Triangles size: %lu\n", triangles.size());

    // clock_gettime(CLOCK_MONOTONIC, &prevTime);

    std::mutex drawObjectLock;
    // printf("Using %d threads for close objects\n", rendererThreadCount);
    int threadNumber = rendererThreadCount;
    int amount = triangles.size()/threadNumber;
    std::vector<std::thread> threads;
    for(int i=0;i<threadNumber-1;i++){
        threads.push_back(std::thread(calculateCloseObjectTrianglesMultiThread, this, object, &triangles, drawObject, i*amount, amount, &drawObjectLock));
    }
    threads.push_back(std::thread(calculateCloseObjectTrianglesMultiThread, this, object, &triangles, drawObject, (threadNumber-1)*amount, triangles.size()-(threadNumber-1)*amount, &drawObjectLock));
    for(int i=0;i<threadNumber;i++){
        threads.at(i).join();
    }
    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
    // printf("Rendering all triangles took %d mics\n", updateTime);

    objectsOnScreen.push_back(drawObject);
    triangles.clear();    

    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // updateTime = ((1000000000*(currTime.tv_sec-beginTime.tv_sec)+(currTime.tv_nsec-beginTime.tv_nsec))/1000);
    // printf("Total function took %d mics\n", updateTime);
}

void Renderer::calculateReferenceLinePositions(int x, int y, int size, StellarObject *object){
    int windowWidth = myWindow->getWindowWidth();
    int windowHeight = myWindow->getWindowHeight();
    // First we find out the positions on screen of the endpoints of the reference lines
    PositionVector point1XAxis = PositionVector(-3 * object->getRadius() + object->getPositionAtPointInTime().getX(), 0 + object->getPositionAtPointInTime().getY(), 0 + object->getPositionAtPointInTime().getZ());
    PositionVector point2XAxis = PositionVector(3 * object->getRadius() + object->getPositionAtPointInTime().getX(), 0 + object->getPositionAtPointInTime().getY(), 0 + object->getPositionAtPointInTime().getZ());
    PositionVector point1ObjectSurfaceXAxis = PositionVector(-object->getRadius() + object->getPositionAtPointInTime().getX(), 0 + object->getPositionAtPointInTime().getY(), 0 + object->getPositionAtPointInTime().getZ());
    PositionVector point2ObjectSurfaceXAxis = PositionVector(object->getRadius() + object->getPositionAtPointInTime().getX(), 0 + object->getPositionAtPointInTime().getY(), 0 + object->getPositionAtPointInTime().getZ());

    PositionVector point1YAxis = PositionVector(0 + object->getPositionAtPointInTime().getX(), -3 * object->getRadius() + object->getPositionAtPointInTime().getY(), 0 + object->getPositionAtPointInTime().getZ());
    PositionVector point2YAxis = PositionVector(0 + object->getPositionAtPointInTime().getX(), 3 * object->getRadius() + object->getPositionAtPointInTime().getY(), 0 + object->getPositionAtPointInTime().getZ());
    PositionVector point1ObjectSurfaceYAxis = PositionVector(0 + object->getPositionAtPointInTime().getX(), -object->getRadius() + object->getPositionAtPointInTime().getY(), 0 + object->getPositionAtPointInTime().getZ());
    PositionVector point2ObjectSurfaceYAxis = PositionVector(0 + object->getPositionAtPointInTime().getX(), object->getRadius() + object->getPositionAtPointInTime().getY(), 0 + object->getPositionAtPointInTime().getZ());

    PositionVector point1ZAxis = PositionVector(0 + object->getPositionAtPointInTime().getX(), 0 + object->getPositionAtPointInTime().getY(), -3 * object->getRadius() + object->getPositionAtPointInTime().getZ());
    PositionVector point2ZAxis = PositionVector(0 + object->getPositionAtPointInTime().getX(), 0 + object->getPositionAtPointInTime().getY(), 3 * object->getRadius() + object->getPositionAtPointInTime().getZ());
    PositionVector point1ObjectSurfaceZAxis = PositionVector(0 + object->getPositionAtPointInTime().getX(), 0 + object->getPositionAtPointInTime().getY(), -object->getRadius() + object->getPositionAtPointInTime().getZ());
    PositionVector point2ObjectSurfaceZAxis = PositionVector(0 + object->getPositionAtPointInTime().getX(), 0 + object->getPositionAtPointInTime().getY(), object->getRadius() + object->getPositionAtPointInTime().getZ());

    // Calculate distance vectors in respect to the basis vectors of the camera
    PositionVector absoluteCameraPosition = cameraPosition + referenceObject->getPositionAtPointInTime();

    PositionVector distanceNewBasisP1XAxis;
    PositionVector distanceNewBasisP2XAxis;
    PositionVector distanceNewBasisP1ObjectSurfaceXAxis;
    PositionVector distanceNewBasisP2ObjectSurfaceXAxis;

    PositionVector distanceNewBasisP1YAxis;
    PositionVector distanceNewBasisP2YAxis;
    PositionVector distanceNewBasisP1ObjectSurfaceYAxis;
    PositionVector distanceNewBasisP2ObjectSurfaceYAxis;

    PositionVector distanceNewBasisP1ZAxis;
    PositionVector distanceNewBasisP2ZAxis;
    PositionVector distanceNewBasisP1ObjectSurfaceZAxis;
    PositionVector distanceNewBasisP2ObjectSurfaceZAxis;
    if(referenceObject != object){
        distanceNewBasisP1XAxis = inverseTransformationMatrixCameraBasis * (point1XAxis - absoluteCameraPosition);
        distanceNewBasisP2XAxis = inverseTransformationMatrixCameraBasis * (point2XAxis - absoluteCameraPosition);
        distanceNewBasisP1ObjectSurfaceXAxis = inverseTransformationMatrixCameraBasis * (point1ObjectSurfaceXAxis - absoluteCameraPosition);
        distanceNewBasisP2ObjectSurfaceXAxis = inverseTransformationMatrixCameraBasis * (point2ObjectSurfaceXAxis - absoluteCameraPosition);

        distanceNewBasisP1YAxis = inverseTransformationMatrixCameraBasis * (point1YAxis - absoluteCameraPosition);
        distanceNewBasisP2YAxis = inverseTransformationMatrixCameraBasis * (point2YAxis - absoluteCameraPosition);
        distanceNewBasisP1ObjectSurfaceYAxis = inverseTransformationMatrixCameraBasis * (point1ObjectSurfaceYAxis - absoluteCameraPosition);
        distanceNewBasisP2ObjectSurfaceYAxis = inverseTransformationMatrixCameraBasis * (point2ObjectSurfaceYAxis - absoluteCameraPosition);

        distanceNewBasisP1ZAxis = inverseTransformationMatrixCameraBasis * (point1ZAxis - absoluteCameraPosition);
        distanceNewBasisP2ZAxis = inverseTransformationMatrixCameraBasis * (point2ZAxis - absoluteCameraPosition);
        distanceNewBasisP1ObjectSurfaceZAxis = inverseTransformationMatrixCameraBasis * (point1ObjectSurfaceZAxis - absoluteCameraPosition);
        distanceNewBasisP2ObjectSurfaceZAxis = inverseTransformationMatrixCameraBasis * (point2ObjectSurfaceZAxis - absoluteCameraPosition);
    }
    else{
        // This is used to reduce numerical instability, if the objects in question is the reference object
        distanceNewBasisP1XAxis = inverseTransformationMatrixCameraBasis * (PositionVector(-3 * object->getRadius(), 0, 0) - cameraPosition);
        distanceNewBasisP2XAxis = inverseTransformationMatrixCameraBasis * (PositionVector(3 * object->getRadius(), 0, 0) - cameraPosition);
        distanceNewBasisP1ObjectSurfaceXAxis = inverseTransformationMatrixCameraBasis * (PositionVector(-1 * object->getRadius(), 0, 0) - cameraPosition);
        distanceNewBasisP2ObjectSurfaceXAxis = inverseTransformationMatrixCameraBasis * (PositionVector(1 * object->getRadius(), 0, 0) - cameraPosition);

        distanceNewBasisP1YAxis = inverseTransformationMatrixCameraBasis * (PositionVector(0, -3 * object->getRadius(), 0) - cameraPosition);
        distanceNewBasisP2YAxis = inverseTransformationMatrixCameraBasis * (PositionVector(0, 3 * object->getRadius(), 0) - cameraPosition);
        distanceNewBasisP1ObjectSurfaceYAxis = inverseTransformationMatrixCameraBasis * (PositionVector(0, -1 * object->getRadius(), 0) - cameraPosition);
        distanceNewBasisP2ObjectSurfaceYAxis = inverseTransformationMatrixCameraBasis * (PositionVector(0, 1 * object->getRadius(), 0) - cameraPosition);

        distanceNewBasisP1ZAxis = inverseTransformationMatrixCameraBasis * (PositionVector(0, 0, -3 * object->getRadius()) - cameraPosition);
        distanceNewBasisP2ZAxis = inverseTransformationMatrixCameraBasis * (PositionVector(0, 0, 3 * object->getRadius()) - cameraPosition);
        distanceNewBasisP1ObjectSurfaceZAxis = inverseTransformationMatrixCameraBasis * (PositionVector(0, 0, -1 * object->getRadius()) - cameraPosition);
        distanceNewBasisP2ObjectSurfaceZAxis = inverseTransformationMatrixCameraBasis * (PositionVector(0, 0, 1 * object->getRadius()) - cameraPosition);
    }

    // Calculate the positions on screen
    int px1X = (windowWidth/2) + std::floor(distanceNewBasisP1XAxis.getX() / std::abs(distanceNewBasisP1XAxis.getZ()) * (windowWidth/2));
    int py1X = (windowHeight/2) + std::floor(distanceNewBasisP1XAxis.getY() / std::abs(distanceNewBasisP1XAxis.getZ()) * (windowWidth/2));
    int px2X = (windowWidth/2) + std::floor(distanceNewBasisP2XAxis.getX() / std::abs(distanceNewBasisP2XAxis.getZ()) * (windowWidth/2));
    int py2X = (windowHeight/2) + std::floor(distanceNewBasisP2XAxis.getY() / std::abs(distanceNewBasisP2XAxis.getZ()) * (windowWidth/2));
    int px1OSX = (windowWidth/2) + std::floor(distanceNewBasisP1ObjectSurfaceXAxis.getX() / std::abs(distanceNewBasisP1ObjectSurfaceXAxis.getZ()) * (windowWidth/2));
    int py1OSX = (windowHeight/2) + std::floor(distanceNewBasisP1ObjectSurfaceXAxis.getY() / std::abs(distanceNewBasisP1ObjectSurfaceXAxis.getZ()) * (windowWidth/2));
    int px2OSX = (windowWidth/2) + std::floor(distanceNewBasisP2ObjectSurfaceXAxis.getX() / std::abs(distanceNewBasisP2ObjectSurfaceXAxis.getZ()) * (windowWidth/2));
    int py2OSX = (windowHeight/2) + std::floor(distanceNewBasisP2ObjectSurfaceXAxis.getY() / std::abs(distanceNewBasisP2ObjectSurfaceXAxis.getZ()) * (windowWidth/2));
    // printf("One line should go from (%d, %d) to (%d, %d)\n", px1X, py1X, px1OSX, py1OSX);

    int px1Y = (windowWidth/2) + std::floor(distanceNewBasisP1YAxis.getX() / std::abs(distanceNewBasisP1YAxis.getZ()) * (windowWidth/2));
    int py1Y = (windowHeight/2) + std::floor(distanceNewBasisP1YAxis.getY() / std::abs(distanceNewBasisP1YAxis.getZ()) * (windowWidth/2));
    int px2Y = (windowWidth/2) + std::floor(distanceNewBasisP2YAxis.getX() / std::abs(distanceNewBasisP2YAxis.getZ()) * (windowWidth/2));
    int py2Y = (windowHeight/2) + std::floor(distanceNewBasisP2YAxis.getY() / std::abs(distanceNewBasisP2YAxis.getZ()) * (windowWidth/2));
    int px1OSY = (windowWidth/2) + std::floor(distanceNewBasisP1ObjectSurfaceYAxis.getX() / std::abs(distanceNewBasisP1ObjectSurfaceYAxis.getZ()) * (windowWidth/2));
    int py1OSY = (windowHeight/2) + std::floor(distanceNewBasisP1ObjectSurfaceYAxis.getY() / std::abs(distanceNewBasisP1ObjectSurfaceYAxis.getZ()) * (windowWidth/2));
    int px2OSY = (windowWidth/2) + std::floor(distanceNewBasisP2ObjectSurfaceYAxis.getX() / std::abs(distanceNewBasisP2ObjectSurfaceYAxis.getZ()) * (windowWidth/2));
    int py2OSY = (windowHeight/2) + std::floor(distanceNewBasisP2ObjectSurfaceYAxis.getY() / std::abs(distanceNewBasisP2ObjectSurfaceYAxis.getZ()) * (windowWidth/2));

    int px1Z = (windowWidth/2) + std::floor(distanceNewBasisP1ZAxis.getX() / std::abs(distanceNewBasisP1ZAxis.getZ()) * (windowWidth/2));
    int py1Z = (windowHeight/2) + std::floor(distanceNewBasisP1ZAxis.getY() / std::abs(distanceNewBasisP1ZAxis.getZ()) * (windowWidth/2));
    int px2Z = (windowWidth/2) + std::floor(distanceNewBasisP2ZAxis.getX() / std::abs(distanceNewBasisP2ZAxis.getZ()) * (windowWidth/2));
    int py2Z = (windowHeight/2) + std::floor(distanceNewBasisP2ZAxis.getY() / std::abs(distanceNewBasisP2ZAxis.getZ()) * (windowWidth/2));
    int px1OSZ = (windowWidth/2) + std::floor(distanceNewBasisP1ObjectSurfaceZAxis.getX() / std::abs(distanceNewBasisP1ObjectSurfaceZAxis.getZ()) * (windowWidth/2));
    int py1OSZ = (windowHeight/2) + std::floor(distanceNewBasisP1ObjectSurfaceZAxis.getY() / std::abs(distanceNewBasisP1ObjectSurfaceZAxis.getZ()) * (windowWidth/2));
    int px2OSZ = (windowWidth/2) + std::floor(distanceNewBasisP2ObjectSurfaceZAxis.getX() / std::abs(distanceNewBasisP2ObjectSurfaceZAxis.getZ()) * (windowWidth/2));
    int py2OSZ = (windowHeight/2) + std::floor(distanceNewBasisP2ObjectSurfaceZAxis.getY() / std::abs(distanceNewBasisP2ObjectSurfaceZAxis.getZ()) * (windowWidth/2));

    // No we check if they are visible

    DrawObject *drawObject;
    // drawObject = new DrawObject(RED_COL, px1X, py1X, px1OSX, py1OSX, distanceNewBasisP1XAxis.getLength(), LINE);
    // addObjectOnScreen(drawObject);
    if(visibleOnScreen(px1X, py1X) || visibleOnScreen(px1OSX, py1OSX)){
        drawObject = new DrawObject(RED_COL, px1X, py1X, px1OSX, py1OSX, distanceNewBasisP1XAxis.getLength(), LINE);
        addObjectOnScreen(drawObject);
    }
    if(visibleOnScreen(px2OSX, py2OSX) || visibleOnScreen(px2X, py2X)){
        drawObject = new DrawObject(RED_COL, px2OSX, py2OSX, px2X, py2X, distanceNewBasisP2XAxis.getLength(), LINE);
        addObjectOnScreen(drawObject);
    }

    if(visibleOnScreen(px1Y, py1Y) || visibleOnScreen(px1OSY, py1OSY)){
        drawObject = new DrawObject(GREEN_COL, px1Y, py1Y, px1OSY, py1OSY, distanceNewBasisP1YAxis.getLength(), LINE);
        addObjectOnScreen(drawObject);
    }
    if(visibleOnScreen(px2OSY, py2OSY) || visibleOnScreen(px2Y, py2Y)){
        drawObject = new DrawObject(GREEN_COL, px2OSY, py2OSY, px2Y, py2Y, distanceNewBasisP2YAxis.getLength(), LINE);
        addObjectOnScreen(drawObject);
    }

    if(visibleOnScreen(px1Z, py1Z) || visibleOnScreen(px1OSZ, py1OSZ)){
        drawObject = new DrawObject(ORANGE_COL, px1Z, py1Z, px1OSZ, py1OSZ, distanceNewBasisP1ZAxis.getLength(), LINE);
        addObjectOnScreen(drawObject);
    }
    if(visibleOnScreen(px2OSZ, py2OSZ) || visibleOnScreen(px2Z, py2Z)){
        drawObject = new DrawObject(ORANGE_COL, px2OSZ, py2OSZ, px2Z, py2Z, distanceNewBasisP2ZAxis.getLength(), LINE);
        addObjectOnScreen(drawObject);
    }

    // printf("Drawing line from (%d, %d) to (%d, %d) and from (%d, %d) to (%d, %d), while objects is placed at (%d, %d) with size %d\n", px1X, py1X, px1OSX, py1OSX, px2OSX, py2OSX, px2X, py2X, x, y, size);
}


void Renderer::rotateCamera(long double angle, short axis){
    // The three vectors cameraDirection, cameraPlaneVector1 and cameraPlaneVector2 are an orthonormal basis. We therefore look at them as basisvectors for the rotation
    // Thus we can rotate them around more easily and in the end

    // The equation is like this: VectorStandardBasis = TransformationMatrix * VectorNewBasis,
    // where TransformationMatrix are the vectors (cameraPlaneVector1, cameraPlaneVector2, cameraDirection)

    // printf("Rotating camera by %f on axis %d\n", angle, axis);

    PositionVector tempCameraPlaneVector1 = PositionVector();
    PositionVector tempCameraPlaneVector2 = PositionVector();
    PositionVector tempCameraDirection = PositionVector();

    if(centreObject != NULL || axis == Z_AXIS){
        angle *= -1;
    }

    switch(axis){
        case X_AXIS:
            tempCameraPlaneVector1.setX(1);
            tempCameraPlaneVector1.setY(0);
            tempCameraPlaneVector1.setZ(0);

            tempCameraPlaneVector2.setX(0);
            tempCameraPlaneVector2.setY(std::cos(angle));
            tempCameraPlaneVector2.setZ(std::sin(angle));

            tempCameraDirection.setX(0);
            tempCameraDirection.setY(-std::sin(angle));
            tempCameraDirection.setZ(std::cos(angle));
            break;
        case Y_AXIS:
            tempCameraPlaneVector1.setX(std::cos(angle));
            tempCameraPlaneVector1.setY(0);
            tempCameraPlaneVector1.setZ(-std::sin(angle));

            tempCameraPlaneVector2.setX(0);
            tempCameraPlaneVector2.setY(1);
            tempCameraPlaneVector2.setZ(0);

            tempCameraDirection.setX(std::sin(angle));
            tempCameraDirection.setY(0);
            tempCameraDirection.setZ(std::cos(angle));
            break;
        case Z_AXIS:
            tempCameraPlaneVector1.setX(std::cos(angle));
            tempCameraPlaneVector1.setY(std::sin(angle));
            tempCameraPlaneVector1.setZ(0);

            tempCameraPlaneVector2.setX(-std::sin(angle));
            tempCameraPlaneVector2.setY(std::cos(angle));
            tempCameraPlaneVector2.setZ(0);

            tempCameraDirection.setX(0);
            tempCameraDirection.setY(0);
            tempCameraDirection.setZ(1);
            break;
    }
    cameraPlaneVector1 = transformationMatrixCameraBasis * tempCameraPlaneVector1;
    cameraPlaneVector2 = transformationMatrixCameraBasis * tempCameraPlaneVector2;
    cameraDirection = transformationMatrixCameraBasis * tempCameraDirection;

    // printf("Before - cameraPosition.getLength(): %f\n",  cameraPosition.getLength());
    if(centreObject != NULL){
        cameraPosition = cameraDirection * -1 * cameraPosition.getLength();
    }
    // printf("After - cameraPosition.getLength(): %f\n",  cameraPosition.getLength());

    // Adjust transformationmatrices
    transformationMatrixCameraBasis = Matrix3d(cameraPlaneVector1, cameraPlaneVector2, cameraDirection);
    inverseTransformationMatrixCameraBasis = transformationMatrixCameraBasis.transpose();
    // printf("Length of new camera vectors: %Lf, %Lf, %Lf\n", cameraPlaneVector1.getLength(), cameraPlaneVector2.getLength(), cameraDirection.getLength());
    // transformationMatrixCameraBasis.print();
    // inverseTransformationMatrixCameraBasis.print();
}

void Renderer::moveCamera(short direction, short axis){
    // printf("Moving camera\n");
    if(centreObject == NULL || axis != Z_AXIS){
        switch(axis){
            case X_AXIS:
                cameraPosition.setX(cameraPosition.getX() + direction * cameraMoveAmount * cameraPlaneVector1.getX());
                cameraPosition.setY(cameraPosition.getY() + direction * cameraMoveAmount * cameraPlaneVector1.getY());
                cameraPosition.setZ(cameraPosition.getZ() + direction * cameraMoveAmount * cameraPlaneVector1.getZ());
                break;
            case Y_AXIS:
                cameraPosition.setX(cameraPosition.getX() + direction * cameraMoveAmount * cameraPlaneVector2.getX());
                cameraPosition.setY(cameraPosition.getY() + direction * cameraMoveAmount * cameraPlaneVector2.getY());
                cameraPosition.setZ(cameraPosition.getZ() + direction * cameraMoveAmount * cameraPlaneVector2.getZ());
                break;
            case Z_AXIS:
                cameraPosition.setX(cameraPosition.getX() + direction * cameraMoveAmount * cameraDirection.getX());
                cameraPosition.setY(cameraPosition.getY() + direction * cameraMoveAmount * cameraDirection.getY());
                cameraPosition.setZ(cameraPosition.getZ() + direction * cameraMoveAmount * cameraDirection.getZ());
                break;
        }
        centreObject = NULL;
        return;
    }
    // If object is in focus
    // ------------------------------------------------------------------------ TODO ------------------------------------------------------------------------
    if(direction == FORWARDS){
        if(cameraPosition.getLength() / 1.25 >= centreObject->getRadius()*1.4)
            cameraPosition /= 1.25;
        else if(cameraPosition.getLength() / 1.1 >= centreObject->getRadius()*1.3)
            cameraPosition /= 1.1;
        else if(cameraPosition.getLength() / 1.01 >= centreObject->getRadius()*1.2)
            cameraPosition /= 1.01;
        else if(cameraPosition.getLength() / 1.001 >= centreObject->getRadius()*1.1)
            cameraPosition /= 1.01;
        // printf("New cameraPosition: (%f, %f, %f), distance to centre: %fly\n", cameraPosition.getX(),cameraPosition.getY(), cameraPosition.getZ(), cameraPosition.getLength()/lightyear);
        return;
    }
    else{
        if(cameraPosition.getLength() >= centreObject->getRadius()*1.4)
            cameraPosition *= 1.25;
        else if(cameraPosition.getLength()>= centreObject->getRadius()*1.3)
            cameraPosition *= 1.1;
        else if(cameraPosition.getLength() >= centreObject->getRadius()*1.2)
            cameraPosition *= 1.01;
        else if(cameraPosition.getLength() >= centreObject->getRadius()*1.1)
            cameraPosition *= 1.01;
        else
            cameraPosition *= 1.01;
    }
    // printf("New cameraPosition: (%f, %f, %f), distance to centre: %fly\n", cameraPosition.getX(),cameraPosition.getY(), cameraPosition.getZ(), cameraPosition.getLength()/lightyear);
}

void Renderer::resetCameraOrientation(){
    // ------------------------------------------------------------------------ TODO ------------------------------------------------------------------------
    cameraDirection = PositionVector(-1, 0, 0);
    cameraPlaneVector1 = PositionVector(0, 1, 0);
    cameraPlaneVector2 = PositionVector(0, 0, 1);
    if(centreObject == NULL){
        centreObject = referenceObject;
    }
    cameraPosition = cameraDirection * STANDARD_CAMERA_POSITION;
    transformationMatrixCameraBasis = Matrix3d(cameraPlaneVector1, cameraPlaneVector2, cameraDirection);
    inverseTransformationMatrixCameraBasis = transformationMatrixCameraBasis.transpose();
}

bool Renderer::visibleOnScreen(int x, int y){
    return (x >= 0 && x <= myWindow->getWindowWidth() && y >= 0 && y <= myWindow->getWindowHeight());
}

void Renderer::increaseCameraMoveAmount(){
    cameraMoveAmount *= 1.5;
    if(cameraMoveAmount < astronomicalUnit/2)
        printf("New cameraMoveAmount: %Lfm\n", cameraMoveAmount);
    else if(cameraMoveAmount < lightyear/2)
        printf("New cameraMoveAmount: %LfaU\n", cameraMoveAmount/astronomicalUnit);
    else
        printf("New cameraMoveAmount: %Lfly\n", cameraMoveAmount/lightyear);
}

void Renderer::decreaseCameraMoveAmount(){
    cameraMoveAmount = std::max(cameraMoveAmount / 1.5, (long double) 1.0);
    if(cameraMoveAmount < astronomicalUnit/2)
        printf("New cameraMoveAmount: %Lfm\n", cameraMoveAmount);
    else if(cameraMoveAmount < lightyear/2)
        printf("New cameraMoveAmount: %LfaU\n", cameraMoveAmount/astronomicalUnit);
    else
        printf("New cameraMoveAmount: %Lfly\n", cameraMoveAmount/lightyear);
}

void Renderer::increaseSimulationSpeed(){
    // printf("Old optimalTimeLocalUpdate: %d\n", *optimalTimeLocalUpdate);
    *optimalTimeLocalUpdate /= 2;
    // printf("New optimalTimeLocalUpdate: %d\n", *optimalTimeLocalUpdate);
}

void Renderer::decreaseSimulationSpeed(){
    // printf("Old optimalTimeLocalUpdate: %d\n", *optimalTimeLocalUpdate);
    *optimalTimeLocalUpdate *= 2;
    // printf("New optimalTimeLocalUpdate: %d\n", *optimalTimeLocalUpdate);
}

void Renderer::centreParent(){
    if(referenceObject == NULL){
        // ------------------------------------------------------------------ TODO ------------------------------------------------------------------
        // Search nearest object and centre on it
        centreObject = galaxies->at(0);
    }
    else if(referenceObject->getType() == GALACTIC_CORE){
        centreObject = referenceObject;
        return;
    }
    else if(referenceObject->getParent()->getType() == STARSYSTEM){
        centreObject = referenceObject->getParent()->getParent();
    }
    else{
        centreObject = referenceObject->getParent();
    }
    referenceObject = centreObject;
    // Centre camera on new centre object
    cameraPosition = cameraDirection * STANDARD_CAMERA_POSITION;
    // cameraPosition = cameraDirection * -3 * astronomicalUnit;
    // printf("camera: (%f, %f, %f). Length: %f\n", cameraPosition.getX(), cameraPosition.getY(), cameraPosition.getZ(), cameraPosition.getLength());
}

void Renderer::centreChild(){
    if(referenceObject == NULL){
        // ------------------------------------------------------------------ TODO ------------------------------------------------------------------
        // Search nearest object and centre on it
        centreObject = galaxies->at(0);
    }
    else if(referenceObject->getChildren()->empty()){
        centreObject = referenceObject;
        return;
    }
    else if(referenceObject->getType() == GALACTIC_CORE){
        centreObject = referenceObject->getChildren()->at(0)->getChildren()->at(0);
    }
    else{
        centreObject = referenceObject->getChildren()->at(0);
    }
    referenceObject = centreObject;
    // Centre camera on new centre object
    cameraPosition = cameraDirection * STANDARD_CAMERA_POSITION;
}

void Renderer::centreNext(){
    if(referenceObject == NULL){
        // ------------------------------------------------------------------ TODO ------------------------------------------------------------------
        // Search nearest object and centre on it
        centreObject = galaxies->at(0);
    }
    else if(referenceObject->getType() == GALACTIC_CORE){
        int numberOfGalaxies = galaxies->size();
        for(int i=0;i<numberOfGalaxies;i++){
            if((void*) galaxies->at(i) == (void*) referenceObject){
                // printf("Found object, next index is %d\n", (i+1)%numberOfGalaxies);
                centreObject = galaxies->at((i+1)%numberOfGalaxies);
                break;
            }
        }
    }
    else{
        int numberOfChildren = referenceObject->getParent()->getChildren()->size();
        for(int i=0;i<numberOfChildren;i++){
            if((void*) referenceObject->getParent()->getChildren()->at(i) == (void*) referenceObject){
                centreObject = referenceObject->getParent()->getChildren()->at((i+1)%numberOfChildren);
                break;
            }
            // printf("For i = %d, pointer 1 = %p, pointer 2 = %p. Name of p1: %s, Name of p2: %s\n", i, (void*) centreObject->getParent()->getChildren()->at(i), (void*) centreObject, centreObject->getParent()->getChildren()->at(i)->getName(), centreObject->getName());
        }
    }
    referenceObject = centreObject;
    // Centre camera on new centre object
    cameraPosition = cameraDirection * STANDARD_CAMERA_POSITION;
}

void Renderer::centrePrevious(){
    if(referenceObject->getType() == GALACTIC_CORE){
        int numberOfGalaxies = galaxies->size();
        for(int i=0;i<numberOfGalaxies;i++){
            if((void*) galaxies->at(i) == (void*) referenceObject){
                centreObject = galaxies->at((i+1)%numberOfGalaxies);
                break;
            }
        }
    }
    else{
        int numberOfChildren = referenceObject->getParent()->getChildren()->size();
        for(int i=0;i<numberOfChildren;i++){
            if((void*) referenceObject->getParent()->getChildren()->at(i) == (void*) referenceObject){
                centreObject = referenceObject->getParent()->getChildren()->at((i-1+numberOfChildren)%numberOfChildren);
                break;
            }
        }
    }
    referenceObject = centreObject;
    // Centre camera on new centre object
    cameraPosition = cameraDirection * STANDARD_CAMERA_POSITION;
}

void Renderer::centreNextStarSystem(){
    if(referenceObject == NULL || referenceObject->getType() == GALACTIC_CORE ){
        centreChild();
        return;
    }
    while(referenceObject->getType() != STARSYSTEM) referenceObject = referenceObject->getParent();
    centreNext();
    centreChild();
    printf("Now centring %s\n", centreObject->getName());
}

void Renderer::centrePreviousStarSystem(){
    if(referenceObject == NULL || referenceObject->getType() == GALACTIC_CORE ){
        centreChild();
        return;
    }
    while(referenceObject->getType() != STARSYSTEM) referenceObject = referenceObject->getParent();
    centrePrevious();
    centreChild();
    // printf("Now centring %s\n", centreObject->getName());
}

void Renderer::centreNearest(){
    StellarObject *nearest = galaxies->at(0);
    PositionVector absolutCameraPosition = cameraPosition;
    if(referenceObject != NULL)
        absolutCameraPosition += referenceObject->getPosition();
    long double distance = (nearest->getPosition() - absolutCameraPosition).getLength();
    long double tempDistance;
    // printf("%s has distance %f\n", nearest->getName(), distance);

    for(StellarObject *galacticCore: *galaxies){
        for(StellarObject *starSystem: *(galacticCore->getChildren())){
            for(StellarObject *star: *(starSystem->getChildren())){
                for(StellarObject *planet: *(star->getChildren())){
                    for(StellarObject *moon: *(planet->getChildren())){
                        tempDistance = (moon->getPosition() - absolutCameraPosition).getLength();
                        if(tempDistance<distance){
                            distance = tempDistance;
                            nearest = moon;
                        }
                        // printf("%s has distance %f\n", moon->getName(), tempDistance);
                    }
                    tempDistance = (planet->getPosition() - absolutCameraPosition).getLength();
                    if(tempDistance<distance){
                        distance = tempDistance;
                        nearest = planet;
                    }
                    // printf("%s has distance %f\n", planet->getName(), tempDistance);
                }
                tempDistance = (star->getPosition() - absolutCameraPosition).getLength();
                if(tempDistance<distance){
                    distance = tempDistance;
                    nearest = star;
                }
                // printf("%s has distance %f\n", star->getName(), tempDistance);
            }
        }
        tempDistance = (galacticCore->getPosition() - absolutCameraPosition).getLength();
        if(tempDistance<distance){
            distance = tempDistance;
            nearest = galacticCore;
        }
        // printf("%s has distance %f\n", galacticCore->getName(), tempDistance);
    }

    centreObject = nearest;
    referenceObject = centreObject;

    cameraPosition = cameraDirection * STANDARD_CAMERA_POSITION;
}

void Renderer::toggleCentre(){
    if(centreObject != NULL){
        centreObject = NULL;
        return;
    }
    centreNearest();
}

void Renderer::addObjectOnScreen(DrawObject *drawObject){
    // Still quite inefficient to lock for every single entry
    objectsOnScreenLock.lock();
    objectsOnScreen.push_back(drawObject);
    objectsOnScreenLock.unlock();
}

void Renderer::addDotOnScreen(DrawObject *drawObject){
    // Still quite inefficient to lock for every single entry
    dotsOnScreenLock.lock();
    dotsOnScreen.push_back(drawObject);
    dotsOnScreenLock.unlock();
}

void Renderer::addObjectsOnScreen(std::vector<DrawObject*> *drawObjects){
    objectsOnScreenLock.lock();
    for(DrawObject *drawObject: *drawObjects){
        objectsOnScreen.push_back(drawObject);
    }
    objectsOnScreenLock.unlock();
}

void Renderer::addDotsOnScreen(std::vector<DrawObject*> *drawObjects){
    dotsOnScreenLock.lock();
    for(DrawObject *drawObject: *drawObjects){
        dotsOnScreen.push_back(drawObject);
    }
    dotsOnScreenLock.unlock();
}

void Renderer::initialiseReferenceObject(){
    referenceObject = (*galaxies)[0];
}

void Renderer::adjustThreadCount(int8_t adjustment){
    if((rendererThreadCount == RENDERER_MAX_THREAD_COUNT && adjustment == INCREASE_THREAD_COUNT) || (rendererThreadCount == 1 && adjustment == DECREASE_THREAD_COUNT)){
        // printf("RendererThreadCount is already the max/min, namely %d\n", rendererThreadCount);
        return;
    }
    rendererThreadCount = std::min(RENDERER_MAX_THREAD_COUNT, std::max(1, rendererThreadCount + adjustment));
    // printf("Adjusted rendererThreadCount to %d\n", rendererThreadCount);
}

void Renderer::handleEvents(bool &isRunning, bool &isPaused){
    #define Expose 12
    #define KeyPress 2

    int numberOfPendingEvents = myWindow->getNumberOfPendingEvents();
    int eventTypes[numberOfPendingEvents];
    u_int parameters[numberOfPendingEvents];
    myWindow->getPendingEvents(eventTypes, parameters, numberOfPendingEvents);
    if(eventTypes[0] == -1){
        // Something wrong happened: abort
        printf("Something went wrong with eventtype\n");
        return;
    }
    for(int i=0;i<numberOfPendingEvents;i++){        
        switch(eventTypes[i]){
            case Expose: draw(); break;
            case KeyPress: 
                switch(parameters[i]){
                    case KEY_ESCAPE: isRunning = false; /*printf("Escaped program\n");*/ break;
                    case KEY_SPACE: isPaused = !isPaused; /*isPaused ? printf("Paused\n") : printf("Unpaused\n");*/ break;
                    // Camera movements
                    case KEY_K: moveCamera(FORWARDS, Y_AXIS); break;
                    case KEY_I: moveCamera(BACKWARDS, Y_AXIS); break;
                    case KEY_L: moveCamera(FORWARDS, X_AXIS); break;
                    case KEY_J: moveCamera(BACKWARDS, X_AXIS); break;
                    case KEY_O: moveCamera(FORWARDS, Z_AXIS); break;
                    case KEY_P: moveCamera(BACKWARDS, Z_AXIS); break;
                    case KEY_OE: decreaseCameraMoveAmount(); break;
                    case KEY_AE: increaseCameraMoveAmount(); break;
                    // Camera rotations
                    case KEY_W: rotateCamera(1.0/180*PI, X_AXIS); break;
                    case KEY_S: rotateCamera(-1.0/180*PI, X_AXIS); break;
                    case KEY_D: rotateCamera(1.0/180*PI, Y_AXIS); break;
                    case KEY_A: rotateCamera(-1.0/180*PI, Y_AXIS); break;
                    case KEY_Q: rotateCamera(1.0/180*PI, Z_AXIS); break;
                    case KEY_E: rotateCamera(-1.0/180*PI, Z_AXIS); break;
                    case KEY_R: resetCameraOrientation(); break;
                    // Focus changes
                    case KEY_1: centreParent(); break;
                    case KEY_2: centreChild(); break;
                    case KEY_3: centrePrevious(); break;
                    case KEY_4: centreNext(); break;
                    case KEY_5: centrePreviousStarSystem(); break;
                    case KEY_6: centreNextStarSystem(); break;
                    case KEY_F: toggleCentre(); break;

                    // Test
                    case KEY_9: test = 1-test;

                    // Speed changes
                    case KEY_PG_UP: increaseSimulationSpeed(); break;
                    case KEY_PG_DOWN: decreaseSimulationSpeed(); break;

                    /*default: printf("Key number %d was pressed\n", event.xkey.keycode);*/
                } break;
        }
    }
}

int Renderer::getWindowWidth(){
    return myWindow->getWindowWidth();
}

int Renderer::getWindowHeight(){
    return myWindow->getWindowHeight();
}

MyWindow *Renderer::getMyWindow(){
    return myWindow;
}

Matrix3d Renderer::getInverseTransformationMatrixCameraBasis(){
    return inverseTransformationMatrixCameraBasis;
}

PositionVector Renderer::getCameraPosition(){
    return cameraPosition;
}

StellarObject *Renderer::getReferenceObject(){
    return referenceObject;
}

std::vector<DrawObject*> *Renderer::getObjectsOnScreen(){
    return &objectsOnScreen;
}

std::vector<StellarObject*> *Renderer::getAllObjects(){
    return allObjects;
}

int8_t Renderer::getRendererThreadCount(){
    return rendererThreadCount;
}

void calculateObjectPositionsMultiThread(int start, int amount, Renderer *renderer, int id){
    // printf("Started thread %d\n", id);
    // printf("Started thread with start = %d, amount = %d\n", start, amount);

    std::vector<DrawObject*> objectsToAddOnScreen;
    std::vector<DrawObject*> dotsToAddOnScreen;
    for(int i=start;i<start+amount;i++){
        if(renderer->getAllObjects()->at(i)->getType() == STARSYSTEM) continue;
        // if(i%10==0)
            // printf("Calculating position of %s with number %d\n", allObjects->at(i)->getName(), i);
        renderer->calculateObjectPosition(renderer->getAllObjects()->at(i), &objectsToAddOnScreen, &dotsToAddOnScreen);
    }
    renderer->addObjectsOnScreen(&objectsToAddOnScreen);
    renderer->addDotsOnScreen(&dotsToAddOnScreen);
}

void updatePositionAtPointInTimeMultiThread(std::vector<StellarObject*> *allObjects, int start, int amount){
    for(int i=start;i<start+amount;i++){
        allObjects->at(i)->updatePositionAtPointInTime();
    }
}

void calculateCloseObjectTrianglesMultiThread(Renderer *renderer, StellarObject *object, std::vector<RenderTriangle*> *triangles, DrawObject *drawObject, int start, int amount, std::mutex *drawObjectLock){
    std::vector<DrawObject*> drawObjectsToAdd;
    // printf("Arrived in multithread function with start: %d, amount: %d\n", start, amount);
    // struct timespec prevTime;
	// struct timespec currTime;
    // clock_gettime(CLOCK_MONOTONIC, &prevTime);

    PositionVector cameraPosition = renderer->getCameraPosition();
    Matrix3d inverseTransformationMatrixCameraBasis = renderer->getInverseTransformationMatrixCameraBasis();
    PositionVector absoluteCameraPosition = renderer->getReferenceObject()->getPositionAtPointInTime() + cameraPosition;

    for(int i=start;i<start+amount;i++){
        RenderTriangle* triangle = triangles->at(i);

        PositionVector distanceMidPoint = triangle->getMidPoint() + object->getPositionAtPointInTime() - absoluteCameraPosition;
        PositionVector distanceMidPointNewBasis = inverseTransformationMatrixCameraBasis * distanceMidPoint;
        PositionVector distanceP1 = triangle->getPoint1() + object->getPositionAtPointInTime() - absoluteCameraPosition;
        PositionVector distanceP1NewBasis = inverseTransformationMatrixCameraBasis * distanceP1;
        PositionVector distanceP2 = triangle->getPoint2() + object->getPositionAtPointInTime() - absoluteCameraPosition;
        PositionVector distanceP2NewBasis = inverseTransformationMatrixCameraBasis * distanceP2;
        PositionVector distanceP3 = triangle->getPoint3() + object->getPositionAtPointInTime() - absoluteCameraPosition;
        PositionVector distanceP3NewBasis = inverseTransformationMatrixCameraBasis * distanceP3;

        
        // For debug normals of points
        if(renderer->test){
            PositionVector normal1Endpoint = distanceP1 + triangle->getPoint1Normal().normalise() * (object->getRadius()/10);
            PositionVector normal2Endpoint = distanceP2 + triangle->getPoint2Normal().normalise() * (object->getRadius()/10);
            PositionVector normal3Endpoint = distanceP3 + triangle->getPoint3Normal().normalise() * (object->getRadius()/10);
            PositionVector normal1EndpointNewBasis = inverseTransformationMatrixCameraBasis * normal1Endpoint;
            PositionVector normal2EndpointNewBasis = inverseTransformationMatrixCameraBasis * normal2Endpoint;
            PositionVector normal3EndpointNewBasis = inverseTransformationMatrixCameraBasis * normal3Endpoint;
            if(distanceP1NewBasis.getZ() > normal1EndpointNewBasis.getZ() && distanceP1NewBasis.getZ() >= 0){
                int windowWidth = renderer->getWindowWidth();
                int windowHeight = renderer->getWindowHeight();

                int xP1 = (windowWidth/2) + std::floor(distanceP1NewBasis.getX() / std::abs(distanceP1NewBasis.getZ()) * (windowWidth/2));
                int yP1 = (windowHeight/2) + std::floor(distanceP1NewBasis.getY() / std::abs(distanceP1NewBasis.getZ()) * (windowWidth/2));
                int xP2 = (windowWidth/2) + std::floor(normal1EndpointNewBasis.getX() / std::abs(normal1EndpointNewBasis.getZ()) * (windowWidth/2));
                int yP2 = (windowHeight/2) + std::floor(normal1EndpointNewBasis.getY() / std::abs(normal1EndpointNewBasis.getZ()) * (windowWidth/2));

                drawObjectsToAdd.push_back(new DrawObject(0xFF0000, xP1, yP1, xP2, yP2, 1, LINE));
                // printf("Adding a line from (%d, %d) to (%d, %d). Distance1: %s, normal1Endpoint: %s\n", xP1, yP1, xP2, yP2, distanceP1NewBasis.toString(), normal1EndpointNewBasis.toString());
            }
            if(distanceP2NewBasis.getZ() > normal2EndpointNewBasis.getZ() && distanceP2NewBasis.getZ() >= 0){
                int windowWidth = renderer->getWindowWidth();
                int windowHeight = renderer->getWindowHeight();

                int xP1 = (windowWidth/2) + std::floor(distanceP2NewBasis.getX() / std::abs(distanceP2NewBasis.getZ()) * (windowWidth/2));
                int yP1 = (windowHeight/2) + std::floor(distanceP2NewBasis.getY() / std::abs(distanceP2NewBasis.getZ()) * (windowWidth/2));
                int xP2 = (windowWidth/2) + std::floor(normal2EndpointNewBasis.getX() / std::abs(normal2EndpointNewBasis.getZ()) * (windowWidth/2));
                int yP2 = (windowHeight/2) + std::floor(normal2EndpointNewBasis.getY() / std::abs(normal2EndpointNewBasis.getZ()) * (windowWidth/2));

                drawObjectsToAdd.push_back(new DrawObject(0xFF0000, xP1, yP1, xP2, yP2, 1, LINE));
            }
            if(distanceP3NewBasis.getZ() > normal3EndpointNewBasis.getZ() && distanceP3NewBasis.getZ() >= 0){
                int windowWidth = renderer->getWindowWidth();
                int windowHeight = renderer->getWindowHeight();

                int xP1 = (windowWidth/2) + std::floor(distanceP3NewBasis.getX() / std::abs(distanceP3NewBasis.getZ()) * (windowWidth/2));
                int yP1 = (windowHeight/2) + std::floor(distanceP3NewBasis.getY() / std::abs(distanceP3NewBasis.getZ()) * (windowWidth/2));
                int xP2 = (windowWidth/2) + std::floor(normal3EndpointNewBasis.getX() / std::abs(normal3EndpointNewBasis.getZ()) * (windowWidth/2));
                int yP2 = (windowHeight/2) + std::floor(normal3EndpointNewBasis.getY() / std::abs(normal3EndpointNewBasis.getZ()) * (windowWidth/2));

                drawObjectsToAdd.push_back(new DrawObject(0xFF0000, xP1, yP1, xP2, yP2, 1, LINE));
            }
        }
        
        

        // I do not think we can rule out every of this triangles. Thus we change from (if distance behind) to (if all points behind)
        // if(distanceMidPointNewBasis.getZ()<0) {
        //     // printf("Found triangle with midpoint behind the camera\n");
        //     // delete triangle;
        //     continue;
        // }
        if(distanceP1NewBasis.getZ()<0 && distanceP2NewBasis.getZ()<0 && distanceP3NewBasis.getZ()<0){
            continue;
        }

        // If all three points are outside from the perspective of one of the edges, we can ignore it
        if((distanceP1NewBasis.getX() >= std::abs(distanceP1NewBasis.getZ()) && distanceP2NewBasis.getX() >= std::abs(distanceP2NewBasis.getZ()) && distanceP3NewBasis.getX() >= std::abs(distanceP3NewBasis.getZ())) ||
        (distanceP1NewBasis.getY() >= std::abs(distanceP1NewBasis.getZ()) && distanceP2NewBasis.getY() >= std::abs(distanceP2NewBasis.getZ()) && distanceP3NewBasis.getY() >= std::abs(distanceP3NewBasis.getZ())) ||
        (distanceP1NewBasis.getX() <= -1 * std::abs(distanceP1NewBasis.getZ()) && distanceP2NewBasis.getX() <= -1 * std::abs(distanceP2NewBasis.getZ()) && distanceP3NewBasis.getX() <= -1 * std::abs(distanceP3NewBasis.getZ())) ||
        (distanceP1NewBasis.getY() <= -1 * std::abs(distanceP1NewBasis.getZ()) && distanceP2NewBasis.getY() <= -1 * std::abs(distanceP2NewBasis.getZ()) && distanceP3NewBasis.getY() <= -1 * std::abs(distanceP3NewBasis.getZ()))){
            continue;
        }

        // ------------------------------------------------------------------- TODO -------------------------------------------------------------------
        // Make additional checks to rule out anomalies
        // Cut down lines going behind the camera
        // Mabe?

        bool p1InFront = distanceP1NewBasis.getZ() >= 0;
        bool p2InFront = distanceP2NewBasis.getZ() >= 0;
        bool p3InFront = distanceP3NewBasis.getZ() >= 0;

        int windowWidth = renderer->getWindowWidth();
        int windowHeight = renderer->getWindowHeight();

        int xP1 = (windowWidth/2) + std::floor(distanceP1NewBasis.getX() / std::abs(distanceP1NewBasis.getZ()) * (windowWidth/2));
        int yP1 = (windowHeight/2) + std::floor(distanceP1NewBasis.getY() / std::abs(distanceP1NewBasis.getZ()) * (windowWidth/2));
        int xP2 = (windowWidth/2) + std::floor(distanceP2NewBasis.getX() / std::abs(distanceP2NewBasis.getZ()) * (windowWidth/2));
        int yP2 = (windowHeight/2) + std::floor(distanceP2NewBasis.getY() / std::abs(distanceP2NewBasis.getZ()) * (windowWidth/2));
        int xP3 = (windowWidth/2) + std::floor(distanceP3NewBasis.getX() / std::abs(distanceP3NewBasis.getZ()) * (windowWidth/2));
        int yP3 = (windowHeight/2) + std::floor(distanceP3NewBasis.getY() / std::abs(distanceP3NewBasis.getZ()) * (windowWidth/2));

        // if(xP1 > 390 && xP1 < 410 && yP1 > 290 && yP1 < 310){
        //     printf("Point 1 (%d, %d) has coordinates (%Lf, %Lf, %Lf) and normal (%Lf, %Lf, %Lf)\n", xP1, yP1, triangle->getPoint1().getX(), triangle->getPoint1().getY(), triangle->getPoint1().getZ(), triangle->getPoint1Normal().getX(), triangle->getPoint1Normal().getY(), triangle->getPoint1Normal().getZ());
        // }
        // if(xP2 > 390 && xP2 < 410 && yP2 > 290 && yP2 < 310){
        //     printf("Point 2 (%d, %d) has coordinates (%Lf, %Lf, %Lf) and normal (%Lf, %Lf, %Lf)\n", xP2, yP2, triangle->getPoint2().getX(), triangle->getPoint2().getY(), triangle->getPoint2().getZ(), triangle->getPoint2Normal().getX(), triangle->getPoint2Normal().getY(), triangle->getPoint2Normal().getZ());
        // }
        // if(xP3 > 390 && xP3 < 410 && yP3 > 290 && yP3 < 310){
        //     printf("Point 3 (%d, %d) has coordinates (%Lf, %Lf, %Lf) and normal (%Lf, %Lf, %Lf)\n", xP3, yP3, triangle->getPoint3().getX(), triangle->getPoint3().getY(), triangle->getPoint3().getZ(), triangle->getPoint3Normal().getX(), triangle->getPoint3Normal().getY(), triangle->getPoint3Normal().getZ());
        // }

        int colour = object->getColour();
        int colourP1 = colour;
        int colourP2 = colour;
        int colourP3 = colour;
        long double dotProductNormalVector = 1;
        long double dotProductP1 = 1;
        long double dotProductP2 = 1;
        long double dotProductP3 = 1;

        if(object->getType() != GALACTIC_CORE && object->getType() != STAR){
            // colour = object->getHomeSystem()->getChildren()->at(0)->getColour();
            // colourP1 = colour;
            // colourP2 = colour;
            // colourP3 = colour;
            // This is the dot product of the vector from sun to object and the normal vector
            // AFter that the dot product of the vector from sun to object and the normal vector of the three points
            dotProductNormalVector = std::max(0.0L, (object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - (triangle->getMidPoint() + object->getPositionAtPointInTime())).normalise().dotProduct(triangle->getNormalVector().normalise()));
            dotProductP1 = std::max(0.0L, (object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - (triangle->getPoint1() + object->getPositionAtPointInTime())).normalise().dotProduct(triangle->getPoint1Normal().normalise()));
            dotProductP2 = std::max(0.0L, (object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - (triangle->getPoint2() + object->getPositionAtPointInTime())).normalise().dotProduct(triangle->getPoint2Normal().normalise()));
            dotProductP3 = std::max(0.0L, (object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - (triangle->getPoint3() + object->getPositionAtPointInTime())).normalise().dotProduct(triangle->getPoint3Normal().normalise()));

            // No go through children and parents, to check if something is in between the light source (currently just the first star) and the triangle
            if(object->getParent()->getType() != GALACTIC_CORE && object->getParent()->getType() != STAR){
                if(((object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - object->getPositionAtPointInTime()).dotProduct(object->getParent()->getPositionAtPointInTime() - object->getPositionAtPointInTime())) > 0){
                    Plane parentPlane = Plane(object->getParent()->getPositionAtPointInTime(), object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - (triangle->getMidPoint() + object->getPositionAtPointInTime()));
                    PositionVector intersection = parentPlane.findIntersectionWithLine(object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime(), (triangle->getMidPoint() + object->getPositionAtPointInTime()));
                    long double distanceIntersectionParent = (object->getParent()->getPositionAtPointInTime() - intersection).getLength();
                    if(distanceIntersectionParent <= 0.5 * object->getParent()->getRadius()){
                        dotProductNormalVector = 0;
                        dotProductP1 = 0;
                        dotProductP2 = 0;
                        dotProductP3 = 0;
                    }
                    else if(distanceIntersectionParent <= object->getParent()->getRadius()){
                        // Here we compute this factor with the distance of the intersection to the midpoint to not have to do 3 more calculations that change little
                        long double factorMidPoint = ((distanceIntersectionParent/object->getParent()->getRadius()) - 0.5) * 2;

                        // This caused anomalies. We now calculate three different factors!
                        // That wasn't it. This change did little!!
                        // ---------------------------------------- TODO ---------------------------------------- 
                        // Maybe remove this again
                        PositionVector intersectionP1 = parentPlane.findIntersectionWithLine(object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime(), (triangle->getPoint1() + object->getPositionAtPointInTime()));
                        PositionVector intersectionP2 = parentPlane.findIntersectionWithLine(object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime(), (triangle->getPoint2() + object->getPositionAtPointInTime()));
                        PositionVector intersectionP3 = parentPlane.findIntersectionWithLine(object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime(), (triangle->getPoint3() + object->getPositionAtPointInTime()));
                        long double distanceIntersectionP1Parent = (object->getParent()->getPositionAtPointInTime() - intersectionP1).getLength();
                        long double distanceIntersectionP2Parent = (object->getParent()->getPositionAtPointInTime() - intersectionP2).getLength();
                        long double distanceIntersectionP3Parent = (object->getParent()->getPositionAtPointInTime() - intersectionP3).getLength();
                        long double factorP1 = ((distanceIntersectionP1Parent/object->getParent()->getRadius()) - 0.5) * 2;
                        long double factorP2 = ((distanceIntersectionP2Parent/object->getParent()->getRadius()) - 0.5) * 2;
                        long double factorP3 = ((distanceIntersectionP3Parent/object->getParent()->getRadius()) - 0.5) * 2;

                        dotProductNormalVector *= factorMidPoint;
                        dotProductP1 *= factorP1;
                        dotProductP2 *= factorP2;
                        dotProductP3 *= factorP3;
                    }
                }
            }
            // We don't go trough all children, this is way to expensive and solar eclipses way to rare for that
            // for(StellarObject *child: *(object->getChildren())){
            //     // If the child is behind the object, we ignore it
            //     if(((object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - object->getPositionAtPointInTime()).dotProduct(child->getPositionAtPointInTime() - object->getPositionAtPointInTime())) <= 0){
            //         continue;
            //     }
            //     Plane childPlane = Plane(child->getPositionAtPointInTime(), object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - (triangle->getMidPoint() + object->getPositionAtPointInTime()));
            //     PositionVector intersection = childPlane.findIntersectionWithLine(object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime(), (triangle->getMidPoint() + object->getPositionAtPointInTime()));
            //     long double distanceIntersectionChild = (child->getPositionAtPointInTime() - intersection).getLength(); 
                
            //     // printf("Checking if %s is in front of %s. Relative distance of intersection is %Lf\n", child->getName(), object->getName(), distanceIntersectionChild/child->getRadius());
            //     if(distanceIntersectionChild <= 0.5 * child->getRadius()){
            //         dotProductNormalVector = 0;
            //     }
            //     else if(distanceIntersectionChild <= child->getRadius()){
            //         dotProductNormalVector *= ((distanceIntersectionChild/child->getRadius()) - 0.5) * 2;
            //     }
            // }
        }
        else{
            // We slightly alter colour if the normal vector of the triangles point into the wrong direction from
            dotProductNormalVector = (std::max(0.0L, (absoluteCameraPosition - (triangle->getMidPoint() + object->getPositionAtPointInTime())).normalise().dotProduct(triangle->getNormalVector().normalise())) + 1) / 2;
            dotProductP1 = (std::max(0.0L, (absoluteCameraPosition - (triangle->getPoint1() + object->getPositionAtPointInTime())).normalise().dotProduct(triangle->getPoint1Normal().normalise())) + 1) / 2;
            dotProductP2 = (std::max(0.0L, (absoluteCameraPosition - (triangle->getPoint2() + object->getPositionAtPointInTime())).normalise().dotProduct(triangle->getPoint2Normal().normalise())) + 1) / 2;
            dotProductP3 = (std::max(0.0L, (absoluteCameraPosition - (triangle->getPoint3() + object->getPositionAtPointInTime())).normalise().dotProduct(triangle->getPoint3Normal().normalise())) + 1) / 2;
            // if((xP1 == 400 && yP1 == 300)){
            //     printf("Point1 (400, 300) has dotProduct %Lf. Normal is: (%s)\n", dotProductP1, triangle->getPoint1Normal().toString());
            // }
            // if((xP2 == 400 && yP2 == 300)){
            //     printf("Point2 (400, 300) has dotProduct %Lf. Normal is: (%s)\n", dotProductP2, triangle->getPoint2Normal().toString());
            // }
            // if((xP3 == 400 && yP3 == 300)){
            //     printf("Point3 (400, 300) has dotProduct %Lf. Normal is: (%s)\n", dotProductP3, triangle->getPoint3Normal().toString());
            // }
        }
            
        int backgroundRed = BACKGROUND_COL >> 16;
        int backgroundGreen = (BACKGROUND_COL >> 8) & (0b11111111);
        int backgroundBlue = BACKGROUND_COL & (0b11111111);

        int red = colour >> 16;
        int green = (colour >> 8) & (0b11111111);
        int blue = colour & (0b11111111);
        int redP1 = colourP1 >> 16;
        int greenP1 = (colourP1 >> 8) & (0b11111111);
        int blueP1 = colourP1 & (0b11111111);
        int redP2 = colourP2 >> 16;
        int greenP2 = (colourP2 >> 8) & (0b11111111);
        int blueP2 = colourP2 & (0b11111111);
        int redP3 = colourP3 >> 16;
        int greenP3 = (colourP3 >> 8) & (0b11111111);
        int blueP3 = colourP3 & (0b11111111);
        // Make sure that the colour is at least the background colour
        colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*dotProductNormalVector)) << 16)
        + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*dotProductNormalVector)) << 8)
        + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*dotProductNormalVector));

        colourP1 = ((int)(backgroundRed + (std::max(0, redP1-backgroundRed)*dotProductP1)) << 16)
        + ((int)(backgroundGreen + (std::max(0, greenP1-backgroundGreen)*dotProductP1)) << 8)
        + (int)(backgroundBlue + (std::max(0, blueP1-backgroundBlue)*dotProductP1));

        colourP2 = ((int)(backgroundRed + (std::max(0, redP2-backgroundRed)*dotProductP2)) << 16)
        + ((int)(backgroundGreen + (std::max(0, greenP2-backgroundGreen)*dotProductP2)) << 8)
        + (int)(backgroundBlue + (std::max(0, blueP2-backgroundBlue)*dotProductP2));

        colourP3 = ((int)(backgroundRed + (std::max(0, redP3-backgroundRed)*dotProductP3)) << 16)
        + ((int)(backgroundGreen + (std::max(0, greenP3-backgroundGreen)*dotProductP3)) << 8)
        + (int)(backgroundBlue + (std::max(0, blueP3-backgroundBlue)*dotProductP3));

        // printf("Adding triangle (%d, %d), (%d, %d), (%d, %d)\n", xP1, yP1, xP2, yP2, xP3, yP3);
        drawObjectsToAdd.push_back(new DrawObject(colour, xP1, yP1, xP2, yP2, xP3, yP3, distanceMidPoint.getLength(), TRIANGLE));
        drawObjectsToAdd.back()->colourP1 = colourP1;
        drawObjectsToAdd.back()->colourP2 = colourP2;
        drawObjectsToAdd.back()->colourP3 = colourP3;
        // printf("Added something to drawObjectsToAdd\n");
        // objectsToAddOnScreen->push_back(drawObject);
        // delete triangle;
    }
    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // int updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
    // printf("Rendering %d triangles from %d took %d mics\n", amount, start, updateTime);
    drawObjectLock->lock();
    for(DrawObject *drawObjectToAdd: drawObjectsToAdd){
        drawObject->addDrawObject(drawObjectToAdd);
    }
    drawObjectLock->unlock();
}

// Naive way to multithread while quicksorting: we just assume both sides are about half
void quicksort(int start, int end, std::vector<DrawObject*> *vector, int usedThreads, int maxThreads){
    if(start<end){
        int pivotPosition = quicksortInsert(start, end, vector);
        if(usedThreads > maxThreads/2){
            quicksort(start, pivotPosition, vector, usedThreads, maxThreads);
            quicksort(pivotPosition + 1, end, vector, usedThreads, maxThreads);
        }
        else{
            std::thread thread = std::thread(quicksort, start, pivotPosition, vector, usedThreads*2, maxThreads);
            quicksort(pivotPosition + 1, end, vector, usedThreads*2, maxThreads);
            thread.join();
        }
    }
}

int quicksortInsert(int start, int end, std::vector<DrawObject*> *vector){
    int pivot = end-1;

    int insertPosition = start;

    for(int currentPosition = start;currentPosition<end;currentPosition++){
        if((vector->at(currentPosition))->distance > (vector->at(pivot))->distance){
            std::swap(vector->at(currentPosition), vector->at(insertPosition++));
        }
    }
    std::swap(vector->at(pivot), vector->at(insertPosition));

    return insertPosition;
}