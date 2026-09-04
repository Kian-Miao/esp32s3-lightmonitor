idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES bt APP_TASK SPI SENSOR WIFISTA nvs_flash MYBLE)
