idf_component_register(SRCS "spi.c"
                    INCLUDE_DIRS "."
                    REQUIRES LCD SPI LED esp_driver_spi)
