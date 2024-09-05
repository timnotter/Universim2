#ifndef MATRIX3D_HPP
#define MATRIX3D_HPP

#include "positionVector.hpp"

class Matrix3d{
private:
    PositionVector vector0;
    PositionVector vector1;
    PositionVector vector2;

public:
    Matrix3d();
    Matrix3d(PositionVector vector0, PositionVector vector1, PositionVector vector2);
    void setVector(PositionVector vector, short index);
    PositionVector getVector(short index);
    long double getEntry(short vector, short entry);
    // Used to alter a single value
    void setEntry(short vector, short entry, long double value);
    PositionVector operator * (PositionVector vector);
    Matrix3d transpose();
    void print();
};

#endif