#ifndef DRAWOBJECT
#define DRAWOBJECT

#define CIRCLE 0
#define LINE 1
#define RECTANGLE 2
#define TRIANGLE 3
#define POINT 4
#define POLYGON 5
#define PLUS 6

class Renderer;

class DrawObject{
public:
    int colour;
    int x1;
    int y1;
    int x2;
    int y2;
    int x3;
    int y3;
    int x4;
    int y4;
    int size;
    double distance;
    short type;

    DrawObject(int colour, int x1, int y1, double distance, short type);
    DrawObject(int colour, int x1, int y1, int size, double distance, short type);
    DrawObject(int colour, int x1, int y1, int x2, int y2, double distance, short type);
    DrawObject(int colour, int x1, int y1, int x2, int y2, int x3, int y3, double distance, short type);
    DrawObject(int colour, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, double distance, short type);
    void draw(Renderer *renderer);
};

#endif