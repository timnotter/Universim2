#include <stdio.h>
#include "drawObject.hpp"
#include "renderer.hpp"

DrawObject::DrawObject(int colour, int x1, int y1, double distance, short type){
    this->colour = colour;
    this->x1 = x1;
    this->y1 = y1;
    this->distance = distance;
    this->type = type;
}

DrawObject::DrawObject(int colour, int x1, int y1, int size, double distance, short type){
    this->colour = colour;
    this->x1 = x1;
    this->y1 = y1;
    this->size = size;
    this->distance = distance;
    this->type = type;
}

DrawObject::DrawObject(int colour, int x1, int y1, int x2, int y2, double distance, short type){
    this->colour = colour;
    this->x1 = x1;
    this->y1 = y1;
    this->x2 = x2;
    this->y2 = y2;
    this->distance = distance;
    this->type = type;
}

DrawObject::DrawObject(int colour, int x1, int y1, int x2, int y2, int x3, int y3, double distance, short type){
    this->colour = colour;
    this->x1 = x1;
    this->y1 = y1;
    this->x2 = x2;
    this->y2 = y2;
    this->x3 = x3;
    this->y3 = y3;
    this->distance = distance;
    this->type = type;
}

DrawObject::DrawObject(int colour, int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4, double distance, short type){
    this->colour = colour;
    this->x1 = x1;
    this->y1 = y1;
    this->x2 = x2;
    this->y2 = y2;
    this->x3 = x3;
    this->y3 = y3;
    this->x4 = x4;
    this->y4 = y4;
    this->distance = distance;
    this->type = type;
}

void DrawObject::draw(Renderer *renderer){
    switch(type){
            case CIRCLE: 
                renderer->drawCircle(colour, x1, y1, size*2);
                break;
            case LINE:
                renderer->drawLine(colour, x1, y1, x2, y2);
                break;
            case RECTANGLE:
                renderer->drawRect(colour, x1, y1, x2-x1, y2-y1);
                break;
            case TRIANGLE:
                // ----------------------------------------------------------- TODO -----------------------------------------------------------
                // Draw triangle
                break;
            case POINT:
                renderer->drawPoint(colour, x1, y1);
                break;
            case POLYGON:
                // ----------------------------------------------------------- TODO -----------------------------------------------------------
                // Draw Polygon
                break;
            case PLUS:
                renderer->drawLine(colour, x1-1, y1, x1+1, y1);
                renderer->drawPoint(colour, x1, y1-1);
                renderer->drawPoint(colour, x1, y1+1);
                break;
            default:
                printf("Not implemented drawObject, namely number %d\n", type);
        }
}