#ifndef WINDOW_HPP
#define WINDOW_HPP
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xdbe.h>
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
// For initialisational purposes
#define SCREEN_HEIGHT 600
#define SCREEN_WIDTH 800
class Renderer;
                                                                    // TODO --------------- Changed all variable names to be clearer!!!! -------------------
class MyWindow{
private: 
    Display *display;
    Window window;
    int screen;
    XEvent event;
    Renderer *renderer;
    GC gc;
    XdbeBackBuffer backBuffer;

public:
    MyWindow();
    void handleEvent(XEvent &event, bool &running, bool &isPaused);
    void closeWindow();
    
    void setRenderer(Renderer *renderer);

    int getScreen();
    Window *getWindow();
    Display *getDisplay();
    XEvent *getEvent();
    GC *getGC();
    XdbeBackBuffer *getBackBuffer();
};

#endif