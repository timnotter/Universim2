#ifndef RENDER_TRIANGLE_HPP
#define RENDER_TRIANGLE_HPP

#include "positionVector.hpp"

class RenderTriangle{
private:
    PositionVector point1;
    PositionVector point2;
    PositionVector point3;
    PositionVector normalVector;
    // Used for distance calculation
    PositionVector midPoint;

public:
    RenderTriangle(PositionVector point1, PositionVector point2, PositionVector point3);
    PositionVector getPoint1();
    PositionVector getPoint2();
    PositionVector getPoint3();
    PositionVector getNormalVector();
    PositionVector getMidPoint();
    void switchDirectionOfNormalVector();

    void setPoint1(PositionVector newPoint);
    void setPoint2(PositionVector newPoint);
    void setPoint3(PositionVector newPoint);
};

#endif