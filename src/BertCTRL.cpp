#include "Particle.h"
#include <Wire.h>
#include "Adafruit_Si7021.h"
#include "I2CScanner.h"
#include "neopixel.h"
#include "wifi_creds.h"
#include "ThingsboardClient.h"
#include "NeopixelControls.h"

/* ======================= ThingsboardClient =============================== */
HttpClient http;
const char *thingsBoardServer = "192.168.1.153";
const char *accessToken = "2BodYeWy0G5o82nLrxUk";
const int thingsBoardPort = 8080;
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 5000; // 5 seconds in milliseconds
/* ======================= ThingsboardClient =============================== */

/* ======================= NeopixelControls =============================== */
const int fadeSteps = 50;
const int fadeDelay = 2;
/* ======================= NeopixelControls =============================== */

Adafruit_Si7021 si = Adafruit_Si7021();
I2CScanner scanner;

const int MAX_MAP_TEMPERATURE = 50;
const int MIN_MAP_TEMPERATURE = 0;
const int MAX_TEMPERATURE = 43;
const int MIN_TEMPERATURE = 15;
const int MAX_MAP_HUMIDITY = 100;
const int MIN_MAP_HUMIDITY = 0;
const int MAX_HUMIDITY = 70;
const int MIN_HUMIDITY = 20;
double currentTemperature = 0.0;
double currentHumidity = 0.0;

// custom utils
bool isBelowMin(double value, double min);
bool isAboveMax(double value, double min);
uint32_t ALERT_COLOR = 0xFF0000; // Red

/* ======================= prototypes =============================== */
uint32_t interpolateColor(uint32_t color1, uint32_t color2, float fraction);

// NEOPIXEL: Set pixel COUNT, PIN and TYPE
#define PIXEL_PIN D2
#define PIXEL_COUNT 16
#define PIXEL_TYPE WS2812B
#define BRIGHTNESS 125 // 0 - 255

uint32_t currentPixelColors[PIXEL_COUNT];
uint32_t temperatureDisplayColors[PIXEL_COUNT];
Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

