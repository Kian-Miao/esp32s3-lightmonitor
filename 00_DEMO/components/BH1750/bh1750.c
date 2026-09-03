//使用bh1750_send_cmd向bh1750发送数据
//使用bh1750_read_data将bh1750的数据存入*raw_data并返回是否接收成功
#include "bh1750.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "iic.h"

void iic_init();

//创建发送指令函数，接收值为需要发送的数据
void bh1750_send_cmd(uint8_t cmd_data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();//创建命令链并将命令链句柄命名为cmd
    i2c_master_start(cmd);//开始位
    i2c_master_write_byte(cmd,bh1750_write_addr,true);//寻址，点名从机使之准备接收，并接收应答
    i2c_master_write_byte(cmd,cmd_data,true);//主机发送数据，并接收应答
    i2c_master_stop(cmd);//终止位
    i2c_master_cmd_begin(I2C_NUM_0,cmd,pdMS_TO_TICKS(1000));//i2c通讯使能。若超时1000ms则通讯终止
    i2c_cmd_link_delete(cmd);//删除命令链释放内存
}
//创建接收指令函数，判断是否接收成功，数据
esp_err_t bh1750_read_data(uint16_t *raw_data)
{
    if(raw_data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t light_high = 0,light_low = 0;//定义两个一字节变量用于存放接收的数据
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();//注意在两个函数里虽然句柄都叫cmd但是不一样
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd,bh1750_read_addr,true);//寻址，点名从机使之准备发送，并接受应答
    i2c_master_read_byte(cmd,&light_high,I2C_MASTER_ACK);//接收高八位数据（高位先行）
    i2c_master_read_byte(cmd,&light_low,I2C_MASTER_NACK);//接收第八位数据
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK)
    {
        return ret;
    }

    *raw_data = light_high<<8|light_low;//两个八位数据拼成十六位数据，lighthigh左移八位作为高八位
    return ESP_OK;
}


