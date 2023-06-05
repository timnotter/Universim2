#include <stdio.h>
#include <cmath>
#include "stellarObjectRenderFace.hpp"
#include "stellarObject.hpp"
#include "constants.hpp"

StellarObjectRenderFace::StellarObjectRenderFace(){
    this->owner = nullptr;
    axis = -1;
    direction = 0;
    resolution = 0;
}

StellarObjectRenderFace::StellarObjectRenderFace(StellarObject *owner){
    this->owner = owner;
    axis = -1;
    direction = 0;
    resolution = 0;
}

void StellarObjectRenderFace::initialise(StellarObject *owner){
    this->owner = owner;
}

void StellarObjectRenderFace::updateRenderFace(short axis, short direction, short resolution){
    if(this->resolution == resolution) return;

    // struct timespec prevTime;
	// struct timespec currTime;
    // int updateTime;
    // clock_gettime(CLOCK_MONOTONIC, &prevTime);

    this->axis = axis;
    this->direction = direction;
    this->resolution = resolution;

    points = new PositionVector[resolution*resolution];

    // PositionVector pointTopLeft = owner->getPosition();
    PositionVector pointTopLeft;
    double distanceMidPointOwner = std::sqrt(std::pow(owner->getRadius(), 2)/2);
    double distanceBetweenTwoPoints = (2*distanceMidPointOwner)/(resolution-1);
    // printf("%f, %f\n", distanceBetweenTwoPoints, distanceMidPointOwner);

    switch(axis){
        case X_AXIS:
            pointTopLeft = PositionVector(direction * distanceMidPointOwner, direction * distanceMidPointOwner, direction * distanceMidPointOwner);
            // printf("%s\n", PositionVector(direction * distanceMidPointOwner, -distanceMidPointOwner, -distanceMidPointOwner).toString());
            for(int i=0;i<resolution;i++){
                for(int j=0;j<resolution;j++){
                    points[i*resolution + j] = pointTopLeft + PositionVector(0, i * -direction * distanceBetweenTwoPoints, j * -direction * distanceBetweenTwoPoints);
                }
            }
            break;
        case Y_AXIS:
            pointTopLeft = PositionVector(direction * distanceMidPointOwner, direction * distanceMidPointOwner, direction * distanceMidPointOwner);
            for(int i=0;i<resolution;i++){
                for(int j=0;j<resolution;j++){
                    points[i*resolution + j] = pointTopLeft + PositionVector(i * -direction * distanceBetweenTwoPoints, 0, j * -direction * distanceBetweenTwoPoints);
                }
            }
            break;
        case Z_AXIS:
            pointTopLeft = PositionVector(direction * distanceMidPointOwner, direction * distanceMidPointOwner, direction * distanceMidPointOwner);
            for(int i=0;i<resolution;i++){
                for(int j=0;j<resolution;j++){
                    points[i*resolution + j] = pointTopLeft + PositionVector(i * -direction * distanceBetweenTwoPoints, j * -direction * distanceBetweenTwoPoints, 0);
                }
            }
            break;
    }

    // "Blow up" square, such that each points distance to the centre equals the radius
    PositionVector distanceVector;
    for(int i=0;i<resolution;i++){
        for(int j=0;j<resolution;j++){
            // distanceVector = points[i*resolution + j] - owner->getPosition();
            // points[i*resolution + j] = owner->getPosition() + distanceVector.normalise() * owner->getRadius();
            // printf("Noise: %f\n", owner->getSurfaceNoise()->noise(points[i*resolution + j].getX(), points[i*resolution + j].getY(), points[i*resolution + j].getZ()));
            PositionVector normalisation = points[i*resolution + j].normalise();
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
            points[i*resolution + j] = points[i*resolution + j].normalise() * owner->getRadius() * noise;
            // points[i*resolution + j] = pointTopLeft + PositionVector(i * distanceBetweenTwoPoints, j * distanceBetweenTwoPoints, 0);
        }
    }

    // Then clean and fill the renderTriangles with relative positions to their owners position
    for(RenderTriangle *triangle: renderTriangles){
        delete triangle;
    }
    renderTriangles.clear();

    for(int i=0;i<resolution-1;i++){
        for(int j=0;j<resolution-1;j++){
            // Top left triangle
            renderTriangles.push_back(new RenderTriangle(points[i*resolution + j], points[(i+1)*resolution + j], points[(i)*resolution + (j+1)]));
            // Bottom right triangle
            renderTriangles.push_back(new RenderTriangle(points[(i+1)*resolution + j], points[(i+1)*resolution + (j+1)], points[i*resolution + (j+1)]));
        }
    }

    
    // If neccessary adjust the normal vectors. This has to be done, because of the ordering of the points in the triangles
    if((axis==X_AXIS && direction == BACKWARDS) || (axis==Y_AXIS && direction == FORWARDS) || (axis==Z_AXIS && direction == BACKWARDS)){
        for(RenderTriangle *renderTriangle: renderTriangles){
            renderTriangle->switchDirectionOfNormalVector();
        }
    }

    // clock_gettime(CLOCK_MONOTONIC, &currTime);
    // updateTime = ((1000000000*(currTime.tv_sec-prevTime.tv_sec)+(currTime.tv_nsec-prevTime.tv_nsec))/1000);
    // printf("Updating renderFaces took %d mics\n", updateTime);
}

void StellarObjectRenderFace::getRenderTriangles(std::vector<RenderTriangle*> *triangles, PositionVector absoluteCameraPosition){
    // struct timespec prevTime;
	// struct timespec currTime;
    // int updateTime;
    // clock_gettime(CLOCK_MONOTONIC, &prevTime);
    // int counter = 0;
    for(RenderTriangle *triangle: renderTriangles){
        // If the normal vector of the triangle points in the same direction as the vector from the camera to the object center, 
        // the triangle does not have to be rendered, since it must be behind other triangles
        PositionVector objectToCamera = owner->getPositionAtPointInTime() - absoluteCameraPosition;

        if(triangle->getNormalVector().normalise().dotProduct(objectToCamera)<0){
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