float clamp(float value, float min, float max)
{
  if (value < min)
    return min;
  if (value > max)
    return max;
  return value;
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int mapTemperatureToLED(float temperature)
{
  temperature = clamp(temperature, MIN_MAP_TEMPERATURE, MAX_MAP_TEMPERATURE);
  int mappedValue = static_cast<int>(mapFloat(temperature, MIN_MAP_TEMPERATURE, MAX_MAP_TEMPERATURE, 0, strip.numPixels() - 1));
  return mappedValue;
}

int mapHumidityToLED(float humidity)
{
  humidity = clamp(humidity, MIN_MAP_HUMIDITY, MAX_MAP_HUMIDITY);
  int mappedValue = static_cast<int>(mapFloat(humidity, MIN_MAP_HUMIDITY, MAX_MAP_HUMIDITY, 0, strip.numPixels() - 1));
  return mappedValue;
}

enum SensorState
{
  TEMPERATURE,
  HUMIDITY,
};

volatile SensorState sensorState = TEMPERATURE; // Use volatile for variables shared with ISR
SensorState previousSensorState = TEMPERATURE;  // Track the previous state
int previousTemperatureLED = -1;                // Track the previous temperatureLED value
int previousHumidityLED = -1;                   // Track the previous humidityLED value

String sensorStateString = "TEMPERATURE";
int setSensorState(String command)
{
  int state = command.toInt(); // Convert the String argument to an integer
  switch (state)
  {
  case 0:
    sensorState = TEMPERATURE;
    sensorStateString = "TEMPERATURE";
    break;
  case 1:
    sensorState = HUMIDITY;
    sensorStateString = "HUMIDITY";
    break;
  default:
    return -1; // Invalid state
  }
  return 1; // Success
}

void setLEDColorBasedOnState(SensorState sensorState, int temperatureLED, int humidityLED)
{
  // Normal operation: set LED colors based on state
  switch (sensorState)
  {
  case TEMPERATURE:
    for (int i = 0; i <= temperatureLED; i++)
    {
      // float fraction = (float)i / strip.numPixels();

      // uint32_t color1 = 0x0000FF; // Blue
      // uint32_t color2 = 0x00FF00; // Green
      // uint32_t color = interpolateColor(color1, color2, fraction);
      if (isBelowMin(currentTemperature, MIN_TEMPERATURE + 5) && i == temperatureLED)
      {
        fadeInPixel(i, ALERT_COLOR);
      }
      else if (isAboveMax(currentTemperature, MAX_TEMPERATURE - 5) && i == temperatureLED)
      {
        fadeInPixel(i, ALERT_COLOR);
      }
      else
      {
        fadeInPixel(i, temperatureDisplayColors[i]);
      }
    }
    for (int i = strip.numPixels(); i > temperatureLED; i--)
    {
      fadeOutPixel(i);
    }
    break;
  case HUMIDITY:
    for (int i = 0; i <= humidityLED; i++)
    {
      float fraction = (float)i / strip.numPixels();
      uint32_t color1 = 0x00FF00; // Green
      uint32_t color2 = 0x0000FF; // Blue
      uint32_t color = interpolateColor(color1, color2, fraction);
      if (isBelowMin(currentHumidity, MIN_HUMIDITY + 5) && i == humidityLED)
      {
        fadeInPixel(i, ALERT_COLOR);
      }
      else if (isAboveMax(currentHumidity, MAX_HUMIDITY - 5) && i == humidityLED)
      {
        fadeInPixel(i, ALERT_COLOR);
      }
      else
      {
        fadeInPixel(i, color);
      }
    }
    for (int i = strip.numPixels(); i > humidityLED; i--)
    {

      fadeOutPixel(i);
    }
    break;
  }
  strip.show();
}

SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

void setupWifi()
{
  credentials creds;
  WiFi.on();
  WiFi.disconnect();
  WiFi.clearCredentials();

  initWifiCredentials(); // Initialize credentials from environment variables

  for (int i = 0; i < NUM_WIFI_CREDS; i++)
  {
    creds = wifiCreds[i];
    if (creds.ssid && creds.password)
    {
      WiFi.setCredentials(creds.ssid, creds.password, creds.authType, creds.cipher);
    }
  }

  WiFi.connect();
  while (!WiFi.ready())
  {
    Serial.println("Connecting to WiFi...");
    Serial.printlnf("Connecting to SSID: %s with Password: %s", creds.ssid, creds.password);
    delay(1000);
  }
  Serial.println("Connected to WiFi");
  Particle.connect();
}

int setHumidity(String command)
{
  currentHumidity = command.toFloat();
  return 1; // Success
}

int setTemperature(String command)
{
  currentTemperature = command.toFloat();
  return 1; // Success
}

void setup()
{
  Serial.begin(9600);

  setupWifi();

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

  // COLORS /// INTERPOLATION
  uint32_t blue = 0x0000FF;   // Blue
  uint32_t green = 0x00FF00;  // Green
  uint32_t purple = 0x800080; // Purple
  uint32_t red = 0xFF0000;    // Red

  // Number of steps per segment
  uint32_t segmentSteps = strip.numPixels() / 3;
  uint32_t remainingSteps = strip.numPixels() % 3;

  // Precompute the colors in the array
  for (uint32_t step = 0; step < strip.numPixels(); step++)
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
  //////////////////////////

  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  for (int i = 0; i < PIXEL_COUNT; i++)
  {
    currentPixelColors[i] = strip.getPixelColor(i);
  }
  strip.show();
  Particle.function("setSensorState", setSensorState); // Register the cloud function
  Particle.variable("temperature", currentTemperature);
  Particle.variable("humidity", currentHumidity);
  Particle.variable("sensorState", sensorStateString);

  // Testing

  // Register Particle functions
  // Particle.function("setHumidity", setHumidity);
  // Particle.function("setTemperature", setTemperature);
}

void loop()
{
  // si7021 - Get humidity and temperature data
  currentHumidity = si.readHumidity();
  currentTemperature = si.readTemperature();
  int temperatureLED = mapTemperatureToLED(currentTemperature);
  int humidityLED = mapHumidityToLED(currentHumidity);

  // Check if the sensor state or LED values have changed
  if (sensorState != previousSensorState || temperatureLED != previousTemperatureLED || humidityLED != previousHumidityLED)
  {
    // clearLEDs();
    setLEDColorBasedOnState(sensorState, temperatureLED, humidityLED);
    previousSensorState = sensorState;
    previousTemperatureLED = temperatureLED;
    previousHumidityLED = humidityLED;
  }

  // Check if 30 seconds have passed since the last send
  if (millis() - lastSendTime >= sendInterval)
  {
    // Send data to ThingsBoard
    sendDataToThingsBoard(currentTemperature, currentHumidity);

    // Update the last send time
    lastSendTime = millis();
  }
}

// Option 2: Separate Functions
bool isBelowMin(double value, double min)
{
  return value < min;
}

bool isAboveMax(double value, double max)
{
  return value > max;
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
