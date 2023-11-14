#pragma once

#define FILE_PATH "/spiffs/audio.mp3"
#define API_KEY "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"

#define MAX_HTTP_RECV_BUFFER 1024

void spiffs_init();
void send_post_request(const char* api_key, const char* file_path);