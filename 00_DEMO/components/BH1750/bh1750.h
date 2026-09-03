#ifndef MY_BH1750_H_
#define MY_BH1750_H_

#include <stdint.h>
#include "esp_err.h"

#define bh1750_write_addr 0x46
#define bh1750_read_addr 0x47

#define PowerDown 0x00
#define PowerOn 0x01
#define Reset 0x07
#define HResolutionMode 0x10

void bh1750_send_cmd(uint8_t cmd_data);
esp_err_t bh1750_read_data(uint16_t *raw_data);

#endif