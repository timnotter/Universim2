#include <iostream>
#include "window.hpp"
#include "renderer.hpp"
#include "constants.hpp"
#include "point2d.hpp"
#ifdef _WIN32
void MyWindow::drawBackground(int colour);
void MyWindow::endDrawing();

int MyWindow::drawPoint(unsigned int col, int x, int y);
int MyWindow::drawLine(unsigned int col, int x1, int y1, int x2, int y2);
int MyWindow::drawRect(unsigned int col, int x, int y, int width, int height);
int MyWindow::drawCircle(unsigned int col, int x, int y, int diam);
int MyWindow::drawString(unsigned int col, int x, int y, const char *stringToBe);
int MyWindow::drawTriangle(unsigned int col, int x1, int y1, int x2, int y2, int x3, int y3);
int MyWindow::drawPolygon(unsigned int col, short count, Point2d *points, bool checks);

bool MyWindow::visibleOnScreen(int x, int y);
#endif

// #ifdef __unix
#ifndef _WIN32
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>

MyWindow::MyWindow(){
    display = XOpenDisplay(NULL);
    if (display==NULL){
        throw std::runtime_error("Unable to open display");
    }
    screen = DefaultScreen(display);
    // Simple window with no alpha value
    window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 1, BlackPixel(display, screen), BlackPixel(display, screen));

    int major_version_return, minor_version_return;
    if(!XdbeQueryExtension(display, &major_version_return, &minor_version_return)){
        printf("DBE Problem\n");
    }

    XMapWindow(display, window);
    backBuffer = XdbeAllocateBackBufferName(display, window, XdbeBackground);
    XSelectInput(display, window, KeyPressMask | ExposureMask);
    gc = XCreateGC(display, window, 0, 0);
}

void MyWindow::handleEvent(XEvent &event, bool &running, bool &isPaused){
    if(event.type == Expose){
        // std::cout << "Expose event\n";
        renderer->draw();
    }

    else if(event.type == KeyPress){
        // printf("Key pressed\n");
        switch(event.xkey.keycode){
            case KEY_ESCAPE: running = false; /*printf("Escaped program\n");*/ break;
            case KEY_SPACE: isPaused = !isPaused; /*isPaused ? printf("Paused\n") : printf("Unpaused\n");*/ break;
            // Camera movements
            case KEY_K: renderer->moveCamera(FORWARDS, Y_AXIS); break;
            case KEY_I: renderer->moveCamera(BACKWARDS, Y_AXIS); break;
            case KEY_L: renderer->moveCamera(FORWARDS, X_AXIS); break;
            case KEY_J: renderer->moveCamera(BACKWARDS, X_AXIS); break;
            case KEY_O: renderer->moveCamera(FORWARDS, Z_AXIS); break;
            case KEY_P: renderer->moveCamera(BACKWARDS, Z_AXIS); break;
            case KEY_OE: renderer->decreaseCameraMoveAmount(); break;
            case KEY_AE: renderer->increaseCameraMoveAmount(); break;
            // Camera rotations
            case KEY_W: renderer->rotateCamera(1.0/180*PI, X_AXIS); break;
            case KEY_S: renderer->rotateCamera(-1.0/180*PI, X_AXIS); break;
            case KEY_D: renderer->rotateCamera(1.0/180*PI, Y_AXIS); break;
            case KEY_A: renderer->rotateCamera(-1.0/180*PI, Y_AXIS); break;
            case KEY_Q: renderer->rotateCamera(1.0/180*PI, Z_AXIS); break;
            case KEY_E: renderer->rotateCamera(-1.0/180*PI, Z_AXIS); break;
            case KEY_R: renderer->resetCameraOrientation(); break;
            // Focus changes
            case KEY_1: renderer->centreParent(); break;
            case KEY_2: renderer->centreChild(); break;
            case KEY_3: renderer->centrePrevious(); break;
            case KEY_4: renderer->centreNext(); break;
            case KEY_5: renderer->centrePreviousStarSystem(); break;
            case KEY_6: renderer->centreNextStarSystem(); break;
            case KEY_F: renderer->toggleCentre(); break;
            // Speed changes
            case KEY_PG_UP: renderer->increaseSimulationSpeed(); break;
            case KEY_PG_DOWN: renderer->decreaseSimulationSpeed(); break;

            /*default: printf("Key number %d was pressed\n", event.xkey.keycode);*/
        }
    }
    return;
}

