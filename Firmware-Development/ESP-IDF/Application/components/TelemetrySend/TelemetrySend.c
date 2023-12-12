#include <esp_http_client.h>
#include "TelemetrySend.h"
#include "esp_log.h"


// Define the server endpoint URL
#define SERVER_URL "http://192.168.18.131:8080/"
#define TAG "TelemetrySend" // Change this tag to suit your needs

// Telemetry data structure (customize based on your needs)

// Function to send telemetry data to the server using a POST request
// Function to send telemetry data to the server using a POST request
void send_telemetry_data()
{
    esp_http_client_config_t config = {
        .url = SERVER_URL,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");

    const int value = 100;
    
    char myCharBuffer[10]; // Make sure the buffer is large enough

    // Convert integer to const char* using itoa
    itoa(value, myCharBuffer, 10);
   
    // Create JSON data
    const char *jsonData = createJson("model", "gpt-3.5-turbo-instruct", "name" , "Moajiz" , "value" , myCharBuffer ,
                                             NULL); 
    printf("str len of the jsonData %d\n ",strlen(jsonData));
    esp_http_client_set_post_field(client, jsonData, strlen(jsonData));
    printf("stucking while\n");
    // Perform the HTTP POST request
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Telemetry data sent successfully");
    }
    else
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    // Clean up the HTTP client and free allocated memory for JSON data
    esp_http_client_cleanup(client);
    free(jsonData);
}
