#include <X11/Xlib.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <thread>
#include <unistd.h>
#include "renderer.hpp"
#include "star.hpp"
#include "point2d.hpp"
#include "plane.hpp"

// TODO: For background, make array for every pixel and add up brightness?

// TODO ------------------------------------------------------------------------------------ FIX RENDERER

Renderer::Renderer(MyWindow *myWindow, std::vector<StellarObject*> *galaxies, std::vector<StellarObject*> *allObjects, Date *date, std::mutex *currentlyUpdatingOrDrawingLock, int *optimalTimeLocalUpdate){
    // printf("Renderer constructor begin\n");
    this->myWindow = myWindow;
    this->galaxies = galaxies;
    this->allObjects = allObjects;
    this->myWindow->setRenderer(this);
    this->date = date;
    this->currentlyUpdatingOrDrawingLock = currentlyUpdatingOrDrawingLock;
    this->optimalTimeLocalUpdate = optimalTimeLocalUpdate;
    // cameraPosition = PositionVector(0, 0, 10 * astronomicalUnit);
    // cameraDirection = PositionVector(0, 0, -1);
    // cameraPlaneVector1 = PositionVector(1, 0, 0);
    // cameraPlaneVector2 = PositionVector(0, 1, 0);
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
    XGetGeometry(myWindow->getDisplay(), *(myWindow->getWindow()), &rootWindow, &tempX, &tempY, &windowWidth, &windowHeight, &borderWidth, &depth);
    XSetWindowBackground(myWindow->getDisplay(), *(myWindow->getWindow()), BACKGROUND_COL);

    // Light gray top bar
    drawRect(GREY_COL, 0, 0, windowWidth, TOPBAR_HEIGHT);
    // Dark grey corner top right
    drawRect(DARK_GREY_COL, windowWidth-100, 0, 100, TOPBAR_HEIGHT);
    // Border between the top right corner and the top bar
    drawLine(BLACK_COL, windowWidth-100, 0, windowWidth-100, TOPBAR_HEIGHT);
    // Dark grey corner top left
    drawRect(DARK_GREY_COL, 0, 0, 200, TOPBAR_HEIGHT);
    // Border between the top left corner and the top bar
    drawLine(BLACK_COL, 200, 0, 200, TOPBAR_HEIGHT);
    // Border between top bar and main display
    drawLine(BLACK_COL, 0, TOPBAR_HEIGHT, windowWidth, TOPBAR_HEIGHT);

    drawString(WHITE_COL, 12, 40, "Loading...");

    XdbeSwapInfo swap_info = {*(myWindow->getWindow()), 1};
    XdbeSwapBuffers(myWindow->getDisplay(), &swap_info, 1);
}

void Renderer::draw(){
    // printf("Trying to draw\n");
    XGetGeometry(myWindow->getDisplay(), *(myWindow->getWindow()), &rootWindow, &tempX, &tempY, &windowWidth, &windowHeight, &borderWidth, &depth);
    XSetWindowBackground(myWindow->getDisplay(), *(myWindow->getWindow()), 0x111111);

    drawObjects();
    drawUI();

    // Debugging purposes
    // for(int i=std::max(1, (signed int) ((signed int)(dataPoints.size())-windowWidth));i<dataPoints.size();i++){
    //     drawLine(WHITE_COL, i%windowWidth-1, (dataPoints[i-1]-370000000.0)/50000000*windowHeight, i%windowWidth, (dataPoints[i]-370000000.0)/50000000*windowHeight);
    // }
    // for(int i=0;i<dataPoints2.size();i++){
    //     drawPoint(RED_COL, dataPoints2[i]/100000000.0*(windowHeight/2)+windowWidth/2, dataPoints3[i]/100000000.0*(windowHeight/2)+windowHeight/2);
    // }

    // XClearWindow(myWindow->getDisplay(), *(myWindow->getWindow()));          // Not neccessary, probably because of the swap
    XdbeSwapInfo swap_info = {*(myWindow->getWindow()), 1};
    XdbeSwapBuffers(myWindow->getDisplay(), &swap_info, 1);
}

void Renderer::drawObjects(){
    // ------------------------------------------------------------ TODO ------------------------------------------------------------
    // This function should be executed by the GPU!!!!! - Use OpenCL
    // struct timespec prevTime;
	// struct timespec currTime;
	// clock_gettime(CLOCK_MONOTONIC, &prevTime);

    // Go through all objects and update their positionAtPointInTime
    currentlyUpdatingOrDrawingLock->lock();
    for(StellarObject *stellarObject: *allObjects){
        stellarObject->updatePositionAtPointInTime();
    }
    currentlyUpdatingOrDrawingLock->unlock();

    // Then render all with the positionAtPointInTime
    int threadNumber = rendererThreadCount;
    int amount = allObjects->size()/threadNumber;
    std::vector<std::thread> threads;
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

    // Then we render close objects seperately
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
    quicksort(0, objectsOnScreen.size(), &objectsOnScreen, 1, rendererThreadCount);
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
    // Light gray top bar
    drawRect(GREY_COL, 0, 0, windowWidth, TOPBAR_HEIGHT);
    // Dark grey corner top right
    drawRect(DARK_GREY_COL, windowWidth-100, 0, 100, TOPBAR_HEIGHT);
    // Border between the top right corner and the top bar
    drawLine(BLACK_COL, windowWidth-100, 0, windowWidth-100, TOPBAR_HEIGHT);
    // Dark grey corner top left
    drawRect(DARK_GREY_COL, 0, 0, 200, TOPBAR_HEIGHT);
    // Border between the top left corner and the top bar
    drawLine(BLACK_COL, 200, 0, 200, TOPBAR_HEIGHT);
    // Border between top bar and main display
    drawLine(BLACK_COL, 0, TOPBAR_HEIGHT, windowWidth, TOPBAR_HEIGHT);

    // Display center object
    std::string toDisplay;
    if(centreObject != NULL){
        toDisplay = std::string(centreObject->getName());
        toDisplay.insert(0, "Centre: ");
        // str().insert(0, "Centre object: ")
        drawString(BLACK_COL, 15, 20, toDisplay.c_str());                 // Displays current centre object
    }

    // Display reference object
    if(referenceObject != NULL){
        toDisplay = std::string(referenceObject->getName());
        toDisplay.insert(0, "Reference: ");
        // str().insert(0, "Centre object: ")
        drawString(BLACK_COL, 15, 40, toDisplay.c_str());                 // Displays current reference object
    }
    // Display date
    drawString(BLACK_COL, windowWidth-75, 25, date->toString(true));
    drawString(BLACK_COL, windowWidth-60, 40, date->timeToString());
    // printf("%s\n", date->toString(false));

}

int Renderer::drawPoint(unsigned int col, int x, int y){
    XSetForeground(myWindow->getDisplay(), *(myWindow->getGC()), col);
    XDrawPoint(myWindow->getDisplay(), *(myWindow->getBackBuffer()), *(myWindow->getGC()), x, y);
    return 0;
}

