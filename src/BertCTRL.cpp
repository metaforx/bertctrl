#include "Particle.h"
#include "secrets.h"
#include <Wire.h>
#include "I2CScanner.h"
#include "wifi_creds.h"
#include "SetupWifi.h"
#include "BertUtils.h" // can most likely be removed later
#include "OneWire.h"
#include "spark-dallas-temperature.h"
#include <multi_channel_relay.h>
#include <vector>

/* ======================= Init ================================================ */
I2CScanner scanner;
#define RELAY_ADDRESS 0x11
/* ============================================================================== */

/* ======================= System & Wifi ======================================= */
SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);
/* ============================================================================== */

/* ======================= ThingsboardClient ==================================== */
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 60000 * 15; // 15 minutes in milliseconds
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

// Define the TimeInterval struct
struct TimeInterval {
    int startHour;
    int startMinute;
    int endHour;
    int endMinute;
};

// Define on/off intervals for each channel
std::vector<TimeInterval> relayTimes[3] = {
    {{21, 22, 22, 45}, {23, 0, 23, 60}, {0, 0, 12, 0}}, // CHHANNEL 1: divide packages when steppig over midnight
    {{21, 22, 22, 45}}, // CHHANNEL 2
    {{21, 22, 22, 45}, {22, 55, 23, 00}} // CHHANNEL 3
};
/* =========================================================================== */

/* ======================= Temperature Thresholds ============================ */
const float TEMP_THRESHOLD_HIGH = 25.0;  // High temperature threshold
const float TEMP_THRESHOLD_LOW = 18.0;   // Low temperature threshold
const int TEMP_SENSOR_LOW = 0; // Index of the low temperature sensor
const int TEMP_SENSOR_HIGH = 1; // Index of the high temperature sensor
/* ============================================================================ */


/* ======================= Helper Function ==================================== */
bool isTimeBetween(int currentMinutes, int startHour, int startMinute, int endHour, int endMinute) {
    int startMinutes = startHour * 60 + startMinute;
    int endMinutes = endHour * 60 + endMinute;
    int currentTime = currentMinutes;
    
    return currentTime >= startMinutes && currentTime < endMinutes;
}
/* =========================================================================== */


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

  /* ======================= Multi Channel Relay Test =============================== */
  // Set I2C address and start relay
  relay.begin(RELAY_ADDRESS);
  Time.zone(+2);

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
  int currentHour = Time.hour();
  int currentMinute = Time.minute();
  int currentMinutes = currentHour * 60 + currentMinute;

  tempSensors.requestTemperatures();
  char data[128];
  String deviceId = Particle.deviceID();

  Serial.printlnf("Current time: %d:%d", currentHour, currentMinute);


  // Iterate through each channel
  int activeChannels[] = {2,3,4};
  for (size_t i = 0; i < sizeof(activeChannels) / sizeof(activeChannels[0]); i++) {
    int channel = activeChannels[i]; // Get the current channel
    bool channelOn = false;

    // Check each time interval for the current channel
    for (const auto& timeSlot : relayTimes[channel - 1]) { // Adjust for 0-based indexing in relayTimes
        if (isTimeBetween(currentMinutes, timeSlot.startHour, timeSlot.startMinute, timeSlot.endHour, timeSlot.endMinute)) {
            channelOn = true;
            break;
        }
    }

    // Only check temperature thresholds for channel 1
    // if (channel == 1) {
    //     if (tempSensors.getTempCByIndex(TEMP_SENSOR_HIGH) > TEMP_THRESHOLD_HIGH ||
    //         tempSensors.getTempCByIndex(TEMP_SENSOR_LOW) > TEMP_THRESHOLD_LOW) {
    //         channelOn = false; // Turn off the channel if temperature is out of bounds
    //     }
    // }

    channelOn ? relay.turn_on_channel(channel) : relay.turn_off_channel(channel);
  }

  /* ======================= SEND DATA =================================== */
  if (millis() - lastSendTime >= sendInterval)
  {
    // Display temperature from each sensor
    for (int i = 0; i < tempSensors.getDeviceCount(); i++)
    {
      tempC = tempSensors.getTempCByIndex(i);
      Serial.println(tempC);

      // Prepare JSON payload as a string
      snprintf(data, sizeof(data),
        "{\"value\":%.2f,\"sensor_id\":\"%s\",\"sensor_name\":\"%s\",\"sensor_type\":\"%s\"}",
        tempC,
        deviceId.c_str(), 
        "bertctl",  
        "temperature"  
      );

      // // Log the payload to serial
      Serial.println("Publishing JSON payload:");
      Serial.println(data);

      // Publish the JSON string for the webhook
      Particle.publish("jardin-data", data, PRIVATE);
    }

    // Update the last send time
    lastSendTime = millis();
  }
  /* =========================================================================== */
}

// 3 purple, 2 red, 1 yellow