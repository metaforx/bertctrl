// wifi_creds.h
#ifndef WIFI_CREDS_H
#define WIFI_CREDS_H

// Define the credentials structure
struct credentials
{
    const char *ssid;
    const char *password;
    int authType;
    int cipher;
};

// Define a constant for the number of WiFi credentials
#define NUM_WIFI_CREDS 1 // You can adjust this number as needed

// Declare the wifiCreds array with external linkage
extern credentials wifiCreds[NUM_WIFI_CREDS];

// Function to initialize credentials
void initWifiCredentials();

#endif // WIFI_CREDS_H