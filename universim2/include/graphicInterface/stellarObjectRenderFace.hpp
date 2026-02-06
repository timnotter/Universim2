#ifndef STELLAR_OBJECT_RENDER_FACE_HPP
#define STELLAR_OBJECT_RENDER_FACE_HPP

#include <vector>
#include "graphicInterface/renderTriangle.hpp"
#include "helpers/matrix3d.hpp"

#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2
#define FORWARDS 1
#define BACKWARDS -1

class StellarObject;

class StellarObjectRenderFace {
private:
    StellarObject *owner;
    short resolution;
    std::vector<RenderTriangle*> renderTriangles;


    // Rebuilding this class to store all 6 sides of the stellarObject at the same time


public:
    StellarObjectRenderFace();
    StellarObjectRenderFace(StellarObject *owner);
    void initialise(StellarObject *owner);
    // StellarObjectRenderFace(StellarObject *owner, short axis, short direction, short resolution);
    void updateRenderFaces(short resolution);
    void getRenderTriangles(std::vector<RenderTriangle*> *triangles, PositionVector absoluteCameraPosition, PositionVector cameraDirection);
};

#endif