int Renderer::drawLine(unsigned int col, int x1, int y1, int x2, int y2){
    // if(!visibleOnScreen(x1, y1) && !visibleOnScreen(x2, y2)) return 0;
    // // Make sure no fancy visual bugs get displayed
    int testWindowWidth = windowWidth;                      // Fuck knows why this is neccessary
    int testWindowHeight = windowHeight;                    // Fuck knows why this is neccessary
    // // printf("Would draw line from %d, %d to %d, %d\n", x1, y1, x2, y2);
    // while(x1>2*testWindowWidth || x1<(-1 * testWindowWidth) || y1>2*testWindowHeight || y1<(-1 * testWindowHeight)){
    //     x1 = (x1 - testWindowWidth/2) / 2 + testWindowWidth/2;
    //     y1 = (y1 - testWindowHeight/2) / 2 + testWindowHeight/2;
    // }
    // while(x2>2*testWindowWidth || x2<(-1 * testWindowWidth) || y2>2*testWindowHeight || y2<(-1 * testWindowHeight)){
    //     x2 = (x2 - testWindowWidth/2) / 2 + testWindowWidth/2;
    //     y2 = (y2 - testWindowHeight/2) / 2 + testWindowHeight/2;
    // }
    // // printf("But now drawing line from %d, %d to %d, %d\n", x1, y1, x2, y2);
    // if(col == RED_COL) printf("Would draw line from %d, %d to %d, %d\n", x1, y1, x2, y2);

    // New approach, cut lines
    bool p1Visible = visibleOnScreen(x1, y1);
    bool p2Visible = visibleOnScreen(x2, y2);
    Point2d edgePoint;
    // First handle case that both are not visible
    if(!p1Visible && !p2Visible){
        // printf("Would draw line from %d, %d to %d, %d ------------------------------\n", x1, y1, x2, y2);
        edgePoint = calculateEdgePointWithNoneVisible(x1, y1, x2, y2);
        if(edgePoint.getX() == -1 && edgePoint.getY() == -1) return 1;
        x1 = edgePoint.getX();
        y1 = edgePoint.getY();
        p1Visible = true;
        // printf("P1 and P2 not visible: now drawing line from %d, %d to %d, %d\n", x1, y1, x2, y2);
    }

    if(!p1Visible){
        // printf("Would draw line from %d, %d to %d, %d\n", x1, y1, x2, y2);
        edgePoint = calculateEdgePointWithOneVisible(x1, y1, x2, y2);
        x1 = edgePoint.getX();
        y1 = edgePoint.getY();
        // printf("P1 not visible: now drawing line from %d, %d to %d, %d\n", x1, y1, x2, y2);
    }
    else if(!p2Visible){
        // printf("Would draw line from %d, %d to %d, %d\n", x1, y1, x2, y2);
        edgePoint = calculateEdgePointWithOneVisible(x2, y2, x1, y1);
        x2 = edgePoint.getX();
        y2 = edgePoint.getY();
        // printf("P2 not visible: now drawing line from %d, %d to %d, %d\n", x1, y1, x2, y2);
    }
    // if(col == RED_COL) printf("Draw line from %d, %d to %d, %d\n", x1, y1, x2, y2);
    XSetForeground(myWindow->getDisplay(), *(myWindow->getGC()), col);
    XDrawLine(myWindow->getDisplay(), *(myWindow->getBackBuffer()), *(myWindow->getGC()), x1, y1, x2, y2);
    return 0;
}

Point2d Renderer::calculateEdgePointWithOneVisible(int x1, int y1, int x2, int y2){
    // We look at the vectors from p2 to the edges, f.e p2-(0/0) and p2-(windowWidth/0), and check if the vector p2-p1 is
    // to the right or left of each vector (width the cross product). We find out which edge the line crosses
    // printf("Calculating edge point with one visible: starting points: (%d, %d), (%d, %d)\n", x1, y1, x2, y2);
    int xOriginVector = -x2;
    int yOriginVector = -y2;
    int xWidthVector = windowWidth - x2;
    int yWidthVector = -y2;
    int xWidthHeightVector = windowWidth - x2;
    int yWidthHeightVector = windowHeight - y2;
    int xHeightVector = -x2;
    int yHeightVector = windowHeight - y2;
    int xPointsVector = x1 - x2;
    int yPointsVector = y1 - y2;
    int crossProductOriginVector = xOriginVector * yPointsVector -  yOriginVector * xPointsVector;
    int crossProductWidthVector = xWidthVector * yPointsVector -  yWidthVector * xPointsVector;
    int crossProductWidthHeightVector = xWidthHeightVector * yPointsVector -  yWidthHeightVector * xPointsVector;
    int crossProductHeightVector = xHeightVector * yPointsVector -  yHeightVector * xPointsVector;
    int xDiff, yDiff;
    if(xPointsVector == 0){
        if(yPointsVector>0){
            crossProductOriginVector = -1;
            crossProductWidthVector = -1;
            crossProductWidthHeightVector = 1;
            crossProductHeightVector = -1;
        }
        else{
            crossProductOriginVector = 1;
            crossProductWidthVector = -1;
            crossProductWidthHeightVector = -1;
            crossProductHeightVector = -1;
        }
    }
    if(yPointsVector == 0){
        if(xPointsVector>0){
            crossProductOriginVector = -1;
            crossProductWidthVector = 1;
            crossProductWidthHeightVector = -1;
            crossProductHeightVector = -1;
        }
        else{
            crossProductOriginVector = -1;
            crossProductWidthVector = -1;
            crossProductWidthHeightVector = -1;
            crossProductHeightVector = 1;
        }
    }
    if(crossProductWidthVector > 0 && crossProductWidthHeightVector < 0){
        // printf("Edge x = windowWidth\n");
        // Edge x = windowWidth
        xDiff = windowWidth - x2;
        yDiff = yPointsVector * xDiff / xPointsVector;
        // printf("Danger past\n");
    }
    else if(crossProductOriginVector > 0 && crossProductWidthVector < 0){
        // printf("Edge y = 0\n");
        // Edge y = 0
        yDiff = -y2;
        xDiff = xPointsVector * yDiff / yPointsVector;
        // printf("Danger past\n");
    }
    else if(crossProductWidthHeightVector > 0 && crossProductHeightVector < 0){
        // printf("Edge y = windowHeight\n");
        // Edge y = windowHeight
        yDiff = windowHeight - y2;
        xDiff = xPointsVector * yDiff / yPointsVector;
        // printf("Danger past\n");
    }
    else{
        // printf("Edge x = 0\n");
        // Edge x = 0
        xDiff = -x2;
        yDiff = yPointsVector * xDiff / xPointsVector;
        // printf("Danger past\n");
    }

    Point2d output(x2 + xDiff, y2 + yDiff);

    // printf("Calculated edge point with one visible: starting points: (%d, %d), (%d, %d), end point: (%d, %d)\n", x1, y1, x2, y2, output.x, output.y);

    return output;
}

