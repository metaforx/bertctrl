/* ======================= Thingsboard =============================== */
#include "ThingsboardClient.h"
#include "Particle.h"

const bool ThingsboardClientDebug = false;

// HTTP request and response
http_request_t request;
http_response_t response;
http_header_t headers[] = {
    {"Content-Type", "application/json"},
    {NULL, NULL} // Terminate the headers array
};

void sendDataToThingsBoard(float temperature, float humidity)
{
    // Set the request details
    request.hostname = thingsBoardServer;
    request.port = thingsBoardPort;
    request.path = String("/api/v1/") + accessToken + "/telemetry"; // Construct the request path

    // Create the JSON payload
    String payload = "{\"temperature\":" + String(temperature) + ",\"humidity\":" + String(humidity) + "}";

    // Set the request body
    request.body = payload;

    // Send the POST request
    http.post(request, response, headers);

    if (ThingsboardClientDebug)
    {

        if (response.status == 200)
        {
            Serial.println("Data sent successfully");
            Serial.println("Payload: " + payload);
        }
        else
        {
            Serial.println("Failed to send data");
            Serial.println("Payload: " + payload);
        }

        // Print request details
        Serial.println("Request URL: " + String(request.hostname) + request.path);
        Serial.println("Request Headers:");
        for (int i = 0; headers[i].header != NULL; i++)
        {
            Serial.println(String(headers[i].header) + ": " + String(headers[i].value));
        }
        Serial.println("Request Body: " + request.body);
    }
}
/* =================================================================== */