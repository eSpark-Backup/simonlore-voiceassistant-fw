/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_board_init.h"
#include "model_path.h"
#include "string.h"
#include "esp_log.h"
#include "openapi_http.h"
#include "wifi.h"
#include "nvs_flash.h"
#include "output.h"

static esp_afe_sr_iface_t *afe_handle = NULL;
static esp_afe_sr_data_t *afe_data = NULL;
static volatile int task_flag = 0;

const char* TAG = "ESP-SKAINET";

#define RECORD_TIME_SECONDS 8
#define ONE_SECOND_FRAMES 31.25f

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    assert(nch<feed_channel);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);
    ESP_LOGE("", "chunk size : %d", audio_chunksize);

    while (1)
    {
        while (task_flag) {
            esp_get_feed_data(false, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);

            afe_handle->feed(afe_data, i2s_buff);
        }
        vTaskDelay(10/ portTICK_RATE_MS);
    }
    
    
    if (i2s_buff) {
        free(i2s_buff);
        i2s_buff = NULL;
    }
    vTaskDelete(NULL);
}

void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    int16_t *buff = malloc(afe_chunksize * sizeof(int16_t));
    assert(buff);
    printf("------------detect start------------\n");
    int volume = 0;
    esp_audio_get_play_vol(&volume);
    ESP_LOGE(TAG, "Volume : %d", volume);
    esp_audio_set_play_vol(70);
    esp_audio_get_play_vol(&volume);
    ESP_LOGE(TAG, "Volume : %d", volume);

    int dataSize = (512 * ONE_SECOND_FRAMES * RECORD_TIME_SECONDS);
    int16_t* printer = malloc(dataSize * sizeof(int16_t));
    if (!printer)
    {
        ESP_LOGE(TAG, "memory not allocated");
    }

    while (task_flag) {
        // afe_fetch_result_t* res = afe_handle->fetch(afe_data); 
        // if (!res || res->ret_value == ESP_FAIL) {
        //     printf("fetch error!\n");
        //     break;
        // }

        // if (res->wakeup_state == WAKENET_DETECTED) {
        //     printf("wakeword detected\n");
	    //     printf("model index:%d, word index:%d\n", res->wakenet_model_index, res->wake_word_index);
        //     printf("-----------LISTENING-----------\n");
        // }
        // esp_audio_play(res->data, res->data_size, 1000/ portTICK_RATE_MS);

        printf("\nRecording.......!\n");
        size_t totalDataSize = 0;
        for (size_t i = 0; i < ONE_SECOND_FRAMES * RECORD_TIME_SECONDS; i++)
        {
            afe_fetch_result_t* res = afe_handle->fetch(afe_data); 
            if (!res || res->ret_value == ESP_FAIL) {
                printf("fetch error!\n");
                break;
            }
            memcpy(printer + totalDataSize, res->data, res->data_size * sizeof(int16_t));
            totalDataSize += res->data_size/2;
            esp_audio_play(res->data, res->data_size,  1000 / portTICK_RATE_MS);
        }
        totalDataSize *= 2;
        printf("\nStopped!\n");
        task_flag = 0;
        // printf("\nPlaying!\n");
        // esp_audio_play(printer, totalDataSize, 1000/ portTICK_RATE_MS);
        // printf("\nEnded!\n");
        char* buf = malloc(MAX_RESPONSE_BUFFER);
        transcribe_wav_file(printer, totalDataSize, buf);

        // Parse the JSON string
        cJSON *json = cJSON_Parse(buf);

        if (json == NULL) {
            fprintf(stderr, "Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        }

        // Extract the "text" field
        cJSON *text_field = cJSON_GetObjectItemCaseSensitive(json, "text");

        if (cJSON_IsString(text_field)) {
            printf("%s\n", text_field->valuestring);
            char* compBuf = malloc(MAX_RESPONSE_BUFFER);
            char* data = createJson(  "model", "gpt-3.5-turbo-instruct",
                                            "prompt", text_field->valuestring,
                                            NULL);
            text_completion(data, compBuf);
            // printf("%s\n", compBuf);
            process_response(compBuf);
            free(data);
            free(compBuf);

        } else {
            fprintf(stderr, "Failed to extract 'text' field from JSON.\n");
        }

        // Free the allocated memory for the cJSON object
        cJSON_Delete(json);

        free(buf);
        // vTaskDelay(5000 / portTICK_PERIOD_MS);
        task_flag = 1;
        
    }
    if (buff) {
        free(buff);
        buff = NULL;
    }
    vTaskDelete(NULL);
}

void app_main()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_board_init(16000, 1, 16));
    wifi_init_sta();
    
    // spiffs_init();

    // vTaskDelay(3000 / portTICK_PERIOD_MS);
    // send_post_request(API_KEY, FILE_PATH);
    // ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));

    srmodel_list_t *models = esp_srmodel_init("model");
    char *wn_name = NULL;
    char *wn_name_2 = NULL;

    if (models!=NULL) {
        for (int i=0; i<models->num; i++) {
            if (strstr(models->model_name[i], ESP_WN_PREFIX) != NULL) {
                if (wn_name == NULL) {
                    wn_name = models->model_name[i];
                    printf("The first wakenet model: %s\n", wn_name);
                } else if (wn_name_2 == NULL) {
                    wn_name_2 = models->model_name[i];
                    printf("The second wakenet model: %s\n", wn_name_2);
                }
            }
        }
    } else {
        printf("Please enable wakenet model and select wake word by menuconfig!\n");
        return ;
    }

    afe_handle = (esp_afe_sr_iface_t *)&ESP_AFE_SR_HANDLE;
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;
    afe_config.wakenet_init = true;
    afe_config.wakenet_model_name = wn_name;
    afe_config.wakenet_model_name_2 = wn_name_2;
    afe_config.voice_communication_init = false;

#if defined CONFIG_ESP32_S3_BOX_BOARD || defined CONFIG_ESP32_S3_EYE_BOARD
    afe_config.aec_init = false;
    #if defined CONFIG_ESP32_S3_EYE_BOARD
        afe_config.pcm_config.total_ch_num = 2;
        afe_config.pcm_config.mic_num = 1;
        afe_config.pcm_config.ref_num = 1;
    #endif
#endif
    afe_data = afe_handle->create_from_config(&afe_config);
    
    task_flag = 1;
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void*)afe_data, 5, NULL, 1);

    // // You can call afe_handle->destroy to destroy AFE.
    // task_flag = 0;

    // printf("destroy\n");
    // afe_handle->destroy(afe_data);
    // afe_data = NULL;
    // printf("successful\n");
}
