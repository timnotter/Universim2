#include <iostream>
#include "window.hpp"
#include "renderer.hpp"
#include "constants.hpp"

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

    // Advanced window with alpha value - does not work as intended
    // XVisualInfo visualInfo;
    // XMatchVisualInfo(display, DefaultScreen(display), 32, TrueColor, &visualInfo);
    // XSetWindowAttributes windowAttributes;
    // windowAttributes.colormap = XCreateColormap(display, DefaultRootWindow(display), visualInfo.visual, AllocNone);
    // windowAttributes.border_pixel = 0;
    // windowAttributes.background_pixel = 0xFF222222;
    // window = XCreateWindow(display, DefaultRootWindow(display), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 1, visualInfo.depth, InputOutput, visualInfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &windowAttributes);
    
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

int MyWindow::getScreen(){
    return screen;
}
Window *MyWindow::getWindow(){
    return &window;
}
Display *MyWindow::getDisplay(){
    return display;
}
XEvent *MyWindow::getEvent(){
    return &event;
}
GC *MyWindow::getGC(){
    return &gc;
}
XdbeBackBuffer *MyWindow::getBackBuffer(){
    return &backBuffer;
}