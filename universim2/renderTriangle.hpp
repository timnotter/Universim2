#ifndef RENDER_TRIANGLE_HPP
#define RENDER_TRIANGLE_HPP

#include "positionVector.hpp"

class RenderTriangle{
private:
    PositionVector point1;
    PositionVector point2;
    PositionVector point3;
    PositionVector normalVector;
    PositionVector point1Normal;
    PositionVector point2Normal;
    PositionVector point3Normal;
    // Used for distance calculation
    PositionVector midPoint;

public:
    RenderTriangle(PositionVector point1, PositionVector point2, PositionVector point3);
    RenderTriangle(PositionVector point1, PositionVector point2, PositionVector point3, PositionVector point1Normal, PositionVector point2Normal, PositionVector point3Normal);
    PositionVector getPoint1();
    PositionVector getPoint2();
    PositionVector getPoint3();
    PositionVector getPoint1Normal();
    PositionVector getPoint2Normal();
    PositionVector getPoint3Normal();
    PositionVector getNormalVector();
    PositionVector getMidPoint();
    void switchDirectionOfNormalVector();

    void setPoint1(PositionVector newPoint);
    void setPoint2(PositionVector newPoint);
    void setPoint3(PositionVector newPoint);
    void setPoint1Normal(PositionVector newPoint);
    void setPoint2Normal(PositionVector newPoint);
    void setPoint3Normal(PositionVector newPoint);
};

#endif