void MyWindow::closeWindow(){
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

void MyWindow::setRenderer(Renderer *renderer){
    this->renderer = renderer;
}

void MyWindow::drawBackground(int colour){
    XGetGeometry(display, window, &rootWindow, &tempX, &tempY, &windowWidth, &windowHeight, &borderWidth, &depth);
    XSetWindowBackground(display, window, 0x111111);
}

void MyWindow::endDrawing(){
    XdbeSwapInfo swap_info = {window, 1};
    XdbeSwapBuffers(display, &swap_info, 1);
}

int MyWindow::drawPoint(unsigned int col, int x, int y){
    XSetForeground(display, gc, col);
    XDrawPoint(display, backBuffer, gc, x, y);
    return 0;
}
int MyWindow::drawLine(unsigned int col, int x1, int y1, int x2, int y2){
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
    XSetForeground(display, gc, col);
    XDrawLine(display, backBuffer, gc, x1, y1, x2, y2);
    return 0;
}

int MyWindow::drawRect(unsigned int col, int x, int y, int width, int height){
    XSetForeground(display, gc, col);
    XFillRectangle(display, backBuffer, gc, x, y, width, height);
    return 0;
}

int MyWindow::drawCircle(unsigned int col, int x, int y, int diam){
    XSetForeground(display, gc, col);
    XDrawArc(display, backBuffer, gc, x-diam/2, y-diam/2, diam, diam, 0, 360 * 64);
    XFillArc(display, backBuffer, gc, x-diam/2, y-diam/2, diam, diam, 0, 360 * 64);
    return 0;
}

int MyWindow::drawString(unsigned int col, int x, int y, const char *stringToBe){
    XSetForeground(display, gc, col);
    XDrawString(display, backBuffer, gc, x, y, stringToBe, strlen(stringToBe));
    return 0;
}

int MyWindow::drawTriangle(unsigned int col, int x1, int y1, int x2, int y2, int x3, int y3){
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

    XSetForeground(display, gc, col);
    XFillPolygon(display, backBuffer, gc, xPoints, 3, Convex, CoordModeOrigin);
    // printf("Drawing triangle in renderer (%d, %d), (%d, %d), (%d, %d)\n", x1, y1, x2, y2, x3, y3);
    // XDrawPolygon(myWindow->getDisplay(), *(myWindow->getBackBuffer()), *(myWindow->getGC()), points, 3, Convex, CoordModeOrigin);
    // drawLine(BLACK_COL, x1, y1, x2, y2);
    // drawLine(BLACK_COL, x1, y1, x3, y3);
    // drawLine(BLACK_COL, x3, y3, x2, y2);
    return 0;
}

int MyWindow::drawPolygon(unsigned int col, short count, Point2d *points, bool checks){
    // ------------------------------------------------------------------- TODO -------------------------------------------------------------------
    // If the polygon does not come from the drawTriangle function, we need some checks to catch anomalies
    if(checks){

    }

    XPoint xPoints[count];
    for(int i=0;i<count;i++){
        xPoints[i].x = points[i].getX();
        xPoints[i].y = points[i].getY();
    }


    XSetForeground(display, gc, col);
    XFillPolygon(display, backBuffer, gc, xPoints, count, Convex, CoordModeOrigin);
    // for(int i=0;i<count;i++){
    //     // printf("Trying to draw line from (%d, %d) to (%d, %d)\n", points[i].x, points[i].y, points[(i+1)%count].x, points[(i+1)%count].y);
    //     drawLine(RED_COL, points[i].getX(), points[i].getY(), points[(i+1)%count].getX(), points[(i+1)%count].getY());
    // }
    return 0;
}

bool MyWindow::visibleOnScreen(int x, int y){
    // Executing drawBackground in the start of the draw phase ensures that windowWidth and windowHeight are up to date
    return (x >= 0 && x <= windowWidth && y >= 0 && y <= windowHeight);
}
#endif

Point2d MyWindow::calculateEdgePointWithOneVisible(int x1, int y1, int x2, int y2){
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

Point2d MyWindow::calculateEdgePointWithNoneVisible(int x1, int y1, int x2, int y2){
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

int MyWindow::drawTriangleAllNotVisible(unsigned int col, Point2d *points){
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

int MyWindow::drawTriangleTwoNotVisible(unsigned int col, Point2d *points, short indexVisible){
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

int MyWindow::drawTriangleOneNotVisible(unsigned int col, Point2d *points, short indexNotVisible){
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

int MyWindow::getWindowWidth(){
    return windowWidth;
}
int MyWindow::getWindowHeight(){
    return windowHeight
}