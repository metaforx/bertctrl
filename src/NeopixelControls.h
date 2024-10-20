#ifndef NEOPIXEL_CONTROL_H
#define NEOPIXEL_CONTROL_H

#include "Particle.h"
#include "neopixel.h"

// Declare the strip object as extern
extern Adafruit_NeoPixel strip;

extern const int pixelPin;
extern const int pixelCount;
extern const int pixelType;
extern const int brightness;

extern const int fadeSteps;
extern const int fadeDelay;

#endif // NEOPIXEL_CONTROL_H