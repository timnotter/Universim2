#include <stdio.h>
#include <cmath>
#include "stellarObjectRenderFace.hpp"
#include "stellarObject.hpp"
#include "constants.hpp"
#include <map>

StellarObjectRenderFace::StellarObjectRenderFace(){
    this->owner = nullptr;
    resolution = 0;
}

StellarObjectRenderFace::StellarObjectRenderFace(StellarObject *owner){
    this->owner = owner;
    resolution = 0;
}

void StellarObjectRenderFace::initialise(StellarObject *owner){
    this->owner = owner;
}

void StellarObjectRenderFace::updateRenderFaces(short resolution){
    if(this->resolution == resolution) return;

    // ------------------------------------------------------- TODO -------------------------------------------------------
    // Multithread this bad boy

    // Start new 

    // struct timespec prevTime;
	// struct timespec currTime;
    // int updateTime;
    // clock_gettime(CLOCK_MONOTONIC, &prevTime);

    this->resolution = resolution;

    PositionVector points[resolution*resolution];

    // This is where we store the normalised and rounded values for each
    // Deprecated
    PositionVector normalisations[resolution*resolution];

    // Here we initialise the distance between the mid point of a face and the centre of the object,
    // as well as the difference between each point on a face,
    // We assume that resolution is off (this should be enforced by the caller)
    // Thus we have a grid
    double distanceMidPointOwner = (resolution-1) / 2;
    double distanceBetweenTwoPoints = 1;

    std::map<PositionVector, PositionVector> pointsDirection;



    PositionVector pointTopLeft;
    PositionVector pointsArrayXDirection;
    PositionVector pointsArrayYDirection;

    for(RenderTriangle *triangle: renderTriangles){
        delete triangle;
    }
    renderTriangles.clear();

    PositionVector p1;
    PositionVector p2;
    PositionVector p3;
    PositionVector p4;
    PositionVector p5;
    PositionVector p6;

    // We go through all 6 sides("faces") of the square
    for(int axis=0;axis<3;axis++){
        for(int direction=-1;direction<=1;direction+=2){
            // First we initialise the pointTopLeft and the directions that the array of the current side will have
            switch(axis){
                case X_AXIS:
                    pointTopLeft = PositionVector(direction * distanceMidPointOwner, direction * distanceMidPointOwner, distanceMidPointOwner);
                    pointsArrayXDirection = PositionVector(0, (-1 * direction), 0);
                    pointsArrayYDirection = PositionVector(0, 0, -1);
                    break;
                case Y_AXIS:
                    pointTopLeft = PositionVector((-1 * direction) * distanceMidPointOwner, direction * distanceMidPointOwner, distanceMidPointOwner);
                    pointsArrayXDirection = PositionVector(direction, 0, 0);
                    pointsArrayYDirection = PositionVector(0, 0, -1);
                    break;
                case Z_AXIS:
                    pointTopLeft = PositionVector(direction * distanceMidPointOwner, distanceMidPointOwner, direction * distanceMidPointOwner);
                    pointsArrayXDirection = PositionVector(-1 * direction, 0, 0);
                    pointsArrayYDirection = PositionVector(0, -1, 0);
                    break;
            }

            // "Blow up" square, such that each points distance to the centre equals the radius
            // We add all the points to a map, which represents the direction of the particular point, that is calculated by averaging the neighbouring triangles
            for(int i=0;i<resolution;i++){
                for(int j=0;j<resolution;j++){
                    // We calculated the normalised distance vector from the centre
                    PositionVector xOffset = pointsArrayXDirection * i * distanceBetweenTwoPoints;
                    PositionVector yOffset = pointsArrayYDirection * j * distanceBetweenTwoPoints;
                    PositionVector normalisation = (pointTopLeft + (xOffset) + (yOffset)).normalise();

                    float noise;
                    short type = owner->getType();
                    if(type == GALACTIC_CORE || type == STARSYSTEM || type == STAR){
                        noise = 1;
                    }
                    else{
                        noise = 1 + ((owner->getSurfaceNoise()->fractal(3, normalisation.getX() + owner->getRandomVector().getX(), normalisation.getY() + owner->getRandomVector().getY(), normalisation.getZ() + owner->getRandomVector().getZ()))) / 10 / (std::max(0.50L, owner->getRadius()/(terranRadius/10)))/3;
                    }
                    // if(i == j){
                    //     printf("%s i = %d - noise: %f\n", owner->getName(), i, noise);
                    // }
                    PositionVector point = normalisation * owner->getRadius() * noise;

                    points[i * resolution + j] = point;
                    auto it = pointsDirection.find(point);
                    if (it != pointsDirection.end()) {
                        // printf("Point (%Lf, %Lf, %Lf) is already in map. Normalised coordinates are: (%.12Lf, %.12Lf, %.12Lf)\n", point.getX(), point.getY(), point.getZ(), normalisation.getX(), normalisation.getY(), normalisation.getZ());
                    }
                    else{
                        pointsDirection.insert(std::make_pair(point, PositionVector()));
                    }
                }
            }
            // printf("---------------------\n");

            // We first go through the whole grid, build the triangles and add up all the normals of bordering triangles for the verteces
            for(int i=0;i<resolution-1;i++){
                for(int j=0;j<resolution-1;j++){
                    p1 = points[(i)*resolution + (j+1)];
                    p2 = points[i*resolution + j];
                    p4 = points[(i+1)*resolution + (j)];
                    p5 = points[(i+1)*resolution + (j+1)];

                    int indexP3;
                    int indexP6;
                    if((i+j)%2 == 1){
                        // Top left triangle
                        indexP3 = (i+1)*resolution + (j);
                        // Bottom right triangle
                        indexP6 = (i)*resolution + (j+1);
                    }
                    else{
                        // Top right triangle
                        indexP3 = (i+1)*resolution + (j+1);
                        // Bottom left triangle
                        indexP6 = (i)*resolution + j;
                    }
                    p3 = points[indexP3];
                    p6 = points[indexP6];

                    PositionVector normalVector;
                    renderTriangles.push_back(new RenderTriangle(p1, p2, p3));
                    normalVector = renderTriangles.back()->getNormalVector();
                    pointsDirection[p1] += normalVector;
                    pointsDirection[p2] += normalVector;
                    pointsDirection[p3] += normalVector;

                    renderTriangles.push_back(new RenderTriangle(p4, p5, p6));
                    normalVector = renderTriangles.back()->getNormalVector();
                    pointsDirection[p4] += normalVector;
                    pointsDirection[p5] += normalVector;
                    pointsDirection[p6] += normalVector;
                }
            }
        }
    }
    // After building all the triangles, we go through all of them and add the directions of each point to the triangles
    for(RenderTriangle *renderTriangle: renderTriangles){        
        renderTriangle->setPoint1Normal(pointsDirection[renderTriangle->getPoint1()]);
        renderTriangle->setPoint2Normal(pointsDirection[renderTriangle->getPoint2()]);
        renderTriangle->setPoint3Normal(pointsDirection[renderTriangle->getPoint3()]);
    }
    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
    // printf("Updating renderFaces took %d mics\n", updateTime);
}
// ---------------------------------------------------------------------------------------------------------------------------------
void StellarObjectRenderFace::getRenderTriangles(std::vector<RenderTriangle*> *triangles, PositionVector absoluteCameraPosition, PositionVector cameraDirection){
    // struct timespec prevTime;
	// struct timespec currTime;
    // int updateTime;
    // clock_gettime(CLOCK_MONOTONIC, &prevTime);
    // int counter = 0;
    for(RenderTriangle *triangle: renderTriangles){
        // If the normal vector of the triangle points in the same direction as the vector from the camera to the object centre, ass well es to the triangle centre, 
        // the triangle does not have to be rendered, since it must be behind other triangles
        // Sadly the calculations with the cameraDirection does not work. triangle->getNormalVector().normalise().dotProduct(cameraDirection)<0
        PositionVector cameraToObject = owner->getPositionAtPointInTime() - absoluteCameraPosition;
        PositionVector cameraToTriangle = triangle->getMidPoint() + owner->getPositionAtPointInTime() - absoluteCameraPosition;

        if(triangle->getNormalVector().normalise().dotProduct(cameraToObject)<0 && triangle->getNormalVector().normalise().dotProduct(cameraToTriangle)<0 ){
            // printf("Dot product: %Lf\n", triangle->getNormalVector().normalise().dotProduct(cameraDirection));
            triangles->push_back(triangle);
            // counter++;
        }
    }
    // printf("Added %d triangles, ", counter);
    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
    // printf("Returning triangles took %d mics\n", updateTime);
}