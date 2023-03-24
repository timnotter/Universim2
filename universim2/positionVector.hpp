#ifndef POSITIONVECTOR_HPP
#define POSITIONVECTOR_HPP

#include <string>

class Matrix3d;

class PositionVector{
private:
    double x;
    double y;
    double z;
    double length;
    bool validLength = false;
    std::string vectorString;

public:
    PositionVector();
    PositionVector(double x, double y, double z);
    PositionVector(PositionVector *parent);
    double getX();
    double getY();
    double getZ();
    double getLength();
    void setX(double x);
    void setY(double y);
    void setZ(double z);
    double distance(PositionVector position);
    // Returns the normalised value of the vector, without changing the vector itself
    PositionVector normalise();
    PositionVector operator + (PositionVector position);
    PositionVector operator - (PositionVector position);
    PositionVector operator * (double multiplier);
    PositionVector operator / (double multiplier);
    void operator = (PositionVector position);
    void operator += (PositionVector position);
    void operator -= (PositionVector position);
    void operator *= (double multiplier);
    void operator /= (double multiplier);
    bool operator == (PositionVector position);

    // Functions for the computation of multipole expansion
    double rQr(Matrix3d Q);
    PositionVector Qr(Matrix3d Q);

    const char *toString();
    
};

#endif