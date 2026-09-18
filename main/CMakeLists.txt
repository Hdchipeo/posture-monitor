idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES storage_manager mpu6050_sensor actuator_manager posture_core button_ctrl telemetry web_server
)
