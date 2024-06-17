#include <stdio.h>
#include "renderer.hpp"
#include "point2d.hpp"

DrawObject::DrawObject(double distance, short type){
    this->distance = distance;
    this->type = type;
    children = new std::vector<DrawObject*>();
}

DrawObject::DrawObject(int colour, long int x1, long int y1, double distance, short type){
    this->colour = colour;
    this->x1 = x1;
    this->y1 = y1;
    this->distance = distance;
    this->type = type;
}

DrawObject::DrawObject(int colour, long int x1, long int y1, int size, double distance, short type){
    this->colour = colour;
    this->x1 = x1;
    this->y1 = y1;
    this->size = size;
    this->distance = distance;
    this->type = type;
}

DrawObject::DrawObject(int colour, long int x1, long int y1, long int x2, long int y2, double distance, short type){
    this->colour = colour;
    this->x1 = x1;
    this->y1 = y1;
    this->x2 = x2;
    this->y2 = y2;
    this->distance = distance;
    this->type = type;
}

DrawObject::DrawObject(int colour, long int x1, long int y1, long int x2, long int y2, long int x3, long int y3, double distance, short type){
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

DrawObject::DrawObject(int colour, long int x1, long int y1, long int x2, long int y2, long int x3, long int y3, long int x4, long int y4, double distance, short type){
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

void DrawObject::addDrawObject(DrawObject *drawObject){
    if(type != GROUP){
        printf("Cannot add DrawObject to Non-Group DrawObject\n");
        return;
    }
    else if(drawObject->type == GROUP){
        printf("Cannot add Group DrawObject to another Group DrawObject\n");
        return;
    }
    children->push_back(drawObject);
}

void DrawObject::draw(Renderer *renderer){
    Point2d points[4];
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
                // printf("Drawing triangle in drawObject (%d, %d), (%d, %d), (%d, %d)\n", x1, y1, x2, y2, x3, y3);
                renderer->drawTriangle(colour, colourP1, colourP2, colourP3, x1, y1, x2, y2, x3, y3);
                break;
            case POINT:
                renderer->drawPoint(colour, x1, y1);
                break;
            case POLYGON:
                // Draw Polygon: we only implement polygons with four corners
                points[0] = Point2d(x1, y1);
                points[1] = Point2d(x2, y2);
                points[2] = Point2d(x3, y3);
                points[3] = Point2d(x4, y4);

                renderer->drawPolygon(colour, 4, points);
                // printf("Not implemented draw for polygon\n");
                break;
            case PLUS:
                renderer->getMyWindow()->drawLine(colour, x1-1, y1, x1+1, y1);
                renderer->getMyWindow()->drawPoint(colour, x1, y1-1);
                renderer->getMyWindow()->drawPoint(colour, x1, y1+1);
                break;
            case GROUP:
                // We first have to sort the children
                // printf("Drawing %ld drawObjects\n", children->size());
                quicksort(0, children->size(), children, 1, renderer->getRendererThreadCount());
                for(DrawObject *drawObject: *children){
                    drawObject->draw(renderer);
                    delete drawObject;
                }
                children->clear();
                delete children;
                break;
            default:
                printf("Not implemented drawObject, namely type number %d\n", type);
        }
}