Point2d Renderer::calculateEdgePointWithNoneVisible(int x1, int y1, int x2, int y2){
    // printf("Calculating edge point with none visible: (%d, %d), (%d, %d)\n", x1, y1, x2, y2);
    int testWindowWidth = windowWidth;                      // Fuck knows why this is neccessary
    int testWindowHeight = windowHeight;                    // Fuck knows why this is neccessary

    Point2d output(-1, -1);
    // We initialise output as -1/-1. If we return that, we know that the function has failed and we can safely return

    // First we find out which edge is crossed first. We build the line equation and fill in the appropriate values, with which we hit one of the edges
    // x = x1 + s * (x2 - x1), y = y1 + s * (y2 - y1)
    double edgesS[4];
    if(x2 - x1 != 0){
        edgesS[0] = (0 - x1) / (x2 - x1 + 0.0);                     // X=0 edge
        edgesS[1] = (testWindowWidth - x1) / (x2 - x1 + 0.0);       // X=windowWidth edge
    }
    else{
        edgesS[0] = 2;
        edgesS[1] = 2;
    }
    if(y2 - y1 != 0){
        edgesS[2] = (0 - y1) / (y2 - y1 + 0.0);                     // Y=0 edge
        edgesS[3] = (testWindowHeight - y1) / (y2 - y1 + 0.0);      // Y=windowHeight edge
    }
    else{
        edgesS[2] = 2;
        edgesS[3] = 2;
    }
    // printf("The four esses: %f, %f, %f, %f, calcs: %d, %f, %f, %f\n", edgesS[0], edgesS[1], edgesS[2], edgesS[3], (testWindowWidth - x1), (x2 - x1 + 0.0), (testWindowWidth - x1) / (x2 - x1 + 0.0), (windowWidth - x1) / (x2 - x1 + 0.0));

    // The closest of these points, with s being positive, is the new point
    if(edgesS[0]<=0) edgesS[0] = 2;
    if(edgesS[1]<=0) edgesS[1] = 2;
    if(edgesS[2]<=0) edgesS[2] = 2;
    if(edgesS[3]<=0) edgesS[3] = 2;
    double smallestS = std::min(std::min(std::min(edgesS[0], edgesS[1]), edgesS[2]), edgesS[3]);
    // If this closest s is larger than 1, we don't have to draw anything
    // printf("Smallest s: %f\n", smallestS);
    if(smallestS>=1) return output;
    // Else select the smallest s than leads to a visible point
    int newX = 0;
    int newY = 0;
    double currentS = 2;
    for(int i=0;i<4;i++){
        if(edgesS[i]<0) continue;
        int tempX = x1 + edgesS[i] * (x2 - x1);
        int tempY = y1 + edgesS[i] * (y2 - y1);
        if(edgesS[i]<currentS && visibleOnScreen(tempX, tempY)){
            newX = tempX;
            newY = tempY;
            currentS = edgesS[i];
            // printf("Took edge number %d\n", i);
        }
    }
    // printf("Decided on final S: %f, output: (%d, %d)\n", currentS, output.x, output.y);
    if(currentS >= 1) return output;
    // Now replace the point (x1,y1) with the newly found point, which means p1 is now visible
    output.setX(newX);
    output.setY(newY);
    // printf("Calculated edge point with none visible. Returning: (%d, %d)\n", output.x, output.y);
    return output;
}

int Renderer::drawRect(unsigned int col, int x, int y, int width, int height){
    XSetForeground(myWindow->getDisplay(), *(myWindow->getGC()), col);
    XFillRectangle(myWindow->getDisplay(), *(myWindow->getBackBuffer()), *(myWindow->getGC()), x, y, width, height);
    return 0;
}

int Renderer::drawCircle(unsigned int col, int x, int y, int diam){
    XSetForeground(myWindow->getDisplay(), *(myWindow->getGC()), col);
    XDrawArc(myWindow->getDisplay(), *(myWindow->getBackBuffer()), *(myWindow->getGC()), x-diam/2, y-diam/2, diam, diam, 0, 360 * 64);
    XFillArc(myWindow->getDisplay(), *(myWindow->getBackBuffer()), *(myWindow->getGC()), x-diam/2, y-diam/2, diam, diam, 0, 360 * 64);
    return 0;
}

int Renderer::drawString(unsigned int col, int x, int y, const char *stringToBe){
    XSetForeground(myWindow->getDisplay(), *(myWindow->getGC()), col);
    XDrawString(myWindow->getDisplay(), *(myWindow->getBackBuffer()), *(myWindow->getGC()), x, y, stringToBe, strlen(stringToBe));
    return 0;
}

int Renderer::drawTriangle(unsigned int col, int x1, int y1, int x2, int y2, int x3, int y3){
    // printf("Trying to draw triangle with points: (%d, %d), (%d, %d), (%d, %d)\n", x1, y1, x2, y2, x3, y3);
    Point2d points[3];
    points[0] = Point2d(x1, y1);
    points[1] = Point2d(x2, y2);
    points[2] = Point2d(x3, y3);
    // printf("Trying to draw triangle with pointsInXpoint: (%d, %d), (%d, %d), (%d, %d)\n", points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);

    bool p1Visible = visibleOnScreen(x1, y1);
    bool p2Visible = visibleOnScreen(x2, y2);
    bool p3Visible = visibleOnScreen(x3, y3);
    // if(!p1Visible || !p2Visible || !p3Visible){
    //     return 0;   
    // }
    if(!p1Visible && !p2Visible && !p3Visible){
        drawTriangleAllNotVisible(col, points);
        return 0;
    }
    else if(!p1Visible && !p2Visible){
        drawTriangleTwoNotVisible(col, points, 2);
        return 0;
    }
    else if(!p1Visible && !p3Visible){
        drawTriangleTwoNotVisible(col, points, 1);
        return 0;
    }
    else if(!p3Visible && !p2Visible){
        drawTriangleTwoNotVisible(col, points, 0);
        return 0;
    }
    else if(!p1Visible){
        drawTriangleOneNotVisible(col, points, 0);
        return 0;
    }
    else if(!p2Visible){
        drawTriangleOneNotVisible(col, points, 1);
        return 0;
    }
    else if(!p3Visible){
        drawTriangleOneNotVisible(col, points, 2);
        return 0;
    }

    XPoint xPoints[3];
    xPoints[0].x = x1;
    xPoints[0].y = y1;
    xPoints[1].x = x2;
    xPoints[1].y = y2;
    xPoints[2].x = x3;
    xPoints[2].y = y3;

    XSetForeground(myWindow->getDisplay(), *(myWindow->getGC()), col);
    XFillPolygon(myWindow->getDisplay(), *(myWindow->getBackBuffer()), *(myWindow->getGC()), xPoints, 3, Convex, CoordModeOrigin);
    // printf("Drawing triangle in renderer (%d, %d), (%d, %d), (%d, %d)\n", x1, y1, x2, y2, x3, y3);
    // XDrawPolygon(myWindow->getDisplay(), *(myWindow->getBackBuffer()), *(myWindow->getGC()), points, 3, Convex, CoordModeOrigin);
    // drawLine(BLACK_COL, x1, y1, x2, y2);
    // drawLine(BLACK_COL, x1, y1, x3, y3);
    // drawLine(BLACK_COL, x3, y3, x2, y2);
    return 0;
}

