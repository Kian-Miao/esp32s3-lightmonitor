#ifndef __MYBLE_H_
#define __MYBLE_H_

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

typedef enum {
    BLE_CMD_NONE = 0,
    BLE_CMD_ON,
    BLE_CMD_OFF,
    BLE_CMD_RESET,
} ble_cmd_t;

void start_advertising(void);
void ble_init(void);
void ble_update_sensor_data(float lux, bool enable, bool ok);
ble_cmd_t ble_take_cmd(void);

#endif