// Connect Vin to 3-5VDC
// Connect GND to ground
// Connect SCL to I2C clock pin (D1 on Photon/Electron)
// Connect SDA to I2C data pin  (D0 on Photon/Electron)


#include "Adafruit_Si7021.h"   // Use for Build IDE
// #include "Adafruit_Si7021.h"               // Use for local build

Adafruit_Si7021 sensor = Adafruit_Si7021();

void setup() {
  Serial.begin(115200);
  Serial.println("Si7021 test");
  sensor.begin();
}

void loop() {
  Serial.print("Humidity:    "); Serial.print(sensor.readHumidity(), 2);
  Serial.print("\tTemperature: "); Serial.println(sensor.readTemperature(), 2);
  delay(100);
}