int Renderer::drawPolygon(unsigned int col, short count, Point2d *points, bool checks){
    // ------------------------------------------------------------------- TODO -------------------------------------------------------------------
    // If the polygon does not come from the drawTriangle function, we need some checks to catch anomalies
    if(checks){

    }

    XPoint xPoints[count];
    for(int i=0;i<count;i++){
        xPoints[i].x = points[i].getX();
        xPoints[i].y = points[i].getY();
    }


    XSetForeground(myWindow->getDisplay(), *(myWindow->getGC()), col);
    XFillPolygon(myWindow->getDisplay(), *(myWindow->getBackBuffer()), *(myWindow->getGC()), xPoints, count, Convex, CoordModeOrigin);
    // for(int i=0;i<count;i++){
    //     // printf("Trying to draw line from (%d, %d) to (%d, %d)\n", points[i].x, points[i].y, points[(i+1)%count].x, points[(i+1)%count].y);
    //     drawLine(RED_COL, points[i].getX(), points[i].getY(), points[(i+1)%count].getX(), points[(i+1)%count].getY());
    // }
    return 0;
}

int Renderer::drawTriangleAllNotVisible(unsigned int col, Point2d *points){
    // printf("Drawing all not visible with points: (%s), (%s), (%s)\n", points[0].toString(), points[1].toString(), points[2].toString());
    int testWindowWidth = windowWidth;                      // Fuck knows why this is neccessary
    int testWindowHeight = windowHeight;                    // Fuck knows why this is neccessary
    if((points[0].getX()<0 && points[1].getX()<0 && points[2].getX()<0) ||
    (points[0].getY()<0 && points[1].getY()<0 && points[2].getY()<0) ||
    (points[0].getX()>testWindowWidth && points[1].getX()>testWindowWidth && points[2].getX()>testWindowWidth) ||
    (points[0].getY()>testWindowHeight && points[1].getY()>testWindowHeight && points[2].getY()>testWindowHeight)){
        // printf("Not visible--------------------\n");
        return 1;
    }

    std::vector<Point2d> drawPointsVector;
    Point2d drawPoints[5];
    Point2d edgePoint;
    Point2d edgePoint2;
    // Only relevant for case where only ond line intersects canvas
    short indexOfPointNotUsed;
    // First we go through all three point pairs and check if the line between them intersects the canvas.
    for(int i=0;i<3;i++){
        edgePoint = calculateEdgePointWithNoneVisible(points[i].getX(), points[i].getY(), points[(i+1)%3].getX(), points[(i+1)%3].getY());
        // printf("Edge point i = %d: (%d, %d)\n", i, edgePoint.x, edgePoint.y);
        if(edgePoint.getY() == -1 && edgePoint.getY() == -1) continue;
        edgePoint2 = calculateEdgePointWithOneVisible(points[(i+1)%3].getX(), points[(i+1)%3].getY(), edgePoint.getX(), edgePoint.getY());
        drawPointsVector.push_back(edgePoint);
        drawPointsVector.push_back(edgePoint2);
        indexOfPointNotUsed = (i+2)%3;
    }
    // If only one line intersected the canvas, we know that we have to add at least one corner, so that we can draw a triangle
    // If we have no intersections, we don't draw the triangle, else we draw the polygon with the found points on the edges
    int drawPointsVectorSize = drawPointsVector.size();
    if(drawPointsVectorSize == 0) return 1;
    if(drawPointsVectorSize == 2){
        // printf("Drawing triangle with only two intersection points: (%d, %d), (%d, %d)\n", drawPointsVector[0].x, drawPointsVector[0].y, drawPointsVector[1].x, drawPointsVector[1].y);
        // First we check if the third, unused point is to the right or left of the line going through the intersect points
        // For that we insert the x value of the unused point into the line equation.
        // Equation: y = m * x + y0
        int xDiff = drawPointsVector[1].getX() - drawPointsVector[0].getX();
        int yDiff = drawPointsVector[1].getY() - drawPointsVector[0].getY();

        if(xDiff==0){
            return 1;
        }

        double slope = (yDiff) / (xDiff + 0.0);
        // printf("Slope: %f = (%d-%d) / (%d-%d)\n", slope, drawPointsVector[1].y, drawPointsVector[0].y, drawPointsVector[1].x, drawPointsVector[0].x);
        double y0 = drawPointsVector[0].getY() + (-drawPointsVector[0].getX())/(xDiff) * yDiff;
        // printf("y0: %f = %d + (-%d)/%d * %d\n", y0, drawPointsVector[0].y, drawPointsVector[0].x, xDiff, yDiff);

        // No we check the sign of the y value comparison of the unused point;
        double yLine = slope * points[indexOfPointNotUsed].getX() + y0;
        short sign = (points[indexOfPointNotUsed].getY() - yLine) / (std::abs(points[indexOfPointNotUsed].getY() - yLine));
        // printf("Point (%d, %d) has sign %d for line from (%d, %d) to (%d, %d) with equation y = %f * x + %f\n", points[indexOfPointNotUsed].x, points[indexOfPointNotUsed].y,
        // sign, drawPointsVector[0].x, drawPointsVector[0].y, drawPointsVector[1].x, drawPointsVector[1].y, slope, y0);

        // Now we go through all points, beginning with the one directly to the right of the second point

        Point2d corners[4];
        corners[0] = Point2d(testWindowWidth, 0);
        corners[1] = Point2d(testWindowWidth, testWindowHeight);
        corners[2] = Point2d(0, testWindowHeight);
        corners[3] = Point2d(0, 0);
        // printf("Corners: (%d, %d), (%d, %d), (%d, %d), (%d, %d)\n", corners[0].x, corners[0].y, corners[1].x, corners[1].y, corners[2].x, corners[2].y, corners[3].x, corners[3].y);
        short startIndex;;
        if(drawPoints[0].getY() == 0) startIndex = 0;
        else if(drawPoints[0].getX() == testWindowWidth) startIndex = 1;
        else if(drawPoints[0].getY() == testWindowHeight) startIndex = 2;
        else startIndex = 3;
        short cornerSign;
        double cornerYLine;
        for(int i=0;i<4;i++){
            cornerYLine = slope * corners[(i + startIndex)%4].getX() + y0;
            cornerSign = (corners[(i + startIndex)%4].getY() - cornerYLine) / (std::abs(corners[(i + startIndex)%4].getY() - cornerYLine));
            // printf("Corner (%d, %d) has sign %d for line from (%d, %d) to (%d, %d) with equation y = %f * x + %f\n", corners[(i + startIndex)%4].x, corners[(i + startIndex)%4].y,
            // cornerSign, drawPointsVector[0].x, drawPointsVector[0].y, drawPointsVector[1].x, drawPointsVector[1].y, slope, y0);
            if(cornerSign == sign){
                // printf("Adding corner (%d, %d)\n", corners[(i + startIndex)%4].x, corners[(i + startIndex)%4].y);
                drawPointsVector.push_back(corners[(i + startIndex)%4]);
            }
        }
    }
    drawPointsVectorSize = drawPointsVector.size();
    // printf("Drawing polygon with points: ");
    for(int i=0;i<drawPointsVectorSize;i++){
        drawPoints[i] = drawPointsVector[i];
        // printf("(%d, %d) ", drawPoints[i].x, drawPoints[i].y);
    }
    // printf("\n");
    drawPolygon(col, drawPointsVectorSize, drawPoints);
    // printf("Drawn all not visible with points: (%s), (%s), (%s)--------------------\n", points[0].toString(), points[1].toString(), points[2].toString());
    return 0;
}

