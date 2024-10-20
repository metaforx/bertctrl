// wifi_creds.h
#ifndef WIFI_CREDS_H
#define WIFI_CREDS_H

struct credentials
{
    const char *ssid;
    const char *password;
    int authType;
    int cipher;
};

#define NUM_WIFI_CREDS 1
extern credentials wifiCreds[NUM_WIFI_CREDS];
void initWifiCredentials();

#endif // WIFI_CREDS_H