// wifi_creds.cpp
#include "wifi_creds.h"
#include <cstdlib>    // For getenv
#include "Particle.h" // Include Particle header for WPA2 and WLAN_CIPHER_AES

credentials wifiCreds[NUM_WIFI_CREDS]; // Adjust the size as needed

void initWifiCredentials()
{
    wifiCreds[0].ssid = "SSID";
    wifiCreds[0].password = "PASSWORD";
    wifiCreds[0].authType = WLAN_SEC_WPA2; // Use WLAN_SEC_WPA2 for WPA2
    wifiCreds[0].cipher = WLAN_CIPHER_AES; // Assuming AES for simplicity
}