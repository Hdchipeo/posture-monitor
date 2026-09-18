idf_component_register(
    SRCS "src/mpu6050_sensor.c"
    INCLUDE_DIRS "include"
    REQUIRES driver espressif__mpu6050 esp_timer
)
