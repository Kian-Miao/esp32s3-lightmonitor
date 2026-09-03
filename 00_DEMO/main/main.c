//配置iic通讯，通过写函数配置从机，通过读函数读取从机数据，再将数据传到lcd屏幕
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lcd.h"
#include "sensor.h"

//初始化sensor成员，使用_read函数读取并将结构体数据传递给queue
void sensor_task(void *arg)
{

    //给句柄一个具体的指针类型（void *arg是通用指针，可能代表很多类型）
    QueueHandle_t sensor_queue = (QueueHandle_t)arg;
   
    //创建并初始化sensor成员
    sensor_data_t send_data = {
        .lux_data = 0,
        .sensor_enable = true,
        .sensor_ok = true,
        .sensor_last = true,
    };

    //循环读取.data结构体并将其传递给queue
    while (1)
    {
        sensor_read(&send_data);
        xQueueSend(sensor_queue,&send_data,pdMS_TO_TICKS(50));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
}

//接收queue中的结构体数据，并在接收成功后打印
void data_manager_task(void *arg)
{
    QueueHandle_t sensor_queue = (QueueHandle_t)arg;

    sensor_data_t receive_data;
   
    while (1)
    {    
        int rc = xQueueReceive(sensor_queue,&receive_data,portMAX_DELAY);
        if(rc == pdTRUE)
        {
            if(receive_data.sensor_ok == true)
            {
                printf("data_manager received:%.2f\n",receive_data.lux_data);
            }
            else
            {
                printf("data_manager received:fail\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    lcd_init();
    sensor_init();

    QueueHandle_t sensor_queue;
    sensor_queue = xQueueCreate(5,sizeof(sensor_data_t));

    lcd_show_string(1,1,"light:",GREEN,BLACK);
    
    xTaskCreate(sensor_task,"sensor_task",4086,sensor_queue,4,NULL);//(任务函数，任务函数名称，栈大小，传给任务函数的参数，任务优先级，任务函数句柄)
    xTaskCreate(data_manager_task,"data_manager_task",4086,sensor_queue,4,NULL);
}
