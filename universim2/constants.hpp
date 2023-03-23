#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <math.h>

const static double sagittariusRadius = 51800000000;                            // m
const static double solarRadius = 696392000;
const static double terranRadius = 6371000;
const static double lunarRadius = 1737400;

const static double sagittariusMass = 8.26 * pow(10, 36);                       // kg
const static double solarMass = 1.98855 * pow(10, 30);
const static double terranMass = 5.97217 * pow(10, 24);
const static double lunarMass = 7.342 * pow(10, 22);

const static double lightspeed = 299792458;                                     // m/s

const static double lightyear = 9.4607 * pow(10, 15);                           // m
const static double astronomicalUnit = 1.495978707 * pow(10, 11);
const static double distanceEarthMoon = 384399000;
const static double distanceSagittariusSun = 26660 * lightyear;

const static double G = 6.67430 * pow(10, -11);                                 // Gravitational constant
const static double PI = 3.14159265358979323846;                                // Pi

#endif