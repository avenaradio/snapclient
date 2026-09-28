#include "osc.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "esp_log.h"
#include "esp_osc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "dsp_processor.h"


#define OSC_IN_PORT 9001
#define OSC_OUT_PORT 9002
#define OSC_OUT_ADDRESS "255.255.255.255" // 192.168.178.255 // 255.255.255.255
#define OSC_BUFFER_SIZE 1024

#define TAG "osc"

static esp_osc_client_t client;
static esp_osc_target_t target;

float stored_last_volume = 0.0f;
int muted = 0;

static bool osc_callback(const char *topic, const char *format, esp_osc_value_t *values);
static void receiver(void *parameter);
void gain(float gain);

void osc_init(void){
    dsp_processor_set_volome(0.0);
    target = esp_osc_target(OSC_OUT_ADDRESS, OSC_OUT_PORT);
    esp_osc_init(&client, OSC_BUFFER_SIZE, OSC_IN_PORT);
    // xTaskCreatePinnedToCore(sender, "sender", 4096, NULL, 10, NULL, 1);
    xTaskCreatePinnedToCore(receiver, "receiver", 4096, NULL, 10, NULL, 1);
}

// static void sender(void *parameter){
//     (void)parameter;
//     for (;;) {
//         vTaskDelay(pdMS_TO_TICKS(1000));

//         esp_osc_send(&client, &target, "test", "ihfdsb", 42, (int64_t)84, 3.14f, 6.28, "foo", 3, "bar");
//     }
// }

bool osc_send_floats(const char *topic, const char *format, float *values){
    if (topic == NULL || format == NULL || values == NULL) {
        return false;
    }
    size_t length = strlen(format);
    switch (length) {
        case 1:
            return esp_osc_send(&client, &target, topic, format, values[0]);
        case 2:
            return esp_osc_send(&client, &target, topic, format, values[0], values[1]);
        case 3:
            return esp_osc_send(&client, &target, topic, format, values[0], values[1], values[2]);
        default:
            return false;
    }
}
bool osc_send_int(const char *topic, const char *format, int value){
    if (topic == NULL || format == NULL) {
        return false;
    }
    return esp_osc_send(&client, &target, topic, format, value);
}

static bool osc_callback(const char *topic, const char *format, esp_osc_value_t *values){
    ESP_LOGI(TAG, "got message: %s (%s)", topic, format);
    for (size_t i = 0; i < strlen(format); i++) {
        switch (format[i]) {
        case 'i':
            ESP_LOGI(TAG, "==> i: %d", values[i].i);
            break;

        case 'h':
            ESP_LOGI(TAG, "==> h: %lld", values[i].h);
            break;

        case 'f':
            ESP_LOGI(TAG, "==> f: %f", values[i].f);
            break;

        case 'd':
            ESP_LOGI(TAG, "==> d: %f", values[i].d);
            break;

        case 's':
            ESP_LOGI(TAG, "==> s: %s", values[i].s);
            break;

        case 'b':
            ESP_LOGI(TAG, "==> b: %.*s (%d)",
                     values[i].bl,
                     values[i].b,
                     values[i].bl);
            break;
        }
    }
    if (strcmp(topic, "/adm/obj/16/gain") == 0) {
        if (strcmp(format, "f") == 0) {
            stored_last_volume = values[0].f;
            if(!muted){
                dsp_processor_set_volome(stored_last_volume);
            }
        } else {
            float values[1] = {stored_last_volume};
            osc_send_floats(topic, "f", values);
        }
    } else if (strcmp(topic, "/adm/obj/16/mute") == 0) {
        if (strcmp(format, "i") == 0) {
            if (values[0].i == 1 && muted == 0){
                dsp_processor_set_volome(0.0);
                muted = 1;
            } else if(values[0].i == 0 && muted == 1){
                dsp_processor_set_volome(stored_last_volume);
                muted = 0;
            }
        } else {
            osc_send_int(topic, "i", muted);
        }
    }
    return true;
}

static void receiver(void *parameter){
    (void)parameter;

    while(1){
        esp_osc_receive(&client, osc_callback);
    }
}