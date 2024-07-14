#ifndef POSITIONVECTOR_HPP
#define POSITIONVECTOR_HPP

#include <string>

class Matrix3d;

class PositionVector{
private:
    long double x;
    long double y;
    long double z;
    long double length;
    bool validLength = false;
    std::string vectorString;

public:
    PositionVector();
    PositionVector(long double x, long double y, long double z);
    PositionVector(PositionVector *parent);
    long double getX() const;
    long double getY() const;
    long double getZ() const;
    long double getLength();
    void setX(long double x);
    void setY(long double y);
    void setZ(long double z);
    long double distance(PositionVector position);
    PositionVector crossProduct(PositionVector vector);
    long double dotProduct(PositionVector vector);
    PositionVector piecewiseProduct(PositionVector vector);
    // Returns the normalised value of the vector, without changing the vector itself
    PositionVector normalise();
    PositionVector operator + (PositionVector position);
    PositionVector operator - (PositionVector position);
    PositionVector operator * (long double multiplier);
    PositionVector operator / (long double multiplier);
    PositionVector operator / (PositionVector position);
    void operator = (PositionVector position);
    void operator += (PositionVector position);
    void operator -= (PositionVector position);
    void operator *= (long double multiplier);
    void operator /= (long double multiplier);
    bool operator == (const PositionVector& position) const;
    bool operator < (const PositionVector& position) const;
    bool operator > (const PositionVector& position) const;
    bool operator <= (const PositionVector& position) const;
    bool operator >= (const PositionVector& position) const;
    bool operator != (const PositionVector& position) const;

    // Functions for the computation of multipole expansion
    long double rQr(Matrix3d Q);
    PositionVector Qr(Matrix3d Q);

    const char *toString();
    
};

#endif