//时钟线：io4；数据线：io5
#include "iic.h"
#include "driver/gpio.h"
#include "driver/i2c.h"


void iic_init(void)
{
    i2c_config_t iic_cfg = {
        .clk_flags = 0,//自动分配时钟源
        .master.clk_speed = 100000,//i2c通信速度，可选100khz和400khz
        .mode = I2C_MODE_MASTER,//设置为主机模式
        .scl_io_num = GPIO_NUM_4,//时钟线gpio引脚
        .scl_pullup_en = GPIO_PULLUP_ENABLE,//时钟线开启内部上拉（从机有外部上拉的话可以不开启）
        .sda_io_num = GPIO_NUM_5,//数据线gpio引脚
        .sda_pullup_en = GPIO_PULLUP_ENABLE,//数据线开启内部上拉（若从机有外部上拉的话可以不开启）
    };
    i2c_param_config(I2C_NUM_0,&iic_cfg);

    i2c_driver_install(I2C_NUM_0,I2C_MODE_MASTER,0,0,0);//后面三个零：从机接收缓冲区大小，从机发送缓冲区大小（因为是主机模式所以都是零），默认中断配置
}