#ifndef POINT_2D_HPP
#define POINT_2D_HPP


class Point2d{
private:
    int x;
    int y;

public:
    Point2d();
    Point2d(int x, int y);

    void setX(int x);
    void setY(int y);

    int getX();
    int getY();

    const char *toString();
};

#endif