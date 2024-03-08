#ifndef WINDOW_HPP
#define WINDOW_HPP

// For initialisational purposes
#define SCREEN_HEIGHT 600
#define SCREEN_WIDTH 800


class Renderer;
class Point2d;
                                                                    // TODO --------------- Changed all variable names to be clearer!!!! -------------------
class MyWindow{
private:
    // Used for X11 on Linux. They are all void*, such that we don't have to forward declare types. These void* can possibly reused by the windows graphics API (To be done)
    void *display;
    void *window;
    int screen;
    void *event;
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
    // These functions are platform dependent
    MyWindow();
    
    void drawBackground(int colour);
    // SHOULD be called after a function finished drawing. It swaps currently displayed and newly drawn picture
    void endDrawing();

    // Checks if visible and draw if it is
    int drawPoint(unsigned int col, int x, int y);
    // Checks if visible and draw if it is
    int drawLine(unsigned int col, int x1, int y1, int x2, int y2);
    // Checks if visible and draw if it is
    int drawRect(unsigned int col, int x, int y, int width, int height);
    // Checks if visible and draw if it is
    int drawCircle(unsigned int col, int x, int y, int diam);
    // Checks if visible and draw if it is
    int drawString(unsigned int col, int x, int y, const char *stringToBe);
    // Checks if visible and draw if it is
    int drawTriangle(unsigned int col, int x1, int y1, int x2, int y2, int x3, int y3);

    // Unconditionally draws polygon. Bool checks can be implemented to do some checks
    int drawPolygon(unsigned int col, short count, Point2d *points, bool checks = false);

    // SHOULD be called after finishing execution. Cleans up object
    void closeWindow();

    int getNumberOfPendingEvents();
    // Takes as input two arraypointers and their size. Writes type of event into eventTypes array, and the keycodes into parameters array, if event was a keypress. 
    // If total number of pending events is smaller than numberOfEvents, it writes -1 into the first entry of the evenTypes array
    void getPendingEvents(int *eventTypes, u_int *parameters, int numberOfEvents);

    // These functions should be platform independent

    int getWindowWidth();
    int getWindowHeight();
    Point2d calculateEdgePointWithNoneVisible(int x1, int y1, int x2, int y2);
    Point2d calculateEdgePointWithOneVisible(int x1, int y1, int x2, int y2);
    int drawTriangleOneNotVisible(unsigned int col, Point2d *points, short indexNotVisible);
    int drawTriangleTwoNotVisible(unsigned int col, Point2d *points, short indexVisible);
    int drawTriangleAllNotVisible(unsigned int col, Point2d *points);

    bool visibleOnScreen(int x, int y);
};

#endif