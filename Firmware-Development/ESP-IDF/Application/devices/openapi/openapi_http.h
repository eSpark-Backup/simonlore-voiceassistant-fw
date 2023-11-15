#pragma once
#include <esp_http_client.h>

#define FILE_PATH "/spiffs/audiow.wav"
#define API_KEY "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"

#define MAX_HTTP_RECV_BUFFER 1024

void spiffs_init();
void send_post_request(const char* api_key, const char* file_path);
esp_err_t esp_http_client_event_handler(esp_http_client_event_t *evt);
void generateWavHeader(uint8_t* header, int sampleRate, int dataSize);