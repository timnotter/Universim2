#include "renderTriangle.hpp"

RenderTriangle::RenderTriangle(PositionVector point1, PositionVector point2, PositionVector point3){
    this->point1 = point1;
    this->point2 = point2;
    this->point3 = point3;
    point1Normal = PositionVector();
    point2Normal = PositionVector();
    point3Normal = PositionVector();
    midPoint = (point1 + point2 + point3) / 3;

    // Normal vector does not neccesseraly have the right direction, since we have no constraints on point ordering
    // Manually adjust them if neccessary
    normalVector = PositionVector(point2 - point1).crossProduct(PositionVector(point3 - point1));
}

RenderTriangle::RenderTriangle(PositionVector point1, PositionVector point2, PositionVector point3, PositionVector point1Normal, PositionVector point2Normal, PositionVector point3Normal){
    this->point1 = point1;
    this->point2 = point2;
    this->point3 = point3;
    this->point1Normal = point1Normal.normalise();
    this->point2Normal = point2Normal.normalise();
    this->point3Normal = point3Normal.normalise();
    midPoint = (point1 + point2 + point3) / 3;

    // Normal vector does not neccesseraly have the right direction, since we have no constraints on point ordering
    // Manually adjust them if neccessary
    normalVector = PositionVector(point2 - point1).crossProduct(PositionVector(point3 - point1).normalise());
}

PositionVector RenderTriangle::getPoint1(){
    return point1;
}

PositionVector RenderTriangle::getPoint2(){
    return point2;
}

PositionVector RenderTriangle::getPoint3(){
    return point3;
}

PositionVector RenderTriangle::getPoint1Normal(){
    return point1Normal;
}

PositionVector RenderTriangle::getPoint2Normal(){
    return point2Normal;
}

PositionVector RenderTriangle::getPoint3Normal(){
    return point3Normal;
}

PositionVector RenderTriangle::getNormalVector(){
    return normalVector;
}

PositionVector RenderTriangle::getMidPoint(){
    return midPoint;
}

void RenderTriangle::switchDirectionOfNormalVector(){
    normalVector *= -1;
    point1Normal *= -1;
    point2Normal *= -1;
    point3Normal *= -1;
}

void RenderTriangle::setPoint1(PositionVector newPoint){
    point1 = newPoint;
}

void RenderTriangle::setPoint2(PositionVector newPoint){
    point2 = newPoint;
}

void RenderTriangle::setPoint3(PositionVector newPoint){
    point3 = newPoint;
}

void RenderTriangle::setPoint1Normal(PositionVector newPoint){
    point1Normal = newPoint.normalise();
}

void RenderTriangle::setPoint2Normal(PositionVector newPoint){
    point2Normal = newPoint.normalise();
}

void RenderTriangle::setPoint3Normal(PositionVector newPoint){
    point3Normal = newPoint.normalise();
}