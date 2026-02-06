#include "helpers/plane.hpp"

Plane::Plane(){
    a = b = c = d = 0;
}

Plane::Plane(long double a, long double b, long double c, long double d){
    this->a = a;
    this->b = b;
    this->c = c;
    this->d = d;
}

Plane::Plane(PositionVector supportVector, PositionVector normalVector){
    a = normalVector.getX();
    b = normalVector.getY();
    c = normalVector.getZ();
    d = a * supportVector.getX() + b * supportVector.getY() + c * supportVector.getZ();
}

Plane::Plane(PositionVector supportVector, PositionVector directionVector1, PositionVector directionVector2){
    PositionVector crossProduct = directionVector1.crossProduct(directionVector2);
    a = crossProduct.getX();
    b = crossProduct.getY();
    c = crossProduct.getZ();
    d = a * supportVector.getX() + b * supportVector.getY() + c * supportVector.getZ();
}

PositionVector Plane::findIntersectionWithLine(PositionVector point1, PositionVector point2){
    PositionVector directionVector = point1 - point2;

    // We insert the general line point into the plane equation and search for the unknown
    long double u = (d - a * point2.getX() - b * point2.getY() - c * point2.getZ()) / (a * directionVector.getX() + b * directionVector.getY() + c * directionVector.getZ());
    
    return point2 + directionVector * u;
}

void Plane::setA(long double a){
    this->a = a;
}

void Plane::setB(long double b){
    this->b = b;
}

void Plane::setC(long double c){
    this->c = c;
}

void Plane::setD(long double d){
    this->d = d;
}

long double Plane::getA(){
    return a;
}

long double Plane::getB(){
    return b;
}

long double Plane::getC(){
    return c;
}

long double Plane::getD(){
    return d;
}
