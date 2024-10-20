#ifndef NEOPIXEL_CONTROL_H
#define NEOPIXEL_CONTROL_H

#include "Particle.h"
#include "neopixel.h"

extern Adafruit_NeoPixel strip;
extern const bool NeopixelControlsDebug;
extern const int fadeSteps;
extern const int fadeDelay;

void fadeInPixel(int pixel, uint32_t color);
void fadeOutPixel(int pixel);
void blinkLED(int pixel, uint32_t color, int times, int delayTime);
void clearLEDs();

#endif // NEOPIXEL_CONTROL_H