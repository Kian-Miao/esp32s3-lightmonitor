idf_component_register(SRCS "mqtt_app.c"
                    INCLUDE_DIRS "."
                    REQUIRES SENSOR WIFISTA mqtt esp-tls driver freertos IIC SENSOR LCD BH1750 LED KEY)
