#include "NeopixelControls.h"

const bool NeopixelControlsDebug = false;
uint32_t *temperatureDisplayColors = new uint32_t[16];

void fadeInPixel(int pixel, uint32_t color)
{
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    if (NeopixelControlsDebug)
    {
        Serial.print("Fading in pixel: ");
        Serial.print(pixel);
        Serial.print(" with color: ");
        Serial.println(color, HEX);
        Serial.println(strip.getPixelColor(pixel), HEX);
    }
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

uint32_t interpolateColor(uint32_t color1, uint32_t color2, float fraction)
{
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;

    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;

    uint8_t r = r1 + fraction * (r2 - r1);
    uint8_t g = g1 + fraction * (g2 - g1);
    uint8_t b = b1 + fraction * (b2 - b1);

    return (r << 16) | (g << 8) | b;
}

// Default values for colors
void precomputeColors()
{
    uint32_t numPixels = strip.numPixels();

    // COLORS /// INTERPOLATION
    uint32_t blue = 0x0000FF;   // Blue
    uint32_t green = 0x00FF00;  // Green
    uint32_t purple = 0x800080; // Purple
    uint32_t red = 0xFF0000;    // Red

    // Number of steps per segment
    uint32_t segmentSteps = numPixels / 3;
    uint32_t remainingSteps = numPixels % 3;

    // Precompute the colors in the array
    for (uint32_t step = 0; step < numPixels; step++)
    {
        float fraction;
        uint32_t color;

        if (step < segmentSteps)
        {
            // Interpolate from Blue to Green
            fraction = static_cast<float>(step) / segmentSteps;
            color = interpolateColor(blue, green, fraction);
        }
        else if (step < 2 * segmentSteps + remainingSteps)
        {
            // Interpolate from Green to Purple
            fraction = static_cast<float>(step - segmentSteps) / segmentSteps;
            color = interpolateColor(green, purple, fraction);
        }
        else
        {
            // Interpolate from Purple to Red
            fraction = static_cast<float>(step - 2 * segmentSteps - remainingSteps) / segmentSteps;
            color = interpolateColor(purple, red, fraction);
        }

        // Store the color in the array
        temperatureDisplayColors[step] = color;
    }
}