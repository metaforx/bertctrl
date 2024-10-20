#include "SetupWifi.h"

void setupWifi()
{
    credentials creds;
    WiFi.on();
    WiFi.disconnect();
    WiFi.clearCredentials();

    initWifiCredentials();

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