int Renderer::drawTriangleTwoNotVisible(unsigned int col, Point2d *points, short indexVisible){
    // printf("Drawing two not visible with points: (%s), (%s), (%s)\n", points[0].toString(), points[1].toString(), points[2].toString());
    int testWindowWidth = windowWidth;                      // Fuck knows why this is neccessary
    int testWindowHeight = windowHeight;                    // Fuck knows why this is neccessary
    Point2d drawPoints[5];
    drawPoints[0] = points[(indexVisible)];
    // First we find the points which intersect
    Point2d intersect1 = calculateEdgePointWithOneVisible(points[(indexVisible+1)%3].getX(), points[(indexVisible+1)%3].getY(), points[indexVisible].getX(), points[indexVisible].getY());
    Point2d intersect2 = calculateEdgePointWithOneVisible(points[(indexVisible+2)%3].getX(), points[(indexVisible+2)%3].getY(), points[indexVisible].getX(), points[indexVisible].getY());

    if(intersect1.getX() == intersect2.getX() || intersect1.getY() == intersect2.getY()){
        // printf("Case two points not visible, two intersection points, which are on the same edge\n");
        // This means the intersection points are on the same edge
        drawPoints[1] = intersect2;
        drawPoints[2] = intersect1;
        drawPolygon(col, 3, drawPoints);
        return 0;
    }
    else{
        // Intersections points are not on the same edge. Now we have to check if the line between the invisible points intersects the canvas
        Point2d intersect3 = calculateEdgePointWithNoneVisible(points[(indexVisible+1)%3].getX(), points[(indexVisible+1)%3].getY(), points[(indexVisible+2)%3].getX(), points[(indexVisible+2)%3].getY());
        // printf("Intersectionpoint of invisible points: (%d, %d)\n", intersect3.x, intersect3.y);
        if(intersect3.getX() != -1 && intersect3.getY() != -1){
            // printf("Case two points not visible, four intersection points, which are not on the same edge\n");
            // Case line intersects canvas
            Point2d intersect4 = calculateEdgePointWithOneVisible(points[(indexVisible+2)%3].getX(), points[(indexVisible+2)%3].getY(), intersect3.getX(), intersect3.getY());
            drawPoints[1] = intersect1;
            drawPoints[2] = intersect3;
            drawPoints[3] = intersect4;
            drawPoints[4] = intersect2;
            drawPolygon(col, 5, drawPoints);
            // printf("Drawn two not visible with points: (%d, %d), (%d, %d), (%d, %d)--------------------\n", points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
            return 0;
        }
        else if(intersect1.getX() == 0 || intersect1.getX() == testWindowWidth){
            drawPoints[1] = intersect2;
            Point2d corner(intersect1.getX(), intersect2.getY());
            drawPoints[2] = corner;
            drawPoints[3] = intersect1;
            // printf("Case two points not visible, two intersection points, which are not on the same edge, corner %d, %d included\n", corner.x, corner.y);
        }
        else{
            drawPoints[1] = intersect2;
            Point2d corner(intersect2.getX(), intersect1.getY());
            drawPoints[2] = corner;
            drawPoints[3] = intersect1;
            // printf("Case two points not visible, two intersection points, which are not on the same edge, corner %d, %d included\n", corner.x, corner.y);
        }
        drawPolygon(col, 4, drawPoints);
    }
    // printf("Drawn two not visible with points: (%s), (%s), (%s)--------------------\n", points[0].toString(), points[1].toString(), points[2].toString());
    return 0;
}

int Renderer::drawTriangleOneNotVisible(unsigned int col, Point2d *points, short indexNotVisible){
    // printf("Drawing one not visible with points: (%s), (%s), (%s)\n", points[0].toString(), points[1].toString(), points[2].toString());
    int testWindowWidth = windowWidth;                      // Fuck knows why this is neccessary
    int testWindowHeight = windowHeight;                    // Fuck knows why this is neccessary
    Point2d drawPoints[5];
    drawPoints[0] = points[(indexNotVisible+1)%3];
    drawPoints[1] = points[(indexNotVisible+2)%3];
    // First we find the points which intersect
    Point2d intersect1 = calculateEdgePointWithOneVisible(points[indexNotVisible].getX(), points[indexNotVisible].getY(), points[(indexNotVisible+1)%3].getX(), points[(indexNotVisible+1)%3].getY());
    Point2d intersect2 = calculateEdgePointWithOneVisible(points[indexNotVisible].getX(), points[indexNotVisible].getY(), points[(indexNotVisible+2)%3].getX(), points[(indexNotVisible+2)%3].getY());
    // printf("One Point not visible intersection points: (%d, %d), (%d, %d)\n", intersect1.x, intersect1.y, intersect2.x, intersect2.y);
    if(intersect1.getX() == intersect2.getX() || intersect1.getY() == intersect2.getY()){
        // printf("Case one point not visible, two intersection points, which are on the same edge\n");
        // This means the intersection points are on the same edge
        drawPoints[2] = intersect2;
        drawPoints[3] = intersect1;
        // printf("Drawing polygon: (%d, %d), (%d, %d), (%d, %d), (%d, %d)\n", drawPoints[0].x, drawPoints[0].x, drawPoints[1].x, drawPoints[1].y, drawPoints[2].x, drawPoints[2].y, drawPoints[3].x, drawPoints[3].y);
        drawPolygon(col, 4, drawPoints);
        // printf("Drawn one not visible with points: (%d, %d), (%d, %d), (%d, %d)--------------------\n", points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
        return 0;
    }
    else if(intersect1.getX() == 0 || intersect1.getX() == testWindowWidth){
        drawPoints[2] = intersect2;
        Point2d corner(intersect1.getX(), intersect2.getY());
        drawPoints[3] = corner;
        drawPoints[4] = intersect1;
        // printf("Case one point not visible, two intersection points, which are not on the same edge, corner %d, %d included\n", corner.x, corner.y);
        // printf("Intersect points: (%d, %d), (%d, %d)\n", intersect1.x, intersect1.y, intersect2.x, intersect2.y);
    }
    else{
        drawPoints[2] = intersect2;
        Point2d corner(intersect2.getX(), intersect1.getY());
        drawPoints[3] = corner;
        drawPoints[4] = intersect1;
        // printf("Case one point not visible, two intersection points, which are not on the same edge, corner %d, %d included\n", corner.x, corner.y);
    }
    // printf("Drawing polygon: (%d, %d), (%d, %d), (%d, %d), (%d, %d), (%d, %d)\n", drawPoints[0].x, drawPoints[0].x, drawPoints[1].x, drawPoints[1].y, drawPoints[2].x, drawPoints[2].y, drawPoints[3].x, drawPoints[3].y, drawPoints[4].x, drawPoints[4].y);
    drawPolygon(col, 5, drawPoints);
    // printf("Drawn one not visible with points: (%s), (%s), (%s)--------------------\n", points[0].toString(), points[1].toString(), points[2].toString());
    return 0;
}

