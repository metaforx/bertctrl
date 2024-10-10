// Example usage for Adafruit_SGP30 library.
// This library was modified/wrapped by SJB (https://github.com/dyadica)
// in order to work with Particle Photon & Core.

#include "Particle.h"
#include <Wire.h>
#include "Adafruit_SGP30.h"
#include "Adafruit_Si7021.h"
#include "I2CScanner.h"
#include "neopixel.h"

Adafruit_Si7021 si = Adafruit_Si7021();
Adafruit_SGP30 sgp;
I2CScanner scanner;

/* ======================= prototypes =============================== */

uint32_t Wheel(byte WheelPos);
uint8_t red(uint32_t c);
uint8_t green(uint32_t c);
uint8_t blue(uint32_t c);
void colorWipe(uint32_t c, uint8_t wait);
void rainbowFade2White(uint8_t wait, int rainbowLoops, int whiteLoops);

// NEOPIXEL: Set pixel COUNT, PIN and TYPE
#define PIXEL_PIN D2
#define PIXEL_COUNT 16
#define PIXEL_TYPE WS2812B
#define BRIGHTNESS 50 // 0 - 255
Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

// RGB Rotary Encoder
#define RGB_ROTARY_ENCODER_SW_PIN A3
bool switchState = false;
bool lastSwitchState = false;

void setup()
{
  pinMode(RGB_ROTARY_ENCODER_SW_PIN, INPUT_PULLDOWN);

  Serial.begin(9600);
  Serial.println("Si7021 test");

  if (!si.begin())
  {
    Serial.println("Did not find Si7021 sensor!");
    while (1)
      ;
  }

  if (scanner.begin())
  {
    Serial.println("I2C bus initialized successfully.");
    scanner.scan();
  }
  else
  {
    Serial.println("Failed to initialize I2C bus.");
  }

  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}

int counter = 0;
void loop()
{
  int readSwitchState = digitalRead(RGB_ROTARY_ENCODER_SW_PIN);

  if (readSwitchState != lastSwitchState)
  {
    if (readSwitchState == HIGH)
    {
      switchState = !switchState;
      Serial.println("Button toggled");
    }
    lastSwitchState = readSwitchState;
  }

  Serial.println("Button state: " + String(switchState));
  colorWipe(strip.Color(255, 0, 0), 50);    // Red
  colorWipe(strip.Color(0, 255, 0), 50);    // Green
  colorWipe(strip.Color(0, 0, 255), 50);    // Blue
  colorWipe(strip.Color(0, 0, 0, 255), 50); // White

  Serial.print("Humidity:    ");
  Serial.print(si.readHumidity(), 2);
  Serial.print("\tTemperature: ");
  Serial.println(si.readTemperature(), 2);

  rainbowFade2White(3, 3, 1);
}

void colorWipe(uint32_t c, uint8_t wait)
{
  for (uint16_t i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbowFade2White(uint8_t wait, int rainbowLoops, int whiteLoops)
{
  float fadeMax = 100.0;
  int fadeVal = 0;
  uint32_t wheelVal;
  int redVal, greenVal, blueVal;

  for (int k = 0; k < rainbowLoops; k++)
  {
    for (int j = 0; j < 256; j++)
    { // 5 cycles of all colors on wheel
      for (int i = 0; i < strip.numPixels(); i++)
      {
        wheelVal = Wheel(((i * 256 / strip.numPixels()) + j) & 255);

        redVal = red(wheelVal) * float(fadeVal / fadeMax);
        greenVal = green(wheelVal) * float(fadeVal / fadeMax);
        blueVal = blue(wheelVal) * float(fadeVal / fadeMax);

        strip.setPixelColor(i, strip.Color(redVal, greenVal, blueVal));
      }

      // First loop, fade in!
      if (k == 0 && fadeVal < fadeMax - 1)
      {
        fadeVal++;
      }
      // Last loop, fade out!
      else if (k == rainbowLoops - 1 && j > 255 - fadeMax)
      {
        fadeVal--;
      }

      strip.show();
      delay(wait);
    }
  }
}

uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3, 0);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0, 0);
}

uint8_t red(uint32_t c)
{
  return (c >> 8);
}
uint8_t green(uint32_t c)
{
  return (c >> 16);
}
uint8_t blue(uint32_t c)
{
  return (c);
}
