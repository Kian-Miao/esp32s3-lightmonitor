//sensor_data_t结构体中的成员为bh1750的各项数据，目前有光照强度lux_data成员
//使用sensor_switch以实现：gpio6：bh1750关机，gpio7：复位，gpio8：开机并连续监测
//使用sensor_read以实现：读取光照强度并写入结构体sensor_data_t中的lux_data成员;通过sensor_ok成员判断是否成功读取到数据；通过sensor_read返回值判断是否成功获取lux_data的地址
//使用sensor_init以实现：初始化sensor所要使用的其他初始配置
#include "sensor.h"
#include "bh1750.h"
#include "iic.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "key.h"
#include <stdint.h>
#include "esp_err.h"
#include "led.h"

esp_err_t sensor_read(sensor_data_t *data)
{
    if (data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    uint16_t raw = 0;
    esp_err_t ret = bh1750_read_data(&raw);
    if(ret != ESP_OK)
    {
        data->sensor_ok = false;
        return ret;
    }

    data->lux_data = raw / 1.2f;
    data->sensor_ok = true;

    return ESP_OK;
}


//目前该初始化函数默认开机自启动
esp_err_t sensor_init(void)
{
    iic_init();
    key_init();
    led_init();

    bh1750_send_cmd(PowerOn);
    bh1750_send_cmd(HResolutionMode);

    vTaskDelay(pdMS_TO_TICKS(180));

    return ESP_OK;
}

void sensor_switch(sensor_data_t *data)
{
    int order = key_scan();  

    if(order == 0)
    {
        bh1750_send_cmd(PowerDown);
        data->sensor_enable = false;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    else if (order == 1)
    {
        bh1750_send_cmd(PowerOn);
        bh1750_send_cmd(Reset);
        bh1750_send_cmd(HResolutionMode);
        data->sensor_enable = true;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    else if (order == 2)
    {
        bh1750_send_cmd(PowerOn);
        bh1750_send_cmd(HResolutionMode);
        data->sensor_enable = true;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    
}

