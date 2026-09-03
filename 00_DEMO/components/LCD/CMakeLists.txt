idf_component_register(SRCS "lcd.c"
                    INCLUDE_DIRS "."
                    REQUIRES freertos SPI LED esp_driver_gpio)
