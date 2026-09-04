idf_component_register(SRCS "wifista.c"
                    INCLUDE_DIRS "."
                    REQUIRES LCD esp_wifi esp_event esp_netif driver)