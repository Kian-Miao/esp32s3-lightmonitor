#include "wifista.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "lcd.h"
#include <stdint.h>
#include <stdio.h>
#include "freertos/event_groups.h"

// 事件组句柄只创建一次，用来保存 Wi-Fi 是否已经拿到 IP。
static EventGroupHandle_t wifi_event_group = NULL;

// wifi初始化成功后处理事件队列的函数
void wifista_event_handler(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (wifi_event_group == NULL)
    {
        return;
    }

    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_START)
        {
            esp_wifi_connect();
        }
        else if (event_id == WIFI_EVENT_STA_CONNECTED)
        {
            printf("wifi:connected\n");
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_IPGET);
            xEventGroupSetBits(wifi_event_group, WIFI_DISCONNECTED);
            printf("wifi:disconnected\n");
            esp_wifi_connect();
        }
    }
    else if (event_base == IP_EVENT)
    {
        if (event_id == IP_EVENT_STA_GOT_IP)
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            printf("wifi:got ip: %d.%d.%d.%d\n",
                   esp_ip4_addr1_16(&event->ip_info.ip),
                   esp_ip4_addr2_16(&event->ip_info.ip),
                   esp_ip4_addr3_16(&event->ip_info.ip),
                   esp_ip4_addr4_16(&event->ip_info.ip));

            xEventGroupClearBits(wifi_event_group, WIFI_DISCONNECTED);
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_IPGET);
        }
    }
}

// 依靠事件组判断wifi是否连接的函数，供外部使用
bool wifi_check(void)
{
    if (wifi_event_group == NULL)
    {
        return false;
    }

    EventBits_t check = xEventGroupGetBits(wifi_event_group);
    return (check & WIFI_CONNECTED_IPGET) != 0;
}

bool wifista_wait_connected(uint32_t timeout_ms)
{
    if (wifi_event_group == NULL)
    {
        return false;
    }

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_IPGET,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(timeout_ms));
    return (bits & WIFI_CONNECTED_IPGET) != 0;
}

// 把 ESP32-S3 配置成 Wi-Fi STA（客户端）模式，设置要连接的 Wi-Fi 名称和密码，注册事件处理函数，然后启动 Wi-Fi。
void wifista_init(void)
{
    wifi_event_group = xEventGroupCreate();
    if (wifi_event_group == NULL)
    {
        printf("wifi_event_group create fail!\n");
        return;
    }

    esp_netif_init();
    esp_event_loop_create_default();

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifista_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifista_event_handler, NULL);

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifista_config = {
        .sta = {
            .ssid = DEFAULT_SSID,
            .password = DEFAULT_PWD,
        }
    };
    esp_wifi_set_config(WIFI_IF_STA, &wifista_config);

    esp_wifi_start();
}