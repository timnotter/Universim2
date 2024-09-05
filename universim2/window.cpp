#include <iostream>
#include "window.hpp"
#include "renderer.hpp"
#include "constants.hpp"
#include "point2d.hpp"
#ifdef _WIN32
MyWindow::MyWindow(){}
void MyWindow::drawBackground(int colour){}
void MyWindow::endDrawing(){}

int MyWindow::drawPoint(unsigned int col, int x, int y){return 1;}
int MyWindow::drawLine(unsigned int col, int x1, int y1, int x2, int y2){return 1;}
int MyWindow::drawRect(unsigned int col, int x, int y, int width, int height){return 1;}
int MyWindow::drawCircle(unsigned int col, int x, int y, int diam){return 1;}
int MyWindow::drawString(unsigned int col, int x, int y, const char *stringToBe){return 1;}
int MyWindow::drawTriangle(unsigned int col, int x1, int y1, int x2, int y2, int x3, int y3){return 1;}
int MyWindow::drawPolygon(unsigned int col, short count, Point2d *points, bool checks){return 1;}
void MyWindow::getPendingEvents(int *eventTypes, u_int *parameters, int numberOfEvents){}
bool MyWindow::visibleOnScreen(int x, int y){return false;}

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
    window = new Window;
    Window windowPointer = 
    *(static_cast<Window *>(window)) = XCreateSimpleWindow((static_cast<Display *>(display)), RootWindow((static_cast<Display *>(display)), screen), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 1, BlackPixel((static_cast<Display *>(display)), screen), BlackPixel((static_cast<Display *>(display)), screen));

    event = new XEvent;
    gc = new GC;

    int major_version_return, minor_version_return;
    if(!XdbeQueryExtension((static_cast<Display *>(display)), &major_version_return, &minor_version_return)){
        printf("DBE Problem\n");
    }
    

    XMapWindow((static_cast<Display *>(display)), *(static_cast<Window *>(window)));
    backBuffer = new XdbeBackBuffer;
    *(static_cast<XdbeBackBuffer *>(backBuffer)) = XdbeAllocateBackBufferName((static_cast<Display *>(display)),*(static_cast<Window *>(window)), XdbeBackground);
    XSelectInput((static_cast<Display *>(display)),*(static_cast<Window *>(window)), KeyPressMask | ExposureMask);
    *(static_cast<GC *>(gc)) = XCreateGC((static_cast<Display *>(display)),*(static_cast<Window *>(window)), 0, 0);

    rootWindow = new Window;
}

MyWindow::~MyWindow(){
    XFreeGC((static_cast<Display *>(display)), *(static_cast<GC *>(gc)));
    XDestroyWindow((static_cast<Display *>(display)),*(static_cast<Window *>(window)));
    XCloseDisplay((static_cast<Display *>(display)));
    delete (static_cast<Window *>(window));
    delete (static_cast<XEvent *>(event));
    delete (static_cast<GC *>(gc));
    delete (static_cast<XdbeBackBuffer *>(backBuffer));
    delete (static_cast<Window *>(rootWindow));
    // delete (static_cast<Display *>(display));                    // Possible memory leak, but throws error if enabled
    // delete event;
    // delete gc;
    // delete backBuffer;
    // delete rootWindow;
    // delete display;
}

void MyWindow::drawBackground(int colour){
    XGetGeometry((static_cast<Display *>(display)),*(static_cast<Window *>(window)), (static_cast<Window *>(rootWindow)), &tempX, &tempY, &windowWidth, &windowHeight, &borderWidth, &depth);
    XSetWindowBackground((static_cast<Display *>(display)),*(static_cast<Window *>(window)), 0x111111);
}

void MyWindow::endDrawing(){
    XdbeSwapInfo swap_info = {*(static_cast<Window *>(window)), 1};
    XdbeSwapBuffers((static_cast<Display *>(display)), &swap_info, 1);
}

int MyWindow::drawPoint(unsigned int col, int x, int y){
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XDrawPoint((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), x, y);
    return 0;
}
int MyWindow::drawLine(unsigned int col, int x1, int y1, int x2, int y2){
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XDrawLine((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), x1, y1, x2, y2);
    return 0;
}

int MyWindow::drawRect(unsigned int col, int x, int y, int width, int height){
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XFillRectangle((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), x, y, width, height);
    return 0;
}

int MyWindow::drawCircle(unsigned int col, int x, int y, int diam){
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XDrawArc((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), x-diam/2, y-diam/2, diam, diam, 0, 360 * 64);
    XFillArc((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), x-diam/2, y-diam/2, diam, diam, 0, 360 * 64);
    return 0;
}

int MyWindow::drawString(unsigned int col, int x, int y, const char *stringToBe){
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XDrawString((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), x, y, stringToBe, strlen(stringToBe));
    return 0;
}

int MyWindow::drawTriangle(unsigned int col, int x1, int y1, int x2, int y2, int x3, int y3){
    // printf("Drawing triangle with points: (%d, %d), (%d, %d), (%d, %d)\n\n", x1, y1, x2, y2, x3, y3);
    XPoint xPoints[3];
    xPoints[0].x = x1;
    xPoints[0].y = y1;
    xPoints[1].x = x2;
    xPoints[1].y = y2;
    xPoints[2].x = x3;
    xPoints[2].y = y3;
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XFillPolygon((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), xPoints, 3, Convex, CoordModeOrigin);
    return 0;
}

int MyWindow::drawPolygon(unsigned int col, short count, Point2d *points){
    // if(count < 3) printf("Tried to draw polygin with %d points", count);
    // printf("Drawing polygon with points: ");
    // for(int i=0;i<count;i++){
    //     printf("(%d, %d), ", points[i].getX(), points[i].getY());
    // }
    // printf("\n\n");
    XPoint xPoints[count];
    for(int i=0;i<count;i++){
        xPoints[i].x = points[i].getX();
        xPoints[i].y = points[i].getY();
    }
    XSetForeground((static_cast<Display *>(display)), *(static_cast<GC *>(gc)), col);
    XFillPolygon((static_cast<Display *>(display)), *(static_cast<XdbeBackBuffer *>(backBuffer)), *(static_cast<GC *>(gc)), xPoints, count, Convex, CoordModeOrigin);
    // for(int i=0;i<count;i++){
    //     // printf("Trying to draw line from (%d, %d) to (%d, %d)\n", points[i].x, points[i].y, points[(i+1)%count].x, points[(i+1)%count].y);
    //     drawLine(RED_COL, points[i].getX(), points[i].getY(), points[(i+1)%count].getX(), points[(i+1)%count].getY());
    // }
    return 0;
}
#endif

int MyWindow::getWindowWidth(){
    return windowWidth;
}
int MyWindow::getWindowHeight(){
    return windowHeight;
}
int MyWindow::getNumberOfPendingEvents(){
    return XEventsQueued((static_cast<Display *>(display)), QueuedAlready);
}
void MyWindow::getPendingEvents(int *eventTypes, u_int *parameters, int numberOfEvents){
    if (XEventsQueued((static_cast<Display *>(display)), QueuedAlready) < numberOfEvents){
        eventTypes[0] = -1;
        return;
    }

    for(int i=0;i<numberOfEvents;i++){
        XNextEvent((static_cast<Display *>(display)), static_cast<XEvent *>(event));
        eventTypes[i] = static_cast<XEvent *>(event)->type;
        if (eventTypes[i] == KeyPress){
            parameters[i] = static_cast<XEvent *>(event)->xkey.keycode;
        }
    }
}