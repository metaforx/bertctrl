#include "Particle.h"
#include "secrets.h"
#include <Wire.h>
#include "I2CScanner.h"
#include "wifi_creds.h"
#include "SetupWifi.h"
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

/* ======================= API publication ====================================== */
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 60000 * 5; // 5 minutes in milliseconds
/* ============================================================================== */

/* ======================= DS18B20 ============================================== */
#define DS18B20_PIN D2 // Define the pin where the DS18B20 is connected
OneWire oneWire(DS18B20_PIN); // Create a OneWire instance
DallasTemperature tempSensors(&oneWire);
int deviceCount = 0;
float tempC;
/* ============================================================================== */

/* ======================= Relays ============================================== */
// 2 red
// 3 purple
// 4 green
Multi_Channel_Relay relay;
const bool TEST_RELAYS_ON_SETUP = true;
const int ACTIVE_CHANNELS[] = {2,3,4};
// Relay channel assignments
const int CHANNEL_HEATLAMP = 2; // Channel for the heatlamp (red)

// Define the TimeInterval struct
struct TimeInterval {
    int startHour;
    int startMinute;
    int endHour;
    int endMinute;
};

// Define on/off intervals for each channel
// Note: divide time packages when stepping over midnight, eg. {{23, 0, 23, 59}, {0, 0, 10, 59}}
std::vector<TimeInterval> relayTimes[3] = {
    {{9, 30, 11, 30}, {13, 30, 14, 30}, {17, 30, 18, 30}},
    {{7, 30, 20, 30}},
    {{7, 30, 20, 30}}
};
/* =========================================================================== */

/* ======================= Temperature Thresholds ============================ */
// Channels per color:
// 0 yellow
// 1 red
// 2 purple
const char* SENSOR_COLORS[] = {"yellow", "red", "purple"};
const float TEMP_THRESHOLD_HEATLAMP_HIGH = 50.0;  // High temperature threshold of heatlamp sensor
const float TEMP_THRESHOLD_SHADOW_HIGH = 28.0;   // High temperature threshold of shadow sensor
const float TEMP_THRESHOLD_SHADOW_LOW = 17.0;   // Low temperature threshold of shadow sensor

// sensor assignments
const int TEMP_SENSOR_SHADOW = 0; // Index of the low temperature sensor  [yellow]
const int TEMP_SENSOR_HEATLAMP = 1; // Index of the high temperature sensor [red]


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

  /* ======================= Timezone Setup ==================================== */
  Time.zone(+2);
  /* =========================================================================== */

  /* ======================= Multi Channel Relay Setup & Test ================== */
  // Set I2C address and start relay
  relay.begin(RELAY_ADDRESS);

  /* Begin Controlling Relay */
  if (TEST_RELAYS_ON_SETUP) {
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
  }
  /* =========================================================================== */
}

void loop()
{
  int currentHour = Time.hour();
  int currentMinute = Time.minute();
  int currentMinutes = currentHour * 60 + currentMinute;

  tempSensors.requestTemperatures();
  char data[256];
  String deviceId = Particle.deviceID();

  Serial.printlnf("Current time: %d:%d", currentHour, currentMinute);


  // Iterate through each active channel
  for (size_t i = 0; i < sizeof(ACTIVE_CHANNELS) / sizeof(ACTIVE_CHANNELS[0]); i++) {
    int channel = ACTIVE_CHANNELS[i]; // Get the current channel
    bool channelOn = false;
    
    // Check each time interval for the current channel
    for (const auto& timeSlot : relayTimes[i]) {

      //check times slotes per channel
      if (isTimeBetween(currentMinutes, timeSlot.startHour, timeSlot.startMinute, timeSlot.endHour, timeSlot.endMinute)) {

        // check temperature confditions only for heat lamp
        if (channel == CHANNEL_HEATLAMP)  {

          // Check if the shadow sensor is above the low threshold, otherwise turn the lamp on
          if (tempSensors.getTempCByIndex(TEMP_SENSOR_SHADOW) > TEMP_THRESHOLD_SHADOW_LOW) {

            //if the temperature is above the high threshold (either of shadow or heat lamp), turn off the lamp
            if ((tempSensors.getTempCByIndex(TEMP_SENSOR_HEATLAMP) > TEMP_THRESHOLD_HEATLAMP_HIGH) ||
                (tempSensors.getTempCByIndex(TEMP_SENSOR_SHADOW) > TEMP_THRESHOLD_SHADOW_HIGH)) {
              channelOn = false; // Turn off the channel if temperature is out of bounds
              break;
            }
          }
        }
        channelOn = true;
        break;
      }
    }
    channelOn ? relay.turn_on_channel(channel) : relay.turn_off_channel(channel);
  }

  /* ======================= SEND DATA =================================== */
  if (millis() - lastSendTime >= sendInterval)
  {
    // Display temperature from each sensor
    for (int i = 0; i < tempSensors.getDeviceCount(); i++)
    {
      tempC = tempSensors.getTempCByIndex(i);
      char sensorId[64];
      snprintf(sensorId, sizeof(sensorId), "%s-%d-%s", deviceId.c_str(), i, SENSOR_COLORS[i]);

      // Prepare JSON payload as a string
      snprintf(data, sizeof(data),
          "{\"value\":%.2f,\"device_id\":\"%s\",\"sensor_id\":\"%s\",\"sensor_name\":\"%s\",\"sensor_type\":\"%s\"}",
          tempC,
          deviceId.c_str(),
          sensorId,
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

