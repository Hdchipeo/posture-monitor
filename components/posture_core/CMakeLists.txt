idf_component_register(
    SRCS "src/posture_core.c"
    INCLUDE_DIRS "include"
    REQUIRES storage_manager mpu6050_sensor
)
