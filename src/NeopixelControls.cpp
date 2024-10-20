#include "NeopixelControls.h"

void fadeInPixel(int pixel, uint32_t color)
{
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    Serial.print("Fading in pixel: ");
    Serial.print(pixel);
    Serial.print(" with color: ");
    Serial.println(color, HEX);
    Serial.println(strip.getPixelColor(pixel), HEX);
    if (strip.getPixelColor(pixel) == 0 || strip.getPixelColor(pixel) != color)
    {
        for (int step = 0; step <= fadeSteps; step++)
        {
            float fraction = (float)step / fadeSteps;
            uint8_t rStep = r * fraction;
            uint8_t gStep = g * fraction;
            uint8_t bStep = b * fraction;
            strip.setPixelColor(pixel, strip.Color(rStep, gStep, bStep));

            strip.show();
            delay(fadeDelay);
        }
    }
}

void fadeOutPixel(int pixel)
{
    uint32_t color = strip.getPixelColor(pixel);
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    if (strip.getPixelColor(pixel) != 0)
    {
        for (int step = fadeSteps; step >= 0; step--)
        {
            float fraction = (float)step / fadeSteps;
            uint8_t rStep = r * fraction;
            uint8_t gStep = g * fraction;
            uint8_t bStep = b * fraction;
            strip.setPixelColor(pixel, strip.Color(rStep, gStep, bStep));
            strip.show();
            delay(fadeDelay);
        }
        strip.setPixelColor(pixel, 0); // Ensure the pixel is completely off
        strip.show();
    }
}

void blinkLED(int pixel, uint32_t color, int times, int delayTime)
{
    for (int i = 0; i < times; i++)
    {
        strip.setPixelColor(pixel, color);
        strip.show();
        delay(delayTime);
        strip.setPixelColor(pixel, 0); // Turn off the LED
        strip.show();
        delay(delayTime);
    }
}

void clearLEDs()
{
    for (int i = 0; i < strip.numPixels(); i++)
    {
        strip.setPixelColor(i, strip.Color(0, 0, 0)); // Turn off each LED
    }
    strip.show();
}