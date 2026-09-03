#ifndef MY_SENSOR_H_
#define MY_SENSOR_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>
#include "esp_err.h"

typedef struct sensor_data_t
{
    float lux_data;
    bool sensor_ok;
    bool sensor_enable;
    bool sensor_last;
}sensor_data_t;

void sensor_switch(sensor_data_t *data);
esp_err_t sensor_read(sensor_data_t *data);
esp_err_t sensor_init(void);

#endif