void Renderer::calculateObjectPosition(StellarObject *object, std::vector<DrawObject*> *objectsToAddOnScreen, std::vector<DrawObject*> *dotsToAddOnScreen){
    // First we calculate the distancevector from camera to object
    // PositionVector distance = object->getPosition() - referenceObject->getPosition() - cameraPosition; // This would be convenient but is comparatively much to slow
    PositionVector distance = PositionVector(object->getPositionAtPointInTime().getX()-referenceObject->getPositionAtPointInTime().getX()-cameraPosition.getX(), object->getPositionAtPointInTime().getY()-referenceObject->getPositionAtPointInTime().getY()-cameraPosition.getY(), object->getPositionAtPointInTime().getZ()-referenceObject->getPositionAtPointInTime().getZ()-cameraPosition.getZ());

    // We then make a basistransformation with the cameraDirection and cameraPlaneVectors
    // PositionStandardBasis = TransformationMatrix * PositionNewBasis
    // Because the basis is orthonormal, the inverse is just the transpose
    // -> InverseTransformationMatrix * PositionStandardBasis = PositionNewBasis
    PositionVector distanceNewBasis = inverseTransformationMatrixCameraBasis * distance;

    // Calculate size in pixels on screen
    long double size = object->getRadius() / distance.getLength() * (windowWidth/2);
    double sizeCutOff = 1.0/1000000000000;
    if(size < sizeCutOff){
        // printf("Exited calculateObjectPosition\n");
        return;
    }

    // If the z-coordinate is negative, it is definitely out of sight, namely behind the camera
    if(distanceNewBasis.getZ()<-object->getRadius()){
        // std::string *nameComparison = new std::string("Sagittarius A*");
        // if(strcmp(object->getName(), nameComparison->c_str()) == 0){
        //     printf("%s exited via z-coordinate constraint\n", object->getName());
        // }
        // delete nameComparison;
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
        x = (windowWidth/2) + std::floor(distanceNewBasis.getX() / distanceNewBasis.getZ() * (windowWidth/2));
        y = (windowHeight/2) + std::floor(distanceNewBasis.getY() / distanceNewBasis.getZ() * (windowWidth/2));
    }

    // If no part of the object would be on screen, we dont draw it - only applicable for circle drawn stellar objects
    if(size < 15 && (x - size > windowWidth || x + size < 0 || y - size > windowHeight || y + size < TOPBAR_HEIGHT)){
        return;
    }
    // ------------------------------------------------- TODO -------------------------------------------------
    int colour = object->getColour();
    float dotProductNormalVector;
    if(object->getType() != GALACTIC_CORE && object->getType() != STAR){
        colour = object->getHomeSystem()->getChildren()->at(0)->getColour();
        dotProductNormalVector = ((object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - object->getPositionAtPointInTime()).normalise().dotProduct(((cameraPosition + referenceObject->getPositionAtPointInTime()) - object->getPositionAtPointInTime()).normalise()) + 1) / 2;
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
    if(object->getType() == GALACTIC_CORE || object->getType() == STAR){
        if(size < 1.0/1000000){
            isPoint = true;
            // colour = ((int)(red*(1 + std::log(1000*size/2+0.5))) << 16) + ((int)(green*(1 + std::log(1000*size/2+0.5))) << 8) + (int)(blue*(1 + std::log(1000*size/2+0.5)));
            colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*(1 + std::log(1000000*size/2+0.0001)/log(100000)))) << 16)
            + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*(1 + std::log(1000000*size/2+0.0001)/log(100000)))) << 8)
            + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*(1 + std::log(1000000*size/2+0.0001)/log(100000))));
            // if(object->getType()==STAR) printf("After - Colour: %d, r: %d, g: %d, b: %d, size: %f\n", colour, (int)(red*(1 + std::log(1000*size/2+0.5))), (int)(green*(1 + std::log(1000*size/2+0.5))), (int)(blue*(1 + std::log(1000*size/2+0.5))), size);
        }
        else if(size < 1){
            colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*(1 + std::log(size/2+0.5)))) << 16)
            + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*(1 + std::log(size/2+0.5)))) << 8)
            + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*(1 + std::log(size/2+0.5))));
            // if(object->getType()==STAR) printf("After - Colour: %d, r: %d, g: %d, b: %d, size: %f\n", colour, (int)(red*(1 + std::log(size/2+0.5)))<<16, (int)(green*(1 + std::log(size/2+0.5)))<<8, (int)(blue*(1 + std::log(size/2+0.5))), size);
        }
    }
    else{
        if(size < 1.0/10000){
            // printf("Exited calculateObjectPosition\n");
            return;
        }
        // DotProductNormalForm does not work as intended
        else if(size < 1.0/100){
            isPoint = true;
            colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*(1 + std::log(100*size/2+0.5))*dotProductNormalVector)) << 16)
            + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*(1 + std::log(100*size/2+0.5))*dotProductNormalVector)) << 8)
            + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*(1 + std::log(100*size/2+0.5))*dotProductNormalVector));
        }
        else if(size < 1){
            colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*(1 + std::log(size/2+0.5))*dotProductNormalVector)) << 16)
            + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*(1 + std::log(size/2+0.5))*dotProductNormalVector)) << 8)
            + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*(1 + std::log(size/2+0.5))*dotProductNormalVector));
        }
        else{
            colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*dotProductNormalVector)) << 16)
            + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*dotProductNormalVector)) << 8)
            + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*dotProductNormalVector));
        }
    }
    // printf("Colour and size of %s: %d, %f\n", object->getName(), colour, size);

    DrawObject *drawObject;
    if(isPoint){
        if(visibleOnScreen(x, y)) {
            drawObject = new DrawObject(colour, x, y, distanceNewBasis.getLength(), POINT);
            dotsToAddOnScreen->push_back(drawObject);
        }
        // printf("Exited calculateObjectPosition\n");
        return;
    }
    else if(size<1){
        drawObject = new DrawObject(colour, x, y, distanceNewBasis.getLength(), PLUS);
        dotsToAddOnScreen->push_back(drawObject);
        drawObject = new DrawObject(originalColour, x, y, distanceNewBasis.getLength()-1, POINT);
        dotsToAddOnScreen->push_back(drawObject);
        return;
    }
    else if(size > 3){
        // Close objects are rendered seperately, to harness the full power of multithreading
        CloseObject *closeObject = new CloseObject();
        closeObject->object = object;
        closeObject->distanceNewBasis = distanceNewBasis;
        closeObject->size = size;
        closeObjects.push_back(closeObject);
        // calculateCloseObject(object, distanceNewBasis, size, objectsToAddOnScreen);
        return;
    }
    drawObject = new DrawObject(colour, x, y, size, distanceNewBasis.getLength(), CIRCLE);
    objectsToAddOnScreen->push_back(drawObject);

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

    int resolution = std::min(std::max((int)(size), 5), 45);
    // int resolution = std::min(std::max((int)(size/10), 5), 25);
    if(resolution % 2 == 0) resolution++;
    // resolution = 3;
    // printf("Resolution: %d\n", resolution);
    // First initialise the 6 different faces of the planet

    StellarObjectRenderFace *faces = object->getRenderFaces();
    faces[0].updateRenderFace(X_AXIS, FORWARDS, resolution);
    faces[1].updateRenderFace(X_AXIS, BACKWARDS, resolution);
    faces[2].updateRenderFace(Y_AXIS, FORWARDS, resolution);
    faces[3].updateRenderFace(Y_AXIS, BACKWARDS, resolution);
    faces[4].updateRenderFace(Z_AXIS, FORWARDS, resolution);
    faces[5].updateRenderFace(Z_AXIS, BACKWARDS, resolution);

    // Then gather all the triangles from the faces
    // clock_gettime(CLOCK_MONOTONIC, &prevTime);
    std::vector<RenderTriangle*> triangles;
    std::mutex trianglesLock;
    PositionVector absoluteCameraPosition = cameraPosition + referenceObject->getPositionAtPointInTime();
    if(rendererThreadCount>=6){
        std::vector<std::thread> threads;
        for(int i=0;i<6;i++){
            threads.push_back(std::thread(getRenderTrianglesMultiThread, &(faces[i]), &triangles, absoluteCameraPosition, &trianglesLock));
        }
        for(int i=0;i<6;i++){
            threads.at(i).join();
        }
        threads.clear();
    }
    else if(rendererThreadCount>=3){
        std::vector<std::thread> threads;
        for(int i=0;i<3;i++){
            threads.push_back(std::thread(getRenderTrianglesMultiThread, &(faces[i]), &triangles, absoluteCameraPosition, &trianglesLock));
        }
        for(int i=0;i<3;i++){
            threads.at(i).join();
        }
        for(int i=3;i<6;i++){
            threads.push_back(std::thread(getRenderTrianglesMultiThread, &(faces[i]), &triangles, absoluteCameraPosition, &trianglesLock));
        }
        for(int i=3;i<6;i++){
            threads.at(i).join();
        }
    }
    else{
        for(int i=0;i<6;i++){
            faces[i].getRenderTriangles(&triangles, absoluteCameraPosition);
        }
    }
    // printf("\n");
    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
    // printf("Getting all triangles took %d mics with %d threads\n", updateTime, rendererThreadCount);

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
        if(cameraPosition.getLength() / 1.5 >= centreObject->getRadius())
            cameraPosition /= 1.5;
        else if(cameraPosition.getLength() / 1.25 >= centreObject->getRadius())
            cameraPosition /= 1.25;
        else if(cameraPosition.getLength() / 1.1 >= centreObject->getRadius())
            cameraPosition /= 1.1;
        // printf("New cameraPosition: (%f, %f, %f), distance to centre: %fly\n", cameraPosition.getX(),cameraPosition.getY(), cameraPosition.getZ(), cameraPosition.getLength()/lightyear);
        return;
    }
    cameraPosition *= 1.5;
    // printf("New cameraPosition: (%f, %f, %f), distance to centre: %fly\n", cameraPosition.getX(),cameraPosition.getY(), cameraPosition.getZ(), cameraPosition.getLength()/lightyear);
}

