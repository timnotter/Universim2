#ifndef DRAWOBJECT
#define DRAWOBJECT

#include <vector>

#define CIRCLE 0
#define LINE 1
#define RECTANGLE 2
#define TRIANGLE 3
#define POINT 4
#define POLYGON 5
#define PLUS 6

#define GROUP 10

class Renderer;

class DrawObject{
public:
    int colour;
    long int x1;
    long int y1;
    long int x2;
    long int y2;
    long int x3;
    long int y3;
    long int x4;
    long int y4;
    int size;
    double distance;
    short type;

    // This is used for type GROUP, when we store a lot of triangles from the same object (so we don't have to sort them)
    std::vector<DrawObject*> *children;

    DrawObject(double distance, short type);
    DrawObject(int colour, long int x1, long int y1, double distance, short type);
    DrawObject(int colour, long int x1, long int y1, int size, double distance, short type);
    DrawObject(int colour, long int x1, long int y1, long int x2, long int y2, double distance, short type);
    DrawObject(int colour, long int x1, long int y1, long int x2, long int y2, long int x3, long int y3, double distance, short type);
    DrawObject(int colour, long int x1, long int y1, long int x2, long int y2, long int x3, long int y3, long int x4, long int y4, double distance, short type);
    void addDrawObject(DrawObject *drawObject);
    void draw(Renderer *renderer);
};

#endif