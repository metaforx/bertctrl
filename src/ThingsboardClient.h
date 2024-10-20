#ifndef THINGSBOARD_CLIENT_H
#define THINGSBOARD_CLIENT_H

#include "Particle.h"
#include "HttpClient.h"

void sendDataToThingsBoard(float temperature, float humidity);
extern const bool ThingsboardClientDebug;
extern const char *accessToken;
extern const char *thingsBoardServer;
extern const int thingsBoardPort;
extern HttpClient http;

#endif // THINGSBOARD_CLIENT_H