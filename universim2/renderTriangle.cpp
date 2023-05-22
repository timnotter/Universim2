#include "renderTriangle.hpp"

RenderTriangle::RenderTriangle(PositionVector point1, PositionVector point2, PositionVector point3){
    this->point1 = point1;
    this->point2 = point2;
    this->point3 = point3;
    midPoint = (point1 + point2 + point3) / 3;

    // Normal vector does not neccesseraly have the right direction, since we have no constraints on point ordering
    // Manually adjust them if neccessary
    normalVector = PositionVector(point2 - point1).crossProduct(PositionVector(point3 - point1));
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

PositionVector RenderTriangle::getNormalVector(){
    return normalVector;
}

PositionVector RenderTriangle::getMidPoint(){
    return midPoint;
}

void RenderTriangle::switchDirectionOfNormalVector(){
    normalVector *= -1;
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