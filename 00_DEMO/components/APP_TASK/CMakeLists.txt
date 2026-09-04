idf_component_register(SRCS "app_task.c"
                    INCLUDE_DIRS "."
                    REQUIRES MYBLE driver freertos IIC SENSOR LCD BH1750 LED KEY MQTT_APP)
