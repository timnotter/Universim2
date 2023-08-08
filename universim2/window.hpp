#ifndef WINDOW_HPP
#define WINDOW_HPP

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
class Point2d;
                                                                    // TODO --------------- Changed all variable names to be clearer!!!! -------------------
class MyWindow{
private:
    // Used for X11 on Linux
    void *display;
    void *window;
    int screen;
    void *event;
    Renderer *renderer;
    void *gc;
    void *backBuffer;
    // Used to store information about the window
    void *rootWindow;
    unsigned int windowWidth;
    unsigned int windowHeight;
    unsigned int borderWidth;
    unsigned int depth;
    int tempX;
    int tempY;

public:
    MyWindow();
    
    void setRenderer(Renderer *renderer);

    int getWindowWidth();
    int getWindowHeight();

    // These functions are platform dependent
    void drawBackground(int colour);
    void endDrawing();

    int drawPoint(unsigned int col, int x, int y);
    int drawLine(unsigned int col, int x1, int y1, int x2, int y2);
    int drawRect(unsigned int col, int x, int y, int width, int height);
    int drawCircle(unsigned int col, int x, int y, int diam);
    int drawString(unsigned int col, int x, int y, const char *stringToBe);
    int drawTriangle(unsigned int col, int x1, int y1, int x2, int y2, int x3, int y3);
    int drawPolygon(unsigned int col, short count, Point2d *points, bool checks = false);

    void handleEvents(bool &running, bool &isPaused);
    void handleEvent(void *eventptr, bool &running, bool &isPaused);
    void closeWindow();

    // These functions should be platform independent
    Point2d calculateEdgePointWithNoneVisible(int x1, int y1, int x2, int y2);
    Point2d calculateEdgePointWithOneVisible(int x1, int y1, int x2, int y2);
    int drawTriangleOneNotVisible(unsigned int col, Point2d *points, short indexNotVisible);
    int drawTriangleTwoNotVisible(unsigned int col, Point2d *points, short indexVisible);
    int drawTriangleAllNotVisible(unsigned int col, Point2d *points);

    bool visibleOnScreen(int x, int y);
};

#endif