void Renderer::resetCameraOrientation(){
    // ------------------------------------------------------------------------ TODO ------------------------------------------------------------------------
    cameraDirection = PositionVector(-1, 0, 0);
    cameraPlaneVector1 = PositionVector(0, 1, 0);
    cameraPlaneVector2 = PositionVector(0, 0, 1);

    cameraPosition = cameraDirection * -5 * centreObject->getRadius();

    transformationMatrixCameraBasis = Matrix3d(cameraPlaneVector1, cameraPlaneVector2, cameraDirection);
    inverseTransformationMatrixCameraBasis = transformationMatrixCameraBasis.transpose();
}

bool Renderer::visibleOnScreen(int x, int y){
    return (x >= 0 && x <= windowWidth && y >= 0 && y <= windowHeight);
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
    *optimalTimeLocalUpdate /= 2;
}

void Renderer::decreaseSimulationSpeed(){
    *optimalTimeLocalUpdate *= 2;
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
    cameraPosition = cameraDirection * -5 * centreObject->getRadius();
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
    cameraPosition = cameraDirection * -5 * centreObject->getRadius();
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
    cameraPosition = cameraDirection * -5 * centreObject->getRadius();
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
    cameraPosition = cameraDirection * -5 * centreObject->getRadius();
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

    cameraPosition = cameraDirection * -5 * centreObject->getRadius();
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
    // Still quite inefficient to lock for every single entry
    objectsOnScreenLock.lock();
    for(DrawObject *drawObject: *drawObjects){
        objectsOnScreen.push_back(drawObject);
    }
    objectsOnScreenLock.unlock();
}

void Renderer::addDotsOnScreen(std::vector<DrawObject*> *drawObjects){
    // Still quite inefficient to lock for every single entry
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
    printf("Adjusted rendererThreadCount to %d\n", rendererThreadCount);
}

int Renderer::getWindowWidth(){
    return windowWidth;
}

