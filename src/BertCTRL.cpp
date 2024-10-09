// Example usage for Adafruit_SGP30 library.
// This library was modified/wrapped by SJB (https://github.com/dyadica)
// in order to work with Particle Photon & Core.

#include "Particle.h"
#include <Wire.h>
#include "Adafruit_SGP30.h"
#include "Adafruit_Si7021.h"
#include "I2CScanner.h"

Adafruit_Si7021 si = Adafruit_Si7021();
Adafruit_SGP30 sgp;
I2CScanner scanner;

void setup()
{
  Serial.begin(9600);
  Serial.println("SGP30 test");
  Serial.println("Si7021 test");

  if (!sgp.begin())
  {
    Serial.println("Sensor not found :(");
    while (1)
      ;
  }
  if (!si.begin())
  {
    Serial.println("Did not find Si7021 sensor!");
    while (1)
      ;
  }

  Serial.print("Found SGP30 serial #");
  Serial.print(sgp.serialnumber[0], HEX);
  Serial.print(sgp.serialnumber[1], HEX);
  Serial.println(sgp.serialnumber[2], HEX);

  if (scanner.begin())
  {
    Serial.println("I2C bus initialized successfully.");
    scanner.scan();
  }
  else
  {
    Serial.println("Failed to initialize I2C bus.");
  }

  // If you have a baseline measurement from before you can assign it to start, to 'self-calibrate'
  // sgp.setIAQBaseline(0x8E68, 0x8F41);  // Will vary for each sensor!
}

int counter = 0;
void loop()
{

  if (!sgp.IAQmeasure())
  {
    Serial.println("Measurement failed");
    return;
  }
  Serial.print("TVOC ");
  Serial.print(sgp.TVOC);
  Serial.print(" ppb\t");
  Serial.print("eCO2 ");
  Serial.print(sgp.eCO2);
  Serial.println(" ppm");
  Serial.print("Humidity:    ");
  Serial.print(si.readHumidity(), 2);
  Serial.print("\tTemperature: ");
  Serial.println(si.readTemperature(), 2);
  delay(100);
  delay(1000);

  counter++;
  if (counter == 30)
  {
    counter = 0;

    uint16_t TVOC_base, eCO2_base;
    if (!sgp.getIAQBaseline(&eCO2_base, &TVOC_base))
    {
      Serial.println("Failed to get baseline readings");
      return;
    }
    Serial.print("****Baseline values: eCO2: 0x");
    Serial.print(eCO2_base, HEX);
    Serial.print(" & TVOC: 0x");
    Serial.println(TVOC_base, HEX);
  }
}

// #include "Particle.h"
// #include "Wire.h"

// void setup() {
//     Serial.begin(9600);
//     delay(1000); // Add a delay to ensure the serial connection is initialized
//     Serial.println("I2C Scanner");

//     Wire.begin();
// }

// void loop() {
//     byte error, address;
//     int nDevices;

//     Serial.println("Scanning...");

//     nDevices = 0;
//     for (address = 1; address < 127; address++) {
//         // The i2c_scanner uses the return value of
//         // the Write.endTransmission to see if
//         // a device did acknowledge to the address.
//         Wire.beginTransmission(address);
//         error = Wire.endTransmission();

//         if (error == 0) {
//             Serial.print("I2C device found at address 0x");
//             if (address < 16) {
//                 Serial.print("0");
//             }
//             Serial.print(address, HEX);
//             Serial.println("  !");

//             nDevices++;
//         } else if (error == 4) {
//             Serial.print("Unknown error at address 0x");
//             if (address < 16) {
//                 Serial.print("0");
//             }
//             Serial.println(address, HEX);
//         }
//     }
//     if (nDevices == 0) {
//         Serial.println("No I2C devices found\n");
//     } else {
//         Serial.println("done\n");
//     }

//     delay(5000); // Wait 5 seconds for the next scan
// }