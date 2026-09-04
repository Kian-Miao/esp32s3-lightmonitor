#include "myble.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "bh1750.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define TAG "NimBLE-Demo"
#define DEVICE_NAME "ESP32S3-NimBLE"
static bool ble_adv_active = false ;//广播标志位，为true时代表广播开启成功
static uint16_t rx_value_handler;//定义写特征值句柄
static uint16_t tx_value_handler;//定义读特征值句柄
static float ble_lux_data = 0;
static bool ble_sensor_enable = false;
static bool ble_sensor_ok = true;
static ble_cmd_t ble_cmd = BLE_CMD_NONE;

//接收sensor数据函数
void ble_update_sensor_data(float lux, bool enable, bool ok)
{
    ble_lux_data = lux;
    ble_sensor_enable = enable;
    ble_sensor_ok = ok;
}

//gatt事件函数（蓝牙通信阶段）
static int gatt_event_handler(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    //通过op的值来判断发生了哪个事件
    if(ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
    {
        if(attr_handle == rx_value_handler)
        {
            char payload[64] = {0};

            int len = ctxt->om->om_len;

            if(len >= sizeof(payload))
            {
                len = sizeof(payload) - 1;
            }

            memcpy(payload, ctxt->om->om_data, len);
            payload[len] = '\0';

            printf("ble_receive:%s\n", payload);

            if(strstr(payload, "on") != NULL)
            {
                ble_cmd = BLE_CMD_ON;
            }
            else if(strstr(payload, "off") != NULL)
            {
                ble_cmd = BLE_CMD_OFF;
            }
            else if(strstr(payload, "reset") != NULL)
            {
                ble_cmd = BLE_CMD_RESET;
            }
        }
    }
    else if(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)//读特征事件
    {
        if(attr_handle == tx_value_handler)//判断句柄是否正确
        {
            bool last_sensor_enable = true;
            
            char value[64];

            if(ble_sensor_ok == true)
            {
                if(ble_sensor_enable == false && last_sensor_enable == true)
                {
                    //数据填入函数。把数据填入到os_mbuf供手机读取
                    os_mbuf_append(ctxt->om, "off", strlen("off"));//要填入的数据缓冲区；填入的内容；内容的长度
                    last_sensor_enable = false;
                }
                else if(ble_sensor_enable == true)
                {
                    snprintf(value, sizeof(value), "{\"light_lux\":%.2f}", ble_lux_data);
                    os_mbuf_append(ctxt->om, value, strlen(value));
                    last_sensor_enable = true;
                }
                
            }
            
        }
    }
    return 0;
}


//gap事件函数（蓝牙连接阶段）
static esp_err_t gap_event_handler(struct ble_gap_event *event, void *arg)
{

    //当事件类型为connect时，通过status的值判断是否连接成功
    if(event->type == BLE_GAP_EVENT_CONNECT)
    {
        if(event->connect.status == 0 )
        {
            printf("ble:connected\n");
            ble_adv_active = false;//连接成功后关闭广播
        }
        else
        {
            printf("ble:connected fail\n");
            if(!ble_adv_active)//先判断广播状态，若广播标志位为false，则开启广播
            {
                start_advertising();
            }
        }
    }
    //当事件类型为disconnect时，打印
    else if(event->type == BLE_GAP_EVENT_DISCONNECT)
    {
        printf("disconnected\n");
        if(!ble_adv_active)
        {
            start_advertising();
        }
    }
    return 0;
}

//配置服务和特征值
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,//服务类型，此处为主服务，主服务能单独存在，副服务只能依赖于主服务
        .uuid = BLE_UUID16_DECLARE(0x00FF),//当前服务的唯一识别码（调用官方宏定义）
        .characteristics = (struct ble_gatt_chr_def[])//当前服务所包含的所有特征
        {
            //ESP32S3接收手机数据
            {
                .uuid = BLE_UUID16_DECLARE(0xFF01),
                .access_cb = gatt_event_handler,//触发的gatt事件函数
                .flags = BLE_GATT_CHR_F_WRITE,//特征属性（此处为当前特征可写）
                .val_handle = &rx_value_handler,//特征值句柄（该特征句柄会传入gatt事件函数）
                .arg = NULL,//自定义gatt额外参数
            },
            //ESP32S3向手机发送数据
            {
                .uuid = BLE_UUID16_DECLARE(0xFF02),
                .access_cb = gatt_event_handler,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &tx_value_handler,
                .arg = NULL,
            },
            { 0 }//特征值创建完毕
        }
    },
    { 0 }
};

//配置并开启广播的函数
void start_advertising(void)
{
    //广播内容
    struct ble_hs_adv_fields fields = {0};
    fields.name = (uint8_t *)DEVICE_NAME;//要广播的设备名称
    fields.name_len = strlen(DEVICE_NAME);//设备名称长度
    fields.name_is_complete = 1;//表示设备名称是完整的设备名称而不是缩写
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;//广播标志位，用于设置设备的发现模式和协议支持。此处为可被持续发现并且仅支持低功耗蓝牙
    fields.tx_pwr_lvl_is_present = 1;//广播信号携带发射功率信息
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;//芯片依据自身消耗自动配置发射功率
    //设置广播内容。
    int rc = ble_gap_adv_set_fields(&fields);//该函数有int返回值，为0时代表广播内容设置成功。定义变量rc接收该返回值
    if(rc != 0)
    {
        printf("advertising set fail\n");//打印字符
        return;
    }

    //进一步配置的广播内容
    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,//当前广播可以被任意设备连接
        .disc_mode = BLE_GAP_DISC_MODE_GEN,//设备一直处于可被发现模式
        //广播信号发射频率的最大最小值，实际频率在此区间随机选择
        .itvl_min = BLE_GAP_ADV_ITVL_MS(200),//（此处为直接将数字转换为毫秒的宏）
        .itvl_max = BLE_GAP_ADV_ITVL_MS(500),
    };
    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL,BLE_HS_FOREVER,&adv_params,gap_event_handler, NULL);//开启广播。 蓝牙地址（此处为出厂默认地址）；不是非定向广播，所有设备均可检测到；蓝牙广播时间（此处为无限广播）；进一步配置广播内容；触发的gap事件函数;自定义gap额外参数
    if(rc != 0)
    {
        ESP_LOGE(TAG,"广播启动失败");
    }
    else
    {
        ESP_LOGI(TAG,"广播已启动，等待连接...");
        ble_adv_active = true;
    }
}

//创建回调函数,初始化结束后开启广播
static void on_sync(void)
{
    printf("initiative success\n");
    start_advertising();
}

//任务函数
void host_task( void * arg)
{
    nimble_port_run();//循环监听蓝牙协议栈发生的事件
}

//初始化ble协议栈
void ble_init(void)
{
    nimble_port_init();//初始化nimble蓝牙协议栈
    ble_svc_gap_init();//初始化gap（别人怎么找到我 怎么连接我）
    ble_svc_gatt_init();//初始化gatt（链接过后怎么交流信息）
    ble_svc_gap_device_name_set(DEVICE_NAME);//设置设备名称（使用宏定义）
    //以下两个函数为注册服务特征列表
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);

    ble_hs_cfg.sync_cb = on_sync;//以上初始化结束后执行回调函数

    //创建监听函数
    nimble_port_freertos_init(host_task);
}

//取命令
ble_cmd_t ble_take_cmd(void)
{
    ble_cmd_t cmd = ble_cmd;
    ble_cmd = BLE_CMD_NONE;
    return cmd;
}