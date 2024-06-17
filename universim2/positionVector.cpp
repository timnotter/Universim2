#include <cmath>
#include <cstdio>
#include <sstream>
#include "positionVector.hpp"
#include "matrix3d.hpp"


PositionVector::PositionVector(){
    validLength = false;
    x = 0;
    y = 0;
    z = 0;
    getLength();
}

PositionVector::PositionVector(long double x, long double y, long double z){
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

long double PositionVector::getX() const{
    return x;
}

long double PositionVector::getY() const{
    return y;
}

long double PositionVector::getZ() const{
    return z;
}

long double PositionVector::getLength(){
    if(validLength){
        return length;
    }
    length = std::sqrt(std::pow(x, 2) + std::pow(y, 2) + std::pow(z, 2));
    validLength = true;
    return length;
}

void PositionVector::setX(long double x){
    this->x = x;
    validLength = false;
}

void PositionVector::setY(long double y){
    this->y = y;
    validLength = false;
}

void PositionVector::setZ(long double z){
    this->z = z;
    validLength = false;
}

long double PositionVector::distance(PositionVector position){
    return std::sqrt(std::pow(x - position.getX(), 2) + std::pow(y - position.getY(), 2) + std::pow(z - position.getZ(), 2));
}

PositionVector PositionVector::crossProduct(PositionVector vector){
    long double newX = y * vector.getZ() - z * vector.getY();
    long double newY = z * vector.getX() - x * vector.getZ();
    long double newZ = x * vector.getY() - y * vector.getX();
    return PositionVector(newX, newY, newZ);
}

long double PositionVector::dotProduct(PositionVector vector){
    return (x*vector.getX() + y*vector.getY() + z*vector.getZ());
}

PositionVector PositionVector::normalise(){
    long double vectorLength = getLength();
    return PositionVector(x/vectorLength, y/vectorLength, z/vectorLength);
}

PositionVector PositionVector::operator + (PositionVector position){
    return PositionVector(x + position.getX(), y + position.getY(), z + position.getZ());
}

PositionVector PositionVector::operator - (PositionVector position){
    return PositionVector(x - position.getX(), y - position.getY(), z - position.getZ());
}

PositionVector PositionVector::operator * (long double multiplier){
    return PositionVector(x * multiplier, y * multiplier, z * multiplier);
}

PositionVector PositionVector::operator / (long double multiplier){
    return PositionVector(x / multiplier, y / multiplier, z / multiplier);
}

PositionVector PositionVector::operator / (PositionVector position){
    return PositionVector(x / position.getX(), y / position.getY(), z / position.getZ());
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

void PositionVector::operator *= (long double multiplier){
    x *= multiplier;
    y *= multiplier;
    z *= multiplier;
    validLength = false;
}

void PositionVector::operator /= (long double multiplier){
    x /= multiplier;
    y /= multiplier;
    z /= multiplier;
    validLength = false;
}

bool PositionVector::operator == (const PositionVector& position) const{
    return (x == position.getX() && y == position.getY() && z == position.getZ());
}

bool PositionVector::operator < (const PositionVector& position) const{
    long double pX = position.getX();
    long double pY = position.getY();
    return ((x < pX) || (x == pX && y < pY) || (x == pX && y == pY && z < position.getZ()));
}

bool PositionVector::operator > (const PositionVector& position) const{
    long double pX = position.getX();
    long double pY = position.getY();
    return ((x > pX) || (x == pX && y > pY) || (x == pX && y == pY && z > position.getZ()));
}

bool PositionVector::operator <= (const PositionVector& position) const{
    return ((*this) < position || (*this) == position);
}

bool PositionVector::operator >= (const PositionVector& position) const{
    return ((*this) > position || (*this) == position);
}

bool PositionVector::operator != (const PositionVector& position) const{
    return !(x == position.getX() && y == position.getY() && z == position.getZ());
}

long double PositionVector::rQr(Matrix3d Q){
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

const char *PositionVector::toString(){
    std::ostringstream outputStream;
    // outputStream.precision(20);
    outputStream << x << ", " << y << ", " << z;
    vectorString = outputStream.str();

    return vectorString.c_str();
}