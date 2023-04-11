#include <stdio.h>
#include "matrix3d.hpp"

Matrix3d::Matrix3d(){
    vector0 = PositionVector();
    vector1 = PositionVector();
    vector2 = PositionVector();
}

Matrix3d::Matrix3d(PositionVector vector0, PositionVector vector1, PositionVector vector2){
    this->vector0 = PositionVector(&vector0);
    this->vector1 = PositionVector(&vector1);
    this->vector2 = PositionVector(&vector2);
}

PositionVector Matrix3d::operator * (PositionVector vector){
    PositionVector output = PositionVector();
    output.setX(vector.getX() * vector0.getX() + vector.getY() * vector1.getX() + vector.getZ() * vector2.getX());
    output.setY(vector.getX() * vector0.getY() + vector.getY() * vector1.getY() + vector.getZ() * vector2.getY());
    output.setZ(vector.getX() * vector0.getZ() + vector.getY() * vector1.getZ() + vector.getZ() * vector2.getZ());
    // printf("first arg = %f, + second arg = %f, + third arg = %f, third arg solo: %f * %f = %f, first + third arg: %f\n", 
    // vector.getX() * vector0.getZ(), vector.getX() * vector0.getZ() + vector.getY() * vector1.getZ(), 
    // vector.getX() * vector0.getZ() + vector.getY() * vector1.getZ() + vector.getZ() * vector2.getZ(), vector.getZ(), vector2.getZ(), vector.getZ() * vector2.getZ(), 
    // vector.getX() * vector0.getZ() + vector.getZ() * vector2.getZ());
    return output;
}

void Matrix3d::setVector(PositionVector vector, short index){
    switch(index){
        case 0: 
            vector0 = vector;
            break;
        case 1:
            vector1 = vector;
            break;
        case 2:
            vector2 = vector;
            break;
    }
}

PositionVector Matrix3d::getVector(short index){
    switch(index){
        case 0: 
            return vector0;
        case 1:
            return vector1;
        case 2:
            return vector2;
        default:
            return PositionVector();
    }
}

long double Matrix3d::getEntry(short vector, short entry){
    switch(entry){
        case 0:
            return getVector(vector).getX();
        case 1:
            return getVector(vector).getY();
        case 2:
            return getVector(vector).getZ();
        default:
            printf("Unsupported parameter for getEntry function!\n");
            return 0;
    }
}

void Matrix3d::setEntry(short vector, short entry, long double value){
    switch(entry){
        case 0:
            getVector(vector).setX(value);
            break;
        case 1:
            getVector(vector).setY(value);
            break;
        case 2:
            getVector(vector).setZ(value);
            break;
        default:
            printf("Unsupported parameter for setEntry function!\n");
    }
}

Matrix3d Matrix3d::transpose(){
    PositionVector tempVector0 = vector0;
    PositionVector tempVector1 = vector1;
    PositionVector tempVector2 = vector2;
    tempVector0.setX(vector0.getX());
    tempVector0.setY(vector1.getX());
    tempVector0.setZ(vector2.getX());
    tempVector1.setX(vector0.getY());
    tempVector1.setY(vector1.getY());
    tempVector1.setZ(vector2.getY());
    tempVector2.setX(vector0.getZ());
    tempVector2.setY(vector1.getZ());
    tempVector2.setZ(vector2.getZ());
    Matrix3d output = Matrix3d(tempVector0, tempVector1, tempVector2);
    return output;
}

void Matrix3d::print(){
    printf("[(%s), (%s), (%s)]\n", vector0.toString(), vector1.toString(), vector2.toString());
}