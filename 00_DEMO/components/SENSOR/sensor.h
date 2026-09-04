#ifndef MY_SENSOR_H_
#define MY_SENSOR_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>
#include "esp_err.h"

//按键逻辑宏定义
#define KEY_POWERDOWN 0
#define KEY_RESET 1
#define KEY_POWERON 2
#define KEY_DEFAULT 3

//声明数据结构体
typedef struct sensor_data_t
{
    float lux_data;
    bool sensor_ok;
    bool sensor_enable;
}sensor_data_t;

void sensor_switch(sensor_data_t *data);
esp_err_t sensor_read(sensor_data_t *data);
esp_err_t sensor_init(void);

#endif