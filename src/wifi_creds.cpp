// wifi_creds.cpp
#include "wifi_creds.h"
#include "Particle.h" // Include Particle header for WPA2 and WLAN_CIPHER_AES
#include "secrets.h" // Include secrets.h for any additional credentials or settings
credentials wifiCreds[NUM_WIFI_CREDS];

void initWifiCredentials()
{
    wifiCreds[0].ssid = WIFI_SSID;
    wifiCreds[0].password = WIFI_PASSWORD;
    wifiCreds[0].authType = WLAN_SEC_WPA2; // Use WLAN_SEC_WPA2 for WPA2
    wifiCreds[0].cipher = WLAN_CIPHER_AES; // Assuming AES for simplicity
}