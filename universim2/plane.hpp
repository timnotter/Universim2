#ifndef PLANE_HPP
#define PLANE_HPP

#include "positionVector.hpp"

class Plane{
private:
    long double a;
    long double b;
    long double c;
    long double d;

public:
    Plane();
    Plane(long double a, long double b, long double c, long double d);
    // Support vector and normal vector
    Plane(PositionVector supportVector, PositionVector normalVector);
    // Support vector and two direction vectors
    Plane(PositionVector supportVector, PositionVector directionVector1, PositionVector directionVector2);

    // Calculate the intersection of the plane with the line from point1 to point2
    PositionVector findIntersectionWithLine(PositionVector point1, PositionVector point2);

    void setA(long double a);
    void setB(long double b);
    void setC(long double c);
    void setD(long double d);

    long double getA();
    long double getB();
    long double getC();
    long double getD();
};

#endif