int Renderer::getWindowHeight(){
    return windowHeight;
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

u_int8_t Renderer::getRendererThreadCount(){
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

void calculateCloseObjectTrianglesMultiThread(Renderer *renderer, StellarObject *object, std::vector<RenderTriangle*> *triangles, DrawObject *drawObject, int start, int amount, std::mutex *drawObjectLock){
    std::vector<DrawObject*> drawObjectsToAdd;
    // printf("Arrived in multithread function with start: %d, amount: %d\n", start, amount);
    // struct timespec prevTime;
	// struct timespec currTime;
    // clock_gettime(CLOCK_MONOTONIC, &prevTime);
    for(int i=start;i<start+amount;i++){
        RenderTriangle* triangle = triangles->at(i);
        // ----------------------------------------------------- TODO -----------------------------------------------------
        // Check here if normal vector is pointing in the same distance as the camera, then ignore the triangle

        PositionVector cameraPosition = renderer->getCameraPosition();
        Matrix3d inverseTransformationMatrixCameraBasis = renderer->getInverseTransformationMatrixCameraBasis();
        PositionVector absoluteCameraPosition = renderer->getReferenceObject()->getPositionAtPointInTime() + cameraPosition;

        PositionVector distanceMidPoint = triangle->getMidPoint() + object->getPositionAtPointInTime() - absoluteCameraPosition;
        PositionVector distanceMidPointNewBasis = inverseTransformationMatrixCameraBasis * distanceMidPoint;
        PositionVector distanceP1 = triangle->getPoint1() + object->getPositionAtPointInTime() - absoluteCameraPosition;
        PositionVector distanceP1NewBasis = inverseTransformationMatrixCameraBasis * distanceP1;
        PositionVector distanceP2 = triangle->getPoint2() + object->getPositionAtPointInTime() - absoluteCameraPosition;
        PositionVector distanceP2NewBasis = inverseTransformationMatrixCameraBasis * distanceP2;
        PositionVector distanceP3 = triangle->getPoint3() + object->getPositionAtPointInTime() - absoluteCameraPosition;
        PositionVector distanceP3NewBasis = inverseTransformationMatrixCameraBasis * distanceP3;
        
        if(distanceMidPointNewBasis.getZ()<0) {
            // printf("Found triangle with midpoint behind the camera\n");
            // delete triangle;
            continue;
        }

        // ------------------------------------------------------------------- TODO -------------------------------------------------------------------
        // Make additional checks to rule out anomalies
        // Cut down lines going behind the camera

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

        int colour = object->getColour();
        long double dotProductNormalVector = 1;

        if(object->getType() != GALACTIC_CORE && object->getType() != STAR){
            colour = object->getHomeSystem()->getChildren()->at(0)->getColour();
            dotProductNormalVector = std::max(0.0L, (object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - (triangle->getMidPoint() + object->getPositionAtPointInTime())).normalise().dotProduct(triangle->getNormalVector().normalise()));
            
            // No go through children and parents, to check if something is in between the light source (currently just the first star) and the triangle
            if(object->getParent()->getType() != GALACTIC_CORE && object->getParent()->getType() != STAR){
                if(((object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - object->getPositionAtPointInTime()).dotProduct(object->getParent()->getPositionAtPointInTime() - object->getPositionAtPointInTime())) > 0){
                    Plane parentPlane = Plane(object->getParent()->getPositionAtPointInTime(), object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - (triangle->getMidPoint() + object->getPositionAtPointInTime()));
                    PositionVector intersection = parentPlane.findIntersectionWithLine(object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime(), (triangle->getMidPoint() + object->getPositionAtPointInTime()));
                    long double distanceIntersectionParent = (object->getParent()->getPositionAtPointInTime() - intersection).getLength(); 
                    if(distanceIntersectionParent <= 0.5 * object->getParent()->getRadius()){
                        dotProductNormalVector = 0;
                    }
                    else if(distanceIntersectionParent <= object->getParent()->getRadius()){
                        dotProductNormalVector *= ((distanceIntersectionParent/object->getParent()->getRadius()) - 0.5) * 2;
                    }
                }
            }
            for(StellarObject *child: *(object->getChildren())){
                // If the child is behind the object, we ignore it
                if(((object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - object->getPositionAtPointInTime()).dotProduct(child->getPositionAtPointInTime() - object->getPositionAtPointInTime())) <= 0){
                    continue;
                }
                Plane childPlane = Plane(child->getPositionAtPointInTime(), object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime() - (triangle->getMidPoint() + object->getPositionAtPointInTime()));
                PositionVector intersection = childPlane.findIntersectionWithLine(object->getHomeSystem()->getChildren()->at(0)->getPositionAtPointInTime(), (triangle->getMidPoint() + object->getPositionAtPointInTime()));
                long double distanceIntersectionChild = (child->getPositionAtPointInTime() - intersection).getLength(); 
                
                // printf("Checking if %s is in front of %s. Relative distance of intersection is %Lf\n", child->getName(), object->getName(), distanceIntersectionChild/child->getRadius());
                if(distanceIntersectionChild <= 0.5 * child->getRadius()){
                    dotProductNormalVector = 0;
                }
                else if(distanceIntersectionChild <= child->getRadius()){
                    dotProductNormalVector *= ((distanceIntersectionChild/child->getRadius()) - 0.5) * 2;
                }
            }
        }
        else{
            // We slightly alter colour if the normal vector of the triangles point into the wrong direction from
            dotProductNormalVector = (std::max(0.0L, (absoluteCameraPosition - (triangle->getMidPoint() + object->getPositionAtPointInTime())).normalise().dotProduct(triangle->getNormalVector().normalise())) + 1) /2;
        }
            
        int red = colour >> 16;
        int green = (colour >> 8) & (0b11111111);
        int blue = colour & (0b11111111);
        // Make sure that the colour is at least the background colour
        int backgroundRed = BACKGROUND_COL >> 16;
        int backgroundGreen = (BACKGROUND_COL >> 8) & (0b11111111);
        int backgroundBlue = BACKGROUND_COL & (0b11111111);
        // printf("Old colour: %d, rgb: %d, %d, %d. Background rgb: %d, %d, %d\n", colour, red, green, blue, backgroundRed, backgroundGreen, backgroundBlue);
        colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*dotProductNormalVector)) << 16)
        + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*dotProductNormalVector)) << 8)
        + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*dotProductNormalVector));
        // printf("New colour: %d, new rgb: %d, %d, %d, dotProductNormalVector: %Lf\n\n", red, green, blue, colour, dotProductNormalVector);

        // printf("Adding triangle (%d, %d), (%d, %d), (%d, %d)\n", xP1, yP1, xP2, yP2, xP3, yP3);
        drawObjectsToAdd.push_back(new DrawObject(colour, xP1, yP1, xP2, yP2, xP3, yP3, distanceMidPoint.getLength(), TRIANGLE));
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

void getRenderTrianglesMultiThread(StellarObjectRenderFace *face, std::vector<RenderTriangle*> *triangles, PositionVector absoluteCameraPosition, std::mutex *trianglesLock){
    std::vector<RenderTriangle*> trianglesToAdd;
    face->getRenderTriangles(&trianglesToAdd, absoluteCameraPosition);
    trianglesLock->lock();
    for(RenderTriangle *renderTriangle: trianglesToAdd){
        triangles->push_back(renderTriangle);
    }
    trianglesLock->unlock();
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