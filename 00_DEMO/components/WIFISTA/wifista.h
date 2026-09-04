#ifndef __WIFISTA_H_
#define __WIFISTA_H_

#include <stdbool.h>
#include <stdint.h>

#define DEFAULT_SSID "Laplace"
#define DEFAULT_PWD  "13616360012mm"
#define WIFI_CONNECTED_IPGET (1<<0)
#define WIFI_DISCONNECTED (1<<1)

bool wifi_check(void);
bool wifista_wait_connected(uint32_t timeout_ms);
void wifista_init(void);

#endif