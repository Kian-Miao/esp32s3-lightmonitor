#include "led.h"//头文件相当于对内和对外的合同，led.c文件的函数类型和函数名应与.h文件中声明的保持一致，因此这里include头文件是为检查实现与接口是否一致
#include "driver/gpio.h"
//该函数功能为初始化gpio
void led_init(void)
{
    esp_err_t err;//esp_err_t是变量类型，一种常见的错误码类型，专门存放某个函数执行成功或失败的结果，err是变量名。如果成功，err==espok，如果失败err！=espok
    gpio_config_t gpio_cfg=//gpio_config_t是结构体类型，gpio_cfg是结构体变量
    {
        //.表示指定成员初始化为
        .intr_type=GPIO_INTR_DISABLE,//中断类型：不中断
        .mode=GPIO_MODE_INPUT_OUTPUT,//输入输出模式：输入输出
        .pin_bit_mask=1ull << GPIO_NUM_38,//位掩码，表示将1向左移动38位，于是只有第38位为1，其余全是0，于是gpio38被选中。ull表示无符号(u)长长整型(ll)
        .pull_down_en=GPIO_PULLDOWN_DISABLE,//关闭下拉电阻
        .pull_up_en=GPIO_PULLUP_ENABLE, //开启上拉电阻
        
    };//这个分号不能少，这是个变量定义语句
    err = gpio_config(&gpio_cfg);//gpio_config是执行配置函数，执行config-返回执行结果-结果保存到err。&是取地址运算符，读取gpio_cfg的内存地址
    if (err != ESP_OK)
    {
        printf("gpio init error!\n");
        return;
    }
}
//该函数功能为使gpio输出高低电平翻转
void gpio_toggle(gpio_num_t gpio_num)
{
    if (gpio_get_level(gpio_num)==0)
    {
        gpio_set_level(gpio_num,1);
    }
    else
    {
        gpio_set_level(gpio_num,0);
    }
    
}