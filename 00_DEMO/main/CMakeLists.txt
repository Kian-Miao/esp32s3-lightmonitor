idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES BH1750 IIC LCD SPI LED SENSOR)
