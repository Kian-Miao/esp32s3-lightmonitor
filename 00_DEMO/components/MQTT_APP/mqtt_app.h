#ifndef MY_MQTT_H_
#define MY_MQTT_H_

#include "esp_err.h"

typedef enum {
    MQTT_CMD_NONE = 0,
    MQTT_CMD_ON,
    MQTT_CMD_OFF,
    MQTT_CMD_RESET,
} mqtt_cmd_t;

esp_err_t mqtt_app_start(void);
esp_err_t mqtt_app_publish_light(float lux);
mqtt_cmd_t mqtt_app_take_cmd(void);
esp_err_t mqtt_app_publish_status(const char *status);


#endif