#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <math.h>

const static long double sagittariusRadius = 51800000000;                            // m
const static long double solarRadius = 696392000;
const static long double terranRadius = 6371000;
const static long double lunarRadius = 1737400;

const static long double sagittariusMass = 8.26 * pow(10, 36);                       // kg
const static long double solarMass = 1.98855 * pow(10, 30);
const static long double terranMass = 5.97217 * pow(10, 24);
const static long double lunarMass = 7.342 * pow(10, 22);

const static long double lightspeed = 299792458;                                     // m/s

const static long double lightyear = 9.4607 * pow(10, 15);                           // m
const static long double astronomicalUnit = 1.495978707 * pow(10, 11);
const static long double distanceEarthMoon = 384399000;
const static long double distanceSagittariusSun = 26660 * lightyear;

const static long double G = 6.67430 * pow(10, -11);                                 // Gravitational constant
const static long double PI = 3.14159265358979323846;                                // Pi

#endif