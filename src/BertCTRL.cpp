#include "Particle.h"
#include <Wire.h>
#include "I2CScanner.h"
#include "wifi_creds.h"
#include "SetupWifi.h"
#include "ThingsboardClient.h"
#include "BertUtils.h"
#include "OneWire.h"
#include "spark-dallas-temperature.h"
#include <multi_channel_relay.h>

/* ======================= Init ================================================ */
I2CScanner scanner;
#define RELAY_ADDRESS 0x11
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
#define DS18B20_PIN D2 // Define the pin where the DS18B20 is connected

OneWire oneWire(DS18B20_PIN); // Create a OneWire instance
DallasTemperature tempSensors(&oneWire);
int deviceCount = 0;
float tempC;
/* ============================================================================== */

/* ======================= Relays ============================================= */
Multi_Channel_Relay relay;
/* ============================================================================== */

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

  /* ======================= Dallas Temperature Sensor ========================= */
  Serial.println("Dallas Temperature IC test");
  tempSensors.begin();
  if (tempSensors.getDeviceCount() == 0)
  {
    Serial.println("No DS18B20 sensors are connected!");
    while (1)
      ; // Halt execution if no sensors are found
  }
  else
  {
    Serial.println("Dallas Temperature IC Control Library");
    // locate devices on the bus
    Serial.print("Locating devices...");
    Serial.print("Found ");
    Serial.print(tempSensors.getDeviceCount(), DEC);
    Serial.println(" devices.");
    Serial.println("");
  }
  /* =========================================================================== */

  // /* ======================= Particle Functions ================================ */
  // Particle.function("setSensorState", setSensorState); // Register the cloud function
  // Particle.variable("temperature", currentTemperature);
  // Particle.variable("humidity", currentHumidity);
  // Particle.variable("sensorState", sensorStateString);
  // /* =========================================================================== */

  /* ======================= Multi Channel Relay Test =============================== */
  // Set I2C address and start relay
  relay.begin(RELAY_ADDRESS);

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
  /* =========================================================================== */
}

void loop()
{
  tempSensors.requestTemperatures();
  /* ======================= API Thingsboard =================================== */
  if (millis() - lastSendTime >= sendInterval)
  {
    // Display temperature from each sensor
    for (int i = 0; i < tempSensors.getDeviceCount(); i++)
    {
      tempC = tempSensors.getTempCByIndex(i);
      Serial.println(tempC);
    }
    // Send data to ThingsBoard
    sendDataToThingsBoard(tempSensors.getTempCByIndex(0), tempSensors.getTempCByIndex(1), tempSensors.getTempCByIndex(2));

    // Update the last send time
    lastSendTime = millis();
  }
  /* =========================================================================== */
}