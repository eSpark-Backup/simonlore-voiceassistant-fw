#include <esp_http_client.h>
#include "esp_spiffs.h"
#include "esp_log.h"
#include "openapi_http.h"

static const char *TAG = "openapi-http";

char buffer[MAX_HTTP_RECV_BUFFER] = {0};

void spiffs_init() 
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) 
    {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } 
        else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } 
        else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    }
}

const char *getFileName(const char *filePath) {
    // Find the position of the last '/' in the file path
    char *lastSlash = strrchr(filePath, '/');

    if (lastSlash != NULL) 
    {
        // Extract the filename with extension
        return lastSlash + 1;
    } 
    else 
    {
        // There is no '/' in the file path, so the entire path is the filename
        return filePath;
    }
}

// Event handler for HTTP events
esp_err_t esp_http_client_event_handler(esp_http_client_event_t *evt) 
{
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;

        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;

        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;

        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;

        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            printf("Received data: %.*s\n", evt->data_len, (char *)evt->data);
            break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;

        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

void send_post_request(const char* api_key, const char* file_path) 
{
    esp_http_client_config_t config = {
        .url = "https://api.openai.com/v1/audio/transcriptions",//"http://192.168.18.191:8000",
        .method = HTTP_METHOD_POST,
        .event_handler = esp_http_client_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    FILE* file = fopen(file_path, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file");
        esp_http_client_cleanup(client);
        return;
    }

    // Calculate file size
    fseek(file, 0, SEEK_END);
    int file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char auth_value[100];
    snprintf(auth_value, sizeof(auth_value), "Bearer %s", api_key);
    esp_http_client_set_header(client, "Authorization", auth_value);
    
    esp_http_client_set_header(client, "Content-Type", "multipart/form-data; boundary=esp32boundary");
    // esp_http_client_set_header(client, "Content-Length", "12667");

    const char* head_boundary = "--esp32boundary\r\n";
    const char* head_template = "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n\r\n";
    const char* filename = getFileName(file_path);
    size_t resultSize = strlen(head_template) + strlen(filename);

    // Allocate memory for the final string
    char *head_final = (char *)malloc(resultSize + 1);

    // Create the final string by formatting the head_template with the filename
    snprintf(head_final, resultSize + 1, head_template, filename);

    const char* model_boundary = "\r\n--esp32boundary\r\n";
    const char* model = "Content-Disposition: form-data; name=\"model\"\r\n\r\nwhisper-1\r\n";
    const char* tail = "--esp32boundary--\r\n";

    // Set content length
    size_t content_length = strlen(head_boundary) + strlen(head_final) + strlen(model_boundary) + strlen(model) + file_size + strlen(tail);
    ESP_LOGE("","Length : %d", content_length);

    esp_err_t ret = esp_http_client_open(client, content_length);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(ret));
        esp_http_client_cleanup(client);
    } 
    
    // Start building the request body
    esp_http_client_write(client, head_boundary, strlen(head_boundary));
    esp_http_client_write(client, head_final, strlen(head_final));

    // Loop to read and send file content
    while (file_size > 0) 
    {
        int read_len = fread(buffer, 1, MAX_HTTP_RECV_BUFFER, file);
        if (read_len <= 0) 
        {
            ESP_LOGE(TAG, "Error reading file");
            break;
        }
        esp_http_client_write(client, buffer, read_len);
        file_size -= read_len;
    }

    // Add model parameter to the request body
    esp_http_client_write(client, model_boundary, strlen(model_boundary));
    esp_http_client_write(client, model, strlen(model));
    esp_http_client_write(client, tail, strlen(tail));

    fclose(file);

    // Perform HTTP request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) 
    {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    } 
    else 
    {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(head_final);
}