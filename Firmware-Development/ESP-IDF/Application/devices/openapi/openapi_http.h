#pragma once
#include <esp_http_client.h>

#define FILE_PATH "/spiffs/audiow.wav"
#define API_KEY "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"

#define MAX_HTTP_RECV_BUFFER 1024

typedef struct {
    char riff[4];           // "RIFF"
    int fileSize;           // Total file size - 8 bytes
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    int fmtSize;            // Size of the fmt chunk
    short audioFormat;      // Audio format (1 for PCM)
    short numChannels;      // Number of channels (1 for mono, 2 for stereo)
    int sampleRate;         // Sample rate
    int byteRate;           // Byte rate (SampleRate * NumChannels * BitsPerSample / 8)
    short blockAlign;       // Block align (NumChannels * BitsPerSample / 8)
    short bitsPerSample;    // Bits per sample
    char data[4];           // "data"
    int dataSize;           // Size of the data chunk
} WavHeader;

void spiffs_init();
void send_post_request(const char* api_key, const char* file_path);
esp_err_t esp_http_client_event_handler(esp_http_client_event_t *evt);
void fillWavHeader(WavHeader *header, int dataSize);
void transcribe_wav_file(int16_t* stream, uint32_t size);