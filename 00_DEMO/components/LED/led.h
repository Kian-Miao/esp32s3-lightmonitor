//头文件保护，是为了防止头文件被重复多次包含，MY_LED_H相当于flag，此flag出现后，即便多次包含头文件，也只会存在一个。
#ifndef MY_LED_H_//ifnotdefine，如果没定义过MY_LED_H_
#define MY_LED_H_//定义MY_LED_H_

#include "driver/gpio.h"

void led_init(void); //函数声明，告诉编译器有这么一个不接收参数不返回值的函数
void gpio_toggle(gpio_num_t gpio_num);//接受引脚参数，无返回值的函数

#endif//如果前面有定义过，则直接结束。