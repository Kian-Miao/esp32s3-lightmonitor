#include "app_task.h"
#include "lcd.h"
#include "sensor.h"
#include "freertos/queue.h"
#include "wifista.h"
#include "nvs_flash.h"
#include "mqtt_app.h"
#include "myble.h"
#include <stdio.h>

// 进行必要的初始化，创建队列，创建任务
void app_main(void)
{
    // 初始化
    lcd_init();
    nvs_flash_init();
    sensor_init();
    wifista_init();

    if (wifista_wait_connected(15000))
    {
        mqtt_app_start();
    }
    else
    {
        printf("wifi wait timeout, mqtt not started\n");
    }

    ble_init();

    // 创建队列并判断是否创建成功，不成功则打印
    QueueHandle_t sensor_queue;
    sensor_queue = xQueueCreate(5, sizeof(sensor_data_t));
    if (sensor_queue == NULL)
    {
        printf("sensor_queue create:fail\n");
        return;
    }

    // 默认显示字样
    lcd_show_string(1, 1, "light:", GREEN, BLACK);

    // 创建队列传递任务与队列读取任务并判断是否创建成功，不成功则打印
    BaseType_t ret0 = xTaskCreate(sensor_task, "sensor_task", 4086, sensor_queue, 4, NULL);
    if (ret0 != pdPASS)
    {
        printf("sensor_task_create:fail\n");
        return;
    }

    BaseType_t ret1 = xTaskCreate(data_manager_task, "data_manager_task", 4086, sensor_queue, 4, NULL);
    if (ret1 != pdPASS)
    {
        printf("manager_task_create:fail\n");
        return;
    }
}