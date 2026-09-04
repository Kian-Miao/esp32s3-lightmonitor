//

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd.h"
#include "sensor.h"
#include "app_task.h"
#include "freertos/queue.h"
#include "mqtt_app.h"
#include "bh1750.h"
#include "myble.h"

//任务函数：初始化sensor成员，调用_switch函数判断按键状态并更新至.data，使用_read函数读取并将.data传递给queue
void sensor_task(void *arg)
{
    //给句柄一个具体的指针类型（void *arg是通用指针，可能代表很多类型）
    QueueHandle_t sensor_queue = (QueueHandle_t)arg;
   
    //创建并初始化sensor成员
    sensor_data_t send_data = {
        .lux_data = 0,
        .sensor_enable = false,
        .sensor_ok = true,
    };

    //循环读取.data结构体并将其传递给queue
    while (1)
    {
        sensor_switch(&send_data);

        //只有开机时读取数据，以免占用i2c通道
        if(send_data.sensor_enable == true)
        {
            sensor_read(&send_data);
        }
        xQueueSend(sensor_queue,&send_data,pdMS_TO_TICKS(50));


        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}

//任务函数：接收queue中的结构体数据，并在检测到开机后在lcd和broker上发布数据；检测到关机后lcd和broker上发布状态
void data_manager_task(void *arg)
{
    QueueHandle_t sensor_queue = (QueueHandle_t)arg;

    sensor_data_t receive_data;

    bool last_sensor_enable = true;
   
    while (1)
    {    
        int rc = xQueueReceive(sensor_queue,&receive_data,portMAX_DELAY);
        ble_update_sensor_data(receive_data.lux_data, receive_data.sensor_enable, receive_data.sensor_ok);
        if(rc == pdTRUE)
        {
            if(receive_data.sensor_ok == true)
            {
                if(receive_data.sensor_enable == false && receive_data.sensor_enable != last_sensor_enable)
                {
                    mqtt_app_publish_status("off");
                    lcd_show_string(1,7,"off       ",GRAY,BLACK);
                    last_sensor_enable = false;
                }
                else if(receive_data.sensor_enable == true)
                {
                    mqtt_app_publish_light(receive_data.lux_data);//向broker发送数据
                    lcd_show_float(1,7,receive_data.lux_data,sizeof(receive_data.lux_data),GREEN,BLACK);
                    last_sensor_enable = true;
                }
            }
            else
            {
                lcd_show_string(1,7,"fail     ",RED,BLACK);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}