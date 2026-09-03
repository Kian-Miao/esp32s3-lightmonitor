//key_scan函数返回值为0,1,2时分别代表io6,7，8被按下
#include "key.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

//初始化key的gpio
void key_init(void)
{
    gpio_config_t gpio_cfg=
    {
       
        .intr_type=GPIO_INTR_DISABLE,
        .mode=GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << GPIO_NUM_6) |
                        (1ULL << GPIO_NUM_7) |
                        (1ULL << GPIO_NUM_8),
        .pull_down_en=GPIO_PULLDOWN_DISABLE,
        .pull_up_en=GPIO_PULLUP_ENABLE, 
        
    };
    gpio_config(&gpio_cfg);//gpio_config是执行配置函数，执行config-返回执行结果-结果保存到err。&是取地址运算符，读取gpio_cfg的内存地址
}

//检测并返回按键状态，上拉电阻，按下为低电平
uint8_t key_scan(void)
{
    uint8_t key_num = 3;//定义变量存放函数返回值
    if (gpio_get_level(GPIO_NUM_6) == 0)//检测到低电平
    {
        vTaskDelay(pdMS_TO_TICKS(20));//延时，防止按键抖动
        while (gpio_get_level(GPIO_NUM_6) == 0);//只有当按键松开，即高电平时，跳出循环
        vTaskDelay(pdMS_TO_TICKS(20));//延时，防止按键抖动
        key_num = 0;//按键动作结束，函数返回值为1，标志按键按下一次
        
    }
     
    if (gpio_get_level(GPIO_NUM_7) == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(20));
        while (gpio_get_level(GPIO_NUM_7) == 0);
        vTaskDelay(pdMS_TO_TICKS(20));
        key_num = 1;
        
    }
    
    if (gpio_get_level(GPIO_NUM_8) == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(20));
        while (gpio_get_level(GPIO_NUM_8) == 0);
        vTaskDelay(pdMS_TO_TICKS(20));
        key_num = 2;
        
    }

    return key_num;//返回结果
}

   
    

