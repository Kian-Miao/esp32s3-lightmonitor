//使用_start函数以初始化mqtt并尝试连接broker，连接状态打印。
//使用_publish函数使esp32向broker发布信息
//发生mqtt事件后触发mqtt回调函数，其中包括：连接事件发生：（订阅控制以接收命令；check置1以能发布信息）；接收数据事件发生：（将接收的数据复制到payload，并根据内容下达命令）
#include "mqtt_app.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "esp_err.h"
#include "mqtt_client.h"
#include "esp_crt_bundle.h"
#include "wifista.h"
#include "sensor.h"
#include "freertos/queue.h"

//宏定义broker信息
#define MQTT_BROKER_URI "mqtts://u3f19057.ala.cn-hangzhou.emqxsl.cn:8883"
#define MQTT_USERNAME   "kulisu"
#define MQTT_PASSWORD   "123"
#define MQTT_LIGHT_TOPIC "esp32/light"
#define MQTT_CONTROL_TOPIC "esp32/control"

//客户端句柄
esp_mqtt_client_handle_t mqtt_client;

//判断mqtt是否连接的变量
bool mqtt_check;

//命令接收的变量
mqtt_cmd_t cmd = MQTT_CMD_NONE;

//mqtt事件处理回调函数
void mqtt_event_handler(void *handler_args,esp_event_base_t base,int32_t event_id,void *event_data)//额外传入的参数；事件类型；事件名称；指向事件具体数据的指针
{
    esp_mqtt_event_handle_t event = event_data;

    switch (event_id)
    {
    case MQTT_EVENT_CONNECTED:
        mqtt_check = true;
        printf("mqtt:connected\n");
        esp_mqtt_client_subscribe(mqtt_client,MQTT_CONTROL_TOPIC,1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        mqtt_check = false;
        printf("mqtt:disconnected\n");
        break;
    case MQTT_EVENT_ERROR:
        printf("mqtt:error\n");
        break;
    case MQTT_EVENT_DATA:
    {
        char payload[64] = {0};//char数组，当函数参数需要地址时，直接引用payload表示该数组第一个数的地址

        //判断接收数据大小防止数据溢出        
        int len = event ->data_len;
        if(len >= sizeof(payload))
        {
            len = sizeof(payload) -1;
        }

        //把mqtt中的数据复制到payload，并在最后添加c字符串结束符，目的是把数据转换为标准字符串，后面才能用printf打印
        memcpy(payload,event->data,len);
        payload[len] = '\0';

        printf("mqtt_receive:%s\n",payload);

        //根据payload内容下达命令
        if(strstr(payload,"on") != NULL )
        {
            cmd = MQTT_CMD_ON;
        }
        else if(strstr(payload,"off") != NULL)
        {
            cmd = MQTT_CMD_OFF;
        }
        else if(strstr(payload,"reset") != NULL)
        {
            cmd = MQTT_CMD_RESET;
        }
        break;

    }
    default:
        break;
    }


}

//mqtt初始化函数
esp_err_t mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);

    if (mqtt_client == NULL)
    {
        printf("mqtt client init failed\n");
        return ESP_FAIL;
    }
    
    esp_mqtt_client_register_event(mqtt_client,ESP_EVENT_ANY_ID,mqtt_event_handler,NULL);//给哪个客户端注册事件函数，注册哪些事件函数，发生事件后执行那个回调函数，额外参数

    return esp_mqtt_client_start(mqtt_client);
}

//mqtt信息发送函数
esp_err_t mqtt_app_publish_light(float lux)
{

    if (mqtt_client == NULL)
    {
        return ESP_FAIL;
    }

    if (mqtt_check == false)
    {
        return ESP_FAIL;
    }

    if (wifi_check() == false)
    {
        return ESP_FAIL;
    }

    //暂存要发送的数据
    char payload[64];

    //生成json，目的是使信息更可读，不是必须步骤
    snprintf(payload, sizeof(payload),"{\"light_lux\":%.2f}",lux);//发送的数据在哪；数据长度；发送内容

    //发送mqtt消息
    int msg_id = esp_mqtt_client_publish(mqtt_client,MQTT_LIGHT_TOPIC,payload,0,1,0);//哪个mqtt客户端；什么主题；发送内容；自动计算长度；服务等级；不要求保存成retained message
    
    if (msg_id < 0)
    {
        printf("mqtt publish failed\n");
        return ESP_FAIL;
    }

    printf("mqtt publish: %s\n", payload);
    
    return ESP_OK;

}

//mqtt状态发送函数
esp_err_t mqtt_app_publish_status(const char *status)
{
    if (mqtt_client == NULL)
    {
        return ESP_FAIL;
    }

    if (mqtt_check == false)
    {
        return ESP_FAIL;
    }

    if (wifi_check() == false)
    {
        return ESP_FAIL;
    }

    int msg_id = esp_mqtt_client_publish(mqtt_client,MQTT_LIGHT_TOPIC,status,0,1,0);

    if (msg_id < 0)
    {
        printf("mqtt publish status failed\n");
        return ESP_FAIL;
    }

    printf("mqtt publish status: %s\n", status);

    return ESP_OK;
}

//取出并清空命令，防止在task函数的while里循环重复执行命令。（off on reset这样的命令只需要执行一次，所以必须取出再清空，这样下一次循环看到的命令就是none了）
mqtt_cmd_t mqtt_app_take_cmd(void)
{
    mqtt_cmd_t mqtt_cmd = cmd;
    cmd = MQTT_CMD_NONE;
    return mqtt_cmd;
}