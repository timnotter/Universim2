#include "graphicInterface/point2d.hpp"
#include <sstream>
#include <string>

Point2d::Point2d(){
    x = y = 0;
}

Point2d::Point2d(int x, int y){
    this->x = x;
    this->y = y;
}

void Point2d::setX(int x){
    this->x = x;
}

void Point2d::setY(int y){
    this->y = y;
}

int Point2d:: getX(){
    return x;
}

int Point2d:: getY(){
    return y;
}

const char *Point2d::toString(){
    std::ostringstream outputStream;
    std::string pointString;
    // outputStream.precision(20);
    outputStream << x << ", " << y;
    pointString = outputStream.str();

    return pointString.c_str();
}

bool Point2d::operator == (Point2d position){
    return (x == position.getX() && y == position.getY());
}
