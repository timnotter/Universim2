#ifndef WINDOW_HPP
#define WINDOW_HPP

// For initialisational purposes
#define SCREEN_HEIGHT 600
#define SCREEN_WIDTH 800

#define KeyPress 2
#define Expose 12

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
#define KEY_9 18

#ifdef _WIN32
#include <Windows.h>
#include <gdiplus.h>
#endif


class Renderer;
class Point2d;
// TODO --------------- Change all variable names to be clearer!!!! -------------------
class MyWindow{
private:
    unsigned int windowWidth;
    unsigned int windowHeight;

    #ifdef _WIN32
        // Windows-specific fields (GDI+)
        HWND hwnd;
        HDC hdc;
        HDC memDC;
        HBITMAP hBitmap;
        Gdiplus::Graphics* graphics;
        ULONG_PTR gdiplusToken;
    #else
        // Linux-specific fields (X11)
        void* display;
        void* window;
        int screen;
        void* event;
        void* gc;
        void* backBuffer;
        void* rootWindow;
        unsigned int borderWidth;
        unsigned int depth;
        int tempX;
        int tempY;
    #endif

public:
    // These functions are platform dependent
    MyWindow();
    ~MyWindow();
    
    void drawBackground(int colour);
    // SHOULD be called after a function finished drawing. It swaps currently displayed and newly drawn picture
    void endDrawing();

    // Draw point
    int drawPoint(unsigned int col, int x, int y);
    // Draws line
    int drawLine(unsigned int col, int x1, int y1, int x2, int y2);
    // Draws rect
    int drawRect(unsigned int col, int x, int y, int width, int height);
    // Draws circle
    int drawCircle(unsigned int col, int x, int y, int diam);
    // Draws string
    int drawString(unsigned int col, int x, int y, const char *stringToBe);
    // Draws triangle
    int drawTriangle(unsigned int col, int x1, int y1, int x2, int y2, int x3, int y3);
    // Draws polygon
    int drawPolygon(unsigned int col, short count, Point2d *points);

    int getNumberOfPendingEvents();
    // Takes as input two arraypointers and their size. Writes type of event into eventTypes array, and the keycodes into parameters array, if event was a keypress. 
    // If total number of pending events is smaller than numberOfEvents, it writes -1 into the first entry of the evenTypes array
    void getPendingEvents(int *eventTypes, u_int *parameters, int numberOfEvents);

    // These functions should be platform independent

    int getWindowWidth();
    int getWindowHeight();
    #ifdef _WIN32
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    #endif
};

#endif
