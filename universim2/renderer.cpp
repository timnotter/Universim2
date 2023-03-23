#include <X11/Xlib.h>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <thread>
#include <unistd.h>
#include "renderer.hpp"
#include "star.hpp"


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
    // cameraPlainVector1 = PositionVector(1, 0, 0);
    // cameraPlainVector2 = PositionVector(0, 1, 0);
    cameraPosition = PositionVector(0, 10 * astronomicalUnit, 0);
    cameraDirection = PositionVector(0, -1, 0);
    cameraPlainVector1 = PositionVector(1, 0, 0);
    cameraPlainVector2 = PositionVector(0, 0, 1);
    centreObject = NULL;
    lastCenterObject = NULL;
    referenceObject = NULL;
    lastReferenceObject = NULL;

    transformationMatrixCameraBasis = Matrix3d(cameraPlainVector1, cameraPlainVector2, cameraDirection);
    inverseTransformationMatrixCameraBasis = transformationMatrixCameraBasis.transpose();
    // printf("Renderer constructor end\n");
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

    // XClearWindow(myWindow->getDisplay(), *(myWindow->getWindow()));          // Not neccessary, probably because of the swap
    XdbeSwapInfo swap_info = {*(myWindow->getWindow()), 1};
    XdbeSwapBuffers(myWindow->getDisplay(), &swap_info, 1);
}

// Needs to be static, because threads can only work on static functions
void static staticDrawObjectMultiThread(int start, int amount, Renderer *renderer, int id){
    // printf("Started thread %d\n", id);
    // printf("Started thread with start = %d, amount = %d\n", start, amount);
    renderer->drawObjectMultiThread(start, amount);
}

void Renderer::drawObjects(){
    // ------------------------------------------------------------ TODO ------------------------------------------------------------
    // This function should be executed by the GPU!!!!! - Use OpenCL
    // struct timespec prevTime;
	// struct timespec currTime;
	// clock_gettime(CLOCK_MONOTONIC, &prevTime);

    int threadNumber = 8;
    int amount = allObjects->size()/threadNumber;
    // printf("Total objects: %ld\n", allObjects->size());
    std::vector<std::thread> threads;
    for(int i=0;i<threadNumber-1;i++){
        // threads.push_back(std::thread (drawObjectMultiThread, i*amount, amount));
        threads.push_back(std::thread (staticDrawObjectMultiThread, i*amount, amount, this, i));
        // printf("Started thread %d\n", i);
    }
    // threads.push_back(std::thread (drawObjectMultiThread, (threadNumber-1)*amount, allObjects->size()-(threadNumber-1)*amount));
    threads.push_back(std::thread (staticDrawObjectMultiThread, (threadNumber-1)*amount, allObjects->size()-(threadNumber-1)*amount, this, threadNumber-1));
    // printf("Started thread %d\n", threadNumber-1);
    // printf("Started all threads\n");
    for(int i=0;i<threadNumber;i++){
        threads.at(i).join();
        // printf("Joined thread %d\n", i);
    }
    // printf("%ld objects on screen\n", objectsOnScreen.size());

	// clock_gettime(CLOCK_MONOTONIC, &currTime);
    // printf("Calculating Image took: %ld mics\n", ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000));

    // clock_gettime(CLOCK_MONOTONIC, &prevTime);
    drawDrawObjects();
    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // printf("Sorting and drawing Image took: %ld mics\n", ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000));
}

// ------------------------------------------------------------ TODO FIX PROBLEM ------------------------------------------------------------
// It has to be some kind of overflow, memory leak, memory scarcity. For small number of stars, it doesn't happen

void Renderer::drawObjectMultiThread(int start, int amount){
    std::vector<DrawObject*> objectsToAddOnScreen;
    std::vector<DrawObject*> dotsToAddOnScreen;
    for(int i=start; i<start + amount;i++){
        if(allObjects->at(i)->getType() == STARSYSTEM) continue;
        // if(i%10==0)
            // printf("Calculating position of %s with number %d\n", allObjects->at(i)->getName(), i);
        calculateObjectPosition(allObjects->at(i), &objectsToAddOnScreen, &dotsToAddOnScreen);
    }
    addObjectsOnScreen(&objectsToAddOnScreen);
    addDotsOnScreen(&dotsToAddOnScreen);
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
    drawString(BLACK_COL, windowWidth-75, 30, date->toString(true));

}

void Renderer::drawDrawObjects(){
    // ------------------------------------------------------- TODO -------------------------------------------------------
    // Why do I even lock here? Isn't that unneccessary?
    sortObjectsOnScreen();
    currentlyUpdatingOrDrawingLock->lock();
    for(DrawObject *drawObject: dotsOnScreen){
        drawObject->draw(this);
    }
    for(DrawObject *drawObject: objectsOnScreen){
        drawObject->draw(this);
    }
    currentlyUpdatingOrDrawingLock->unlock();

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

int Renderer::drawPoint(unsigned int col, int x, int y){
    XSetForeground(myWindow->getDisplay(), *(myWindow->getGC()), col);
    XDrawPoint(myWindow->getDisplay(), *(myWindow->getBackBuffer()), *(myWindow->getGC()), x, y);
    return 0;
}

int Renderer::drawLine(unsigned int col, int x1, int y1, int x2, int y2){
    // Make sure no fancy visual bugs get displayed    
    int testWindowWidth = windowWidth;                      // Fuck knows why this is neccessary
    int testWindowHeight = windowHeight;                    // Fuck knows why this is neccessary
    // printf("Would draw line from %d, %d to %d, %d\n", x1, y1, x2, y2);
    while(x1>2*testWindowWidth || x1<(-1 * testWindowWidth) || y1>2*testWindowHeight || y1<(-1 * testWindowHeight)){
        x1 = (x1 - testWindowWidth/2) / 2 + testWindowWidth/2;
        y1 = (y1 - testWindowHeight/2) / 2 + testWindowHeight/2;
    }
    while(x2>2*testWindowWidth || x2<(-1 * testWindowWidth) || y2>2*testWindowHeight || y2<(-1 * testWindowHeight)){
        x2 = (x2 - testWindowWidth/2) / 2 + testWindowWidth/2;
        y2 = (y2 - testWindowHeight/2) / 2 + testWindowHeight/2;
    }
    // printf("But now drawing line from %d, %d to %d, %d\n", x1, y1, x2, y2);

    XSetForeground(myWindow->getDisplay(), *(myWindow->getGC()), col);
    XDrawLine(myWindow->getDisplay(), *(myWindow->getBackBuffer()), *(myWindow->getGC()), x1, y1, x2, y2);
    return 0;
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

void Renderer::calculateObjectPosition(StellarObject *object, std::vector<DrawObject*> *objectsToAddOnScreen, std::vector<DrawObject*> *dotsToAddOnScreen){
    // We look at 2d circles in 3d space, no depth!!!!
    
    // struct timespec prevTime;
	// struct timespec currTime;
    // clock_gettime(CLOCK_MONOTONIC, &prevTime);
    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // printf("Time taken: %ld, prevTime: %ld, currTime: %ld\n", (1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec)), (1000000000*(prevTime.tv_sec)+(prevTime.tv_nsec)), (1000000000*(currTime.tv_sec)+(currTime.tv_nsec)));

    // First we calculate the distancevector from camera to object
    // PositionVector distance = object->getPosition() - referenceObject->getPosition() - cameraPosition; // This would be convenient but is comparatively much to slow
    PositionVector distance = PositionVector(object->getPosition().getX()-referenceObject->getPosition().getX()-cameraPosition.getX(), object->getPosition().getY()-referenceObject->getPosition().getY()-cameraPosition.getY(), object->getPosition().getZ()-referenceObject->getPosition().getZ()-cameraPosition.getZ());

    // We then make a basistransformation with the cameraDirection and cameraPlainVectors
    // PositionStandardBasis = TransformationMatrix * PositionNewBasis
    // Because the basis is orthonormal, the inverse is just the transpose
    // -> InverseTransformationMatrix * PositionStandardBasis = PositionNewBasis
    PositionVector distanceNewBasis = inverseTransformationMatrixCameraBasis * distance;
    
    // If the z-coordinate is negative, it is definitely out of sight, namely behind the camera
    if(distanceNewBasis.getZ()<0){
        // printf("Exited calculateObjectPosition\n");
        return;
    }

    // Calculate size in pixels on screen
    double size = object->getRadius() / distance.getLength() * (windowWidth/2);
    if(size < 1.0/10000000000){
        // printf("Exited calculateObjectPosition\n");
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

    // If no part of the object would be on screen or it would be too small, we dont draw it
    if(x - size > windowWidth || x + size < 0 || y - size > windowHeight || y + size < TOPBAR_HEIGHT){
        // printf("Exited calculateObjectPosition\n");
        return;
    }
    // ------------------------------------------------- TODO -------------------------------------------------
    // Fade into white
    int colour = object->getColour();
    if(object->getType() != GALACTIC_CORE && object->getType() != STAR){
        colour = 0xFFFFFF;
    }
    // Used such that the "plus" fades into a point
    int originalColour = colour;
    // if(object->getType() == GALACTIC_CORE) colour = object->getColour();
    // else{
    //     StellarObject *colourGiverObject = object;
    //     while(colourGiverObject->getType() != STAR) colourGiverObject = colourGiverObject->getParent();
    //     colour = (static_cast<Star*>(colourGiverObject))->getColour();
    // }
    // originalColour = colour;

    // ------------------------------------------------- Maybe use apparent brightness for realistic fading away -------------------------------------------------
    bool isPoint = false;
    int red = colour >> 16;
    int green = (colour >> 8) & (0b11111111);
    int blue = colour & (0b11111111);
    // Make sure thatthe colour is at least the background colour
    int backgroundRed = BACKGROUND_COL >> 16;
    int backgroundGreen = (BACKGROUND_COL >> 8) & (0b11111111);
    int backgroundBlue = BACKGROUND_COL & (0b11111111);
    // if(object->getType()==STAR) printf("Before - Colour: %d, r: %d, g: %d, b: %d, size: %f\n", colour, red, green, blue, size);
    if(object->getType() == GALACTIC_CORE || object->getType() == STAR){
        if(size < 1.0/1000){
            isPoint = true;
            // colour = ((int)(red*(1 + std::log(1000*size/2+0.5))) << 16) + ((int)(green*(1 + std::log(1000*size/2+0.5))) << 8) + (int)(blue*(1 + std::log(1000*size/2+0.5)));
            colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*(1 + std::log(1000*size/2+0.0001)/log(100000)))) << 16)
            + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*(1 + std::log(1000*size/2+0.0001)/log(100000)))) << 8)
            + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*(1 + std::log(1000*size/2+0.0001)/log(100000))));
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
        if(size < 1.0/100){
            isPoint = true;
            colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*(1 + std::log(100*size/2+0.5)))) << 16)
            + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*(1 + std::log(100*size/2+0.5)))) << 8)
            + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*(1 + std::log(100*size/2+0.5))));
        }
        else if(size < 1){
            colour = ((int)(backgroundRed + (std::max(0, red-backgroundRed)*(1 + std::log(size/2+0.5)))) << 16)
            + ((int)(backgroundGreen + (std::max(0, green-backgroundGreen)*(1 + std::log(size/2+0.5)))) << 8)
            + (int)(backgroundBlue + (std::max(0, blue-backgroundBlue)*(1 + std::log(size/2+0.5))));
        }
    }
    // printf("Colour and size of %s: %d, %f\n", object->getName(), colour, size);

    DrawObject *drawObject;
    if(isPoint){
        if(visibleOnScreen(x, y)) {
            drawObject = new DrawObject(colour, x, y, distanceNewBasis.getLength(), POINT);
            dotsToAddOnScreen->push_back(drawObject);
            // addDotOnScreen(drawObject);
        }
        // printf("Exited calculateObjectPosition\n");
        return;
    }
    else if(size<1){
        drawObject = new DrawObject(colour, x, y, distanceNewBasis.getLength(), PLUS);
        objectsToAddOnScreen->push_back(drawObject);
        // addObjectOnScreen(drawObject);
        drawObject = new DrawObject(originalColour, x, y, distanceNewBasis.getLength()-1, POINT);
        objectsToAddOnScreen->push_back(drawObject);
        // addObjectOnScreen(drawObject);
        return;
    }
    drawObject = new DrawObject(colour, x, y, size, distanceNewBasis.getLength(), CIRCLE);
    objectsToAddOnScreen->push_back(drawObject);
    // addObjectOnScreen(drawObject);
    
    // Draw reference lines
    if(referenceObject == object && size>3)
        calculateReferenceLinePositions(x, y, size, object);
    // printf("Exited calculateObjectPosition\n");
}

void Renderer::calculateReferenceLinePositions(int x, int y, int size, StellarObject *object){
    // First we find out the positions on screen of the endpoints of the reference lines
    PositionVector point1XAxis = PositionVector(-3 * object->getRadius() + object->getX(), 0 + object->getY(), 0 + object->getZ());
    PositionVector point2XAxis = PositionVector(3 * object->getRadius() + object->getX(), 0 + object->getY(), 0 + object->getZ());
    PositionVector point1ObjectSurfaceXAxis = PositionVector(-object->getRadius() + object->getX(), 0 + object->getY(), 0 + object->getZ());
    PositionVector point2ObjectSurfaceXAxis = PositionVector(object->getRadius() + object->getX(), 0 + object->getY(), 0 + object->getZ());

    PositionVector point1YAxis = PositionVector(0 + object->getX(), -3 * object->getRadius() + object->getY(), 0 + object->getZ());
    PositionVector point2YAxis = PositionVector(0 + object->getX(), 3 * object->getRadius() + object->getY(), 0 + object->getZ());
    PositionVector point1ObjectSurfaceYAxis = PositionVector(0 + object->getX(), -object->getRadius() + object->getY(), 0 + object->getZ());
    PositionVector point2ObjectSurfaceYAxis = PositionVector(0 + object->getX(), object->getRadius() + object->getY(), 0 + object->getZ());

    PositionVector point1ZAxis = PositionVector(0 + object->getX(), 0 + object->getY(), -3 * object->getRadius() + object->getZ());
    PositionVector point2ZAxis = PositionVector(0 + object->getX(), 0 + object->getY(), 3 * object->getRadius() + object->getZ());
    PositionVector point1ObjectSurfaceZAxis = PositionVector(0 + object->getX(), 0 + object->getY(), -object->getRadius() + object->getZ());
    PositionVector point2ObjectSurfaceZAxis = PositionVector(0 + object->getX(), 0 + object->getY(), object->getRadius() + object->getZ());

    // Calculate distance vectors in respect to the basis vectors of the camera 
    PositionVector absoluteCameraPosition = cameraPosition + referenceObject->getPosition();

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
        // This more expensive computation is used to reduce numerical instability
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


void Renderer::rotateCamera(double angle, short axis){
    // The three vectors cameraDirection, cameraPlainVector1 and cameraPlainVector2 are an orthonormal basis. We therefore look at them as basisvectors for the rotation
    // Thus we can rotate them around more easily and in the end

    // The equation is like this: VectorStandardBasis = TransformationMatrix * VectorNewBasis, 
    // where TransformationMatrix are the vectors (cameraPlainVector1, cameraPlainVector2, cameraDirection) 

    // printf("Rotating camera by %f on axis %d\n", angle, axis);

    PositionVector tempCameraPlainVector1 = PositionVector();
    PositionVector tempCameraPlainVector2 = PositionVector();
    PositionVector tempCameraDirection = PositionVector();

    if(centreObject != NULL){
        angle *= -1;
    }

    switch(axis){
        case X_AXIS: 
            tempCameraPlainVector1.setX(1);
            tempCameraPlainVector1.setY(0);
            tempCameraPlainVector1.setZ(0);

            tempCameraPlainVector2.setX(0);
            tempCameraPlainVector2.setY(std::cos(angle));
            tempCameraPlainVector2.setZ(std::sin(angle));

            tempCameraDirection.setX(0);
            tempCameraDirection.setY(-std::sin(angle));
            tempCameraDirection.setZ(std::cos(angle));
            break;
        case Y_AXIS:
            tempCameraPlainVector1.setX(std::cos(angle));
            tempCameraPlainVector1.setY(0);
            tempCameraPlainVector1.setZ(-std::sin(angle));

            tempCameraPlainVector2.setX(0);
            tempCameraPlainVector2.setY(1);
            tempCameraPlainVector2.setZ(0);

            tempCameraDirection.setX(std::sin(angle));
            tempCameraDirection.setY(0);
            tempCameraDirection.setZ(std::cos(angle));
            break;
        case Z_AXIS:
            tempCameraPlainVector1.setX(std::cos(angle));
            tempCameraPlainVector1.setY(std::sin(angle));
            tempCameraPlainVector1.setZ(0);

            tempCameraPlainVector2.setX(-std::sin(angle));
            tempCameraPlainVector2.setY(std::cos(angle));
            tempCameraPlainVector2.setZ(0);

            tempCameraDirection.setX(0);
            tempCameraDirection.setY(0);
            tempCameraDirection.setZ(1);
            break;
    }
    cameraPlainVector1 = transformationMatrixCameraBasis * tempCameraPlainVector1;
    cameraPlainVector2 = transformationMatrixCameraBasis * tempCameraPlainVector2;
    cameraDirection = transformationMatrixCameraBasis * tempCameraDirection;

    // printf("Before - cameraPosition.getLength(): %f\n",  cameraPosition.getLength());
    if(centreObject != NULL){
        cameraPosition = cameraDirection * -1 * cameraPosition.getLength();
    }
    // printf("After - cameraPosition.getLength(): %f\n",  cameraPosition.getLength());

    // Adjust transformationmatrices
    transformationMatrixCameraBasis = Matrix3d(cameraPlainVector1, cameraPlainVector2, cameraDirection);
    inverseTransformationMatrixCameraBasis = transformationMatrixCameraBasis.transpose();
    // transformationMatrixCameraBasis.print();
    // inverseTransformationMatrixCameraBasis.print();
}

void Renderer::moveCamera(short direction, short axis){
    // printf("Moving camera\n");
    if(centreObject == NULL || axis != Z_AXIS){
        switch(axis){
            case X_AXIS:
                cameraPosition.setX(cameraPosition.getX() + direction * cameraMoveAmount * cameraPlainVector1.getX());
                cameraPosition.setY(cameraPosition.getY() + direction * cameraMoveAmount * cameraPlainVector1.getY());
                cameraPosition.setZ(cameraPosition.getZ() + direction * cameraMoveAmount * cameraPlainVector1.getZ());
                break;
            case Y_AXIS:
                cameraPosition.setX(cameraPosition.getX() + direction * cameraMoveAmount * cameraPlainVector2.getX());
                cameraPosition.setY(cameraPosition.getY() + direction * cameraMoveAmount * cameraPlainVector2.getY());
                cameraPosition.setZ(cameraPosition.getZ() + direction * cameraMoveAmount * cameraPlainVector2.getZ());
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
        cameraPosition /= 1.5;
        // printf("New cameraPosition: (%f, %f, %f), distance to centre: %fly\n", cameraPosition.getX(),cameraPosition.getY(), cameraPosition.getZ(), cameraPosition.getLength()/lightyear);
        return;
    }
    cameraPosition *= 1.5;
    // printf("New cameraPosition: (%f, %f, %f), distance to centre: %fly\n", cameraPosition.getX(),cameraPosition.getY(), cameraPosition.getZ(), cameraPosition.getLength()/lightyear);
}

void Renderer::resetCameraOrientation(){
    // ------------------------------------------------------------------------ TODO ------------------------------------------------------------------------
    cameraDirection = PositionVector(0, 0, -1);
    cameraPlainVector1 = PositionVector(1, 0, 0);
    cameraPlainVector2 = PositionVector(0, 1, 0);
    
    cameraPosition = cameraDirection * -5 * centreObject->getRadius();

    transformationMatrixCameraBasis = Matrix3d(cameraPlainVector1, cameraPlainVector2, cameraDirection);
    inverseTransformationMatrixCameraBasis = transformationMatrixCameraBasis.transpose();
}

bool Renderer::visibleOnScreen(int x, int y){
    return (x >= 0 && x <= windowWidth && y >= TOPBAR_HEIGHT && y <= windowWidth);
}

void Renderer::increaseCameraMoveAmount(){
    cameraMoveAmount *= 1.5;
    if(cameraMoveAmount < astronomicalUnit/2)
        printf("New cameraMoveAmount: %fm\n", cameraMoveAmount);
    else if(cameraMoveAmount < lightyear/2)
        printf("New cameraMoveAmount: %faU\n", cameraMoveAmount/astronomicalUnit);
    else
        printf("New cameraMoveAmount: %fly\n", cameraMoveAmount/lightyear);
}

void Renderer::decreaseCameraMoveAmount(){
    cameraMoveAmount = std::max(cameraMoveAmount / 1.5, 1.0);
    if(cameraMoveAmount < astronomicalUnit/2)
        printf("New cameraMoveAmount: %fm\n", cameraMoveAmount);
    else if(cameraMoveAmount < lightyear/2)
        printf("New cameraMoveAmount: %faU\n", cameraMoveAmount/astronomicalUnit);
    else
        printf("New cameraMoveAmount: %fly\n", cameraMoveAmount/lightyear);
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
    double distance = (nearest->getPosition() - absolutCameraPosition).getLength();
    double tempDistance;
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

void Renderer::sortObjectsOnScreen(){
    quicksort(0, objectsOnScreen.size());
}

void Renderer::quicksort(int start, int end){
    if(start<end){
        int pivotPosition = quicksortInsert(start, end);
        quicksort(start, pivotPosition);
        quicksort(pivotPosition + 1, end);
    }
}

int Renderer::quicksortInsert(int start, int end){
    int pivot = end-1;

    int insertPosition = start;

    for(int currentPosition = start;currentPosition<end;currentPosition++){
        if((objectsOnScreen[currentPosition])->distance > (objectsOnScreen[pivot])->distance){
            std::swap(objectsOnScreen[currentPosition], objectsOnScreen[insertPosition++]);
        }
    }
    std::swap(objectsOnScreen[pivot], objectsOnScreen[insertPosition]);

    return insertPosition;
}

void Renderer::initialiseReferenceObject(){
    referenceObject = (*galaxies)[0];
}