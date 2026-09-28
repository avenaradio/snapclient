#ifndef OSC_H
#define OSC_H

#include "freertos/FreeRTOS.h"

extern QueueHandle_t volume_queue;
void osc_init(void);

#endif /* OSC_H */
