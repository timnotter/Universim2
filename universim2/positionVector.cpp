#include <cmath>
#include <cstdio>
#include "positionVector.hpp"
#include "matrix3d.hpp"


PositionVector::PositionVector(){
    validLength = false;
    x = 0;
    y = 0;
    z = 0;
    getLength();
}

PositionVector::PositionVector(double x, double y, double z){
    validLength = false;
    this->x = x;
    this->y = y;
    this->z = z;
    getLength();
}

PositionVector::PositionVector(PositionVector *parent){
    validLength = false;
    x = parent->getX();
    y = parent->getY();
    z = parent->getZ();
    getLength();
}

double PositionVector::getX(){
    return x;
}

double PositionVector::getY(){
    return y;
}

double PositionVector::getZ(){
    return z;
}

double PositionVector::getLength(){
    if(validLength){
        return length;
    }
    length = std::sqrt(std::pow(x, 2) + std::pow(y, 2) + std::pow(z, 2));
    validLength = true;
    return length;
}

void PositionVector::setX(double x){
    this->x = x;
    validLength = false;
}

void PositionVector::setY(double y){
    this->y = y;
    validLength = false;
}

void PositionVector::setZ(double z){
    this->z = z;
    validLength = false;
}

double PositionVector::distance(PositionVector position){
    return std::sqrt(std::pow(x - position.getX(), 2) + std::pow(y - position.getY(), 2) + std::pow(z - position.getZ(), 2));
}

PositionVector PositionVector::normalise(){
    double vectorLength = getLength();
    return PositionVector(x/vectorLength, y/vectorLength, z/vectorLength);
}

PositionVector PositionVector::operator + (PositionVector position){
    return PositionVector(x + position.getX(), y + position.getY(), z + position.getZ());
}

PositionVector PositionVector::operator - (PositionVector position){
    return PositionVector(x - position.getX(), y - position.getY(), z - position.getZ());
}

PositionVector PositionVector::operator * (double multiplier){
    return PositionVector(x * multiplier, y * multiplier, z * multiplier);
}

PositionVector PositionVector::operator / (double multiplier){
    return PositionVector(x / multiplier, y / multiplier, z / multiplier);
}

void PositionVector::operator = (PositionVector position){
    x = position.getX();
    y = position.getY();
    z = position.getZ();
    validLength = false;
}

void PositionVector::operator += (PositionVector position){
    x += position.getX();
    y += position.getY();
    z += position.getZ();
    validLength = false;
}

void PositionVector::operator -= (PositionVector position){
    x -= position.getX();
    y -= position.getY();
    z -= position.getZ();
    validLength = false;
}

void PositionVector::operator *= (double multiplier){
    x *= multiplier;
    y *= multiplier;
    z *= multiplier;
    validLength = false;
}

void PositionVector::operator /= (double multiplier){
    x /= multiplier;
    y /= multiplier;
    z /= multiplier;
    validLength = false;
}

bool PositionVector::operator == (PositionVector position){
    return (x == position.getX() && y == position.getY() && z == position.getZ());
}

double PositionVector::rQr(Matrix3d Q){
    PositionVector QR(0, 0, 0);
    QR = Qr(Q);
    return QR.getX() * x + QR.getY() * y + QR.getZ() * z;
}

PositionVector PositionVector::Qr(Matrix3d Q){
    PositionVector QR(0, 0, 0);
    QR.x = x * Q.getEntry(0, 0) + y * Q.getEntry(0, 1) + z * Q.getEntry(0, 2);
    QR.y = x * Q.getEntry(1, 0) + y * Q.getEntry(1, 1) + z * Q.getEntry(1, 2);
    QR.z = x * Q.getEntry(2, 0) + y * Q.getEntry(2, 1) + z * Q.getEntry(2, 2);
    return QR;
}