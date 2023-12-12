#include <esp_http_client.h>
#include "Json.h"

typedef struct {
    int temperature;
    int humidity;
    // Add other parameters as needed
} TelemetryData;

void send_telemetry_data();