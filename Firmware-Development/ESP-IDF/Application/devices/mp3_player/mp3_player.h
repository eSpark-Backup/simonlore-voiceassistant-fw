#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <freertos/queue.h>

extern QueueHandle_t audioQueue;

typedef struct 
{
    const void* data;
    uint32_t len;
} AudioFrame;

void task(void* pv);
