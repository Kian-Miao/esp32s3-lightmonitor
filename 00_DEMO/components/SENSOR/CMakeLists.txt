idf_component_register(SRCS "sensor.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver BH1750 IIC freertos KEY LED)
