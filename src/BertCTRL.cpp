#include "Particle.h"
#include <Wire.h>
#include "Adafruit_Si7021.h"
#include "I2CScanner.h"
#include "neopixel.h"
#include "wifi_creds.h"
#include "SetupWifi.h"
#include "ThingsboardClient.h"
#include "NeopixelControls.h"
#include "BertUtils.h"
#include "OneWire.h"
#include "spark-dallas-temperature.h"
#include <multi_channel_relay.h>

/* ======================= Init ================================================ */
Adafruit_Si7021 si = Adafruit_Si7021();
I2CScanner scanner;
/* ============================================================================== */

/* ======================= System & Wifi ======================================= */
SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);
// Creds defined in wifi_creds.cpp
/* ============================================================================== */

/* ======================= ThingsboardClient ==================================== */
HttpClient http;
const char *thingsBoardServer = "192.168.1.154";
const char *accessToken = "2BodYeWy0G5o82nLrxUk";
const int thingsBoardPort = 8080;
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 60000; // 60 seconds in milliseconds
/* ============================================================================== */

/* ======================= NeopixelControls ==================================== */
const int fadeSteps = 50;
const int fadeDelay = 2;
/* ============================================================================== */

/* ======================= DS18B20 ============================================= */
#define DS18B20_PIN D3 // Define the pin where the DS18B20 is connected

OneWire oneWire(DS18B20_PIN); // Create a OneWire instance
DallasTemperature tempSens(&oneWire);
/* ============================================================================== */

/* ======================= Relays ============================================= */
Multi_Channel_Relay relay;
/* ============================================================================== */

/* ======================= TODO: clean up mess ================================= */
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
uint32_t ALERT_COLOR = 0xFF0000; // Red

// NEOPIXEL: Set pixel COUNT, PIN and TYPE
#define PIXEL_PIN D2
#define PIXEL_COUNT 16
#define PIXEL_TYPE WS2812B
#define BRIGHTNESS 125 // 0 - 255

uint32_t currentPixelColors[PIXEL_COUNT];

Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

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

/* ======================= End Mess ============================================ */

void setup()
{
  Serial.begin(9600);
  /* ======================= Wifi ============================================== */
  setupWifi();
  /* =========================================================================== */

  /* ======================= I2C Scanner ======================================= */
  if (scanner.begin())
  {
    Serial.println("I2C bus initialized successfully.");
    scanner.scan();
  }
  else
  {
    Serial.println("Failed to initialize I2C bus.");
  }

  /* ======================= Si7021 Temperature Sensor ========================= */
  Serial.println("Si7021 test");
  if (!si.begin())
  {
    Serial.println("Did not find Si7021 sensor!");
    while (1)
      ;
  }
  /* =========================================================================== */

  /* ======================= Dallas Temperature Sensor ========================= */
  Serial.println("Dallas Temperature IC test");
  tempSens.begin();
  if (tempSens.getDeviceCount() == 0)
  {
    Serial.println("No DS18B20 sensors are connected!");
    while (1)
      ; // Halt execution if no sensors are found
  }
  else
  {
    Serial.println("Dallas Temperature IC Control Library Demo");
  }
  /* =========================================================================== */

  /* ======================= Color Setup ======================================= */
  precomputeColors();
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  for (int i = 0; i < PIXEL_COUNT; i++)
  {
    currentPixelColors[i] = strip.getPixelColor(i);
  }
  strip.show();
  /* =========================================================================== */

  /* ======================= Particle Functions ================================ */
  Particle.function("setSensorState", setSensorState); // Register the cloud function
  Particle.variable("temperature", currentTemperature);
  Particle.variable("humidity", currentHumidity);
  Particle.variable("sensorState", sensorStateString);

  // Testing
  // Particle.function("setHumidity", setHumidity);
  // Particle.function("setTemperature", setTemperature);
  /* =========================================================================== */

  /* ======================= Multi Channel Relay =============================== */
  // Set I2C address and start relay
  relay.begin(0x11);

  /* Begin Controlling Relay */
  DEBUG_PRINT.println("Channel 1 on");
  relay.turn_on_channel(1);
  delay(500);
  DEBUG_PRINT.println("Channel 2 on");
  relay.turn_off_channel(1);
  relay.turn_on_channel(2);
  delay(500);
  DEBUG_PRINT.println("Channel 3 on");
  relay.turn_off_channel(2);
  relay.turn_on_channel(3);
  delay(500);
  DEBUG_PRINT.println("Channel 4 on");
  relay.turn_off_channel(3);
  relay.turn_on_channel(4);
  delay(500);
  relay.turn_off_channel(4);

  relay.channelCtrl(CHANNLE1_BIT |
                    CHANNLE2_BIT |
                    CHANNLE3_BIT |
                    CHANNLE4_BIT);
  DEBUG_PRINT.print("Turn all channels on, State: ");
  DEBUG_PRINT.println(relay.getChannelState(), BIN);

  delay(2000);

  relay.channelCtrl(CHANNLE1_BIT |
                    CHANNLE3_BIT);
  DEBUG_PRINT.print("Turn 1 3 channels on, State: ");
  DEBUG_PRINT.println(relay.getChannelState(), BIN);

  delay(2000);

  relay.channelCtrl(CHANNLE2_BIT |
                    CHANNLE4_BIT);
  DEBUG_PRINT.print("Turn 2 4 channels on, State: ");
  DEBUG_PRINT.println(relay.getChannelState(), BIN);

  delay(2000);

  relay.channelCtrl(0);
  DEBUG_PRINT.print("Turn off all channels, State: ");
  DEBUG_PRINT.println(relay.getChannelState(), BIN);

  delay(2000);
  /* =========================================================================== */
}

void loop()
{
  /* =============== Dallas Temperature Sensor - Get Sensor Data =============== */
  tempSens.requestTemperatures();
  float temperatureC = tempSens.getTempCByIndex(0);
  /* =========================================================================== */

  /* ======================= si7021 - Get Sensor Data ========================== */
  currentHumidity = si.readHumidity();
  currentTemperature = si.readTemperature();
  /* =========================================================================== */

  /* ======================= LED Display Mapped ================================ */
  int temperatureLED = mapTemperatureToLED(currentTemperature);
  int humidityLED = mapHumidityToLED(currentHumidity);

  // Check if the sensor state or LED values have changed
  if (sensorState != previousSensorState || temperatureLED != previousTemperatureLED || humidityLED != previousHumidityLED)
  {
    setLEDColorBasedOnState(sensorState, temperatureLED, humidityLED);
    previousSensorState = sensorState;
    previousTemperatureLED = temperatureLED;
    previousHumidityLED = humidityLED;
  }
  /* =========================================================================== */

  /* ======================= API Thingsboard =================================== */
  if (millis() - lastSendTime >= sendInterval)
  {
    // Send data to ThingsBoard
    sendDataToThingsBoard(currentTemperature, temperatureC, currentHumidity);

    // Update the last send time
    lastSendTime = millis();
  }
  /* =========================================================================== */
}

/* ======================= Particle Functions ================================ */
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
/* =========================================================================== */