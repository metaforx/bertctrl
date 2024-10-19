#include "Particle.h"
#include <Wire.h>
#include "Adafruit_Si7021.h"
#include "I2CScanner.h"
#include "neopixel.h"
#include "wifi_creds.h"

// // Define the array with a specific size in the .cpp file
// credentials wifiCreds[1]; // Adjust the size as needed

Adafruit_Si7021 si = Adafruit_Si7021();
I2CScanner scanner;

const int MAX_TEMPERATURE = 50;
const int MIN_TEMPERATURE = 0;
const int MAX_HUMIDITY = 100;
const int MIN_HUMIDITY = 20;
double currentTemperature = 0.0;
double currentHumidity = 0.0;

/* ======================= prototypes =============================== */
// uint32_t Wheel(byte WheelPos);
uint8_t red(uint32_t c);
uint8_t green(uint32_t c);
uint8_t blue(uint32_t c);
void colorWipe(uint32_t c, uint8_t wait);
uint32_t interpolateColor(uint32_t color1, uint32_t color2, float fraction);

// NEOPIXEL: Set pixel COUNT, PIN and TYPE
#define PIXEL_PIN D2
#define PIXEL_COUNT 16
#define PIXEL_TYPE WS2812B
#define BRIGHTNESS 125 // 0 - 255
#define FADE_STEPS 50  // Number of steps for the fade-in animation
#define FADE_DELAY 2   // Delay between each fade step (in milliseconds)

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
  temperature = clamp(temperature, MIN_TEMPERATURE, MAX_TEMPERATURE);
  int mappedValue = static_cast<int>(mapFloat(temperature, MIN_TEMPERATURE, MAX_TEMPERATURE, 0, strip.numPixels() - 1));
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" -> Mapped Temperature LED: ");
  Serial.println(mappedValue);
  return mappedValue;
}

int mapHumidityToLED(float humidity)
{
  humidity = clamp(humidity, MIN_HUMIDITY, MAX_HUMIDITY);
  int mappedValue = static_cast<int>(mapFloat(humidity, MIN_HUMIDITY, MAX_HUMIDITY, 0, strip.numPixels() - 1));
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" -> Mapped Humidity LED: ");
  Serial.println(mappedValue);
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

void clearLEDs();
void fadeInPixel(int pixel, uint32_t color); // Function prototype
void blinkLED(int pixel, uint32_t color, int times, int delayTime);

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
  // Check temperature conditions and set LED colors
  if (currentTemperature < MIN_TEMPERATURE)
  {
    blinkLED(0, strip.Color(255, 0, 0), 3, 500); // Blink red on pixel 0
  }
  else if (currentTemperature > MAX_TEMPERATURE)
  {
    blinkLED(15, strip.Color(255, 0, 0), 3, 500); // Blink red on pixel 15
  }
  else if (currentTemperature < MIN_TEMPERATURE + 3)
  {
    blinkLED(0, strip.Color(255, 165, 0), 3, 500); // Blink orange on pixel 0
  }
  else if (currentTemperature > MAX_TEMPERATURE - 3)
  {
    blinkLED(15, strip.Color(255, 165, 0), 3, 500); // Blink orange on pixel 15
  }
  else
  {
    // Normal operation: set LED colors based on state
    switch (sensorState)
    {
    case TEMPERATURE:
      for (int i = 0; i <= temperatureLED; i++)
      {
        float fraction = (float)i / strip.numPixels();
        uint32_t color1 = 0x0000FF; // Blue
        uint32_t color2 = 0x00FF00; // Green
        uint32_t color = interpolateColor(color1, color2, fraction);
        fadeInPixel(i, color);
      }
      break;
    case HUMIDITY:
      for (int i = 0; i <= humidityLED; i++)
      {
        float fraction = (float)i / strip.numPixels();
        uint32_t color1 = 0x00FF00; // Green
        uint32_t color2 = 0x0000FF; // Blue
        uint32_t color = interpolateColor(color1, color2, fraction);
        fadeInPixel(i, color);
      }
      break;
    }
    strip.show();
  }
}

void fadeInPixel(int pixel, uint32_t color)
{
  uint8_t r = (color >> 16) & 0xFF;
  uint8_t g = (color >> 8) & 0xFF;
  uint8_t b = color & 0xFF;

  for (int step = 0; step <= FADE_STEPS; step++)
  {
    float fraction = (float)step / FADE_STEPS;
    uint8_t rStep = r * fraction;
    uint8_t gStep = g * fraction;
    uint8_t bStep = b * fraction;
    strip.setPixelColor(pixel, strip.Color(rStep, gStep, bStep));
    strip.show();
    delay(FADE_DELAY);
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
SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);
// SYSTEM_MODE(AUTOMATIC);
// SYSTEM_THREAD(ENABLED);

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

  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show();
  Particle.function("setSensorState", setSensorState); // Register the cloud function
  Particle.variable("temperature", currentTemperature);
  Particle.variable("humidity", currentHumidity);
  Particle.variable("sensorState", sensorStateString);
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
    clearLEDs();
    setLEDColorBasedOnState(sensorState, temperatureLED, humidityLED);
    previousSensorState = sensorState;
    previousTemperatureLED = temperatureLED;
    previousHumidityLED = humidityLED;
  }
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
