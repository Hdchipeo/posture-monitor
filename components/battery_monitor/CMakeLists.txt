idf_component_register(
    SRCS "src/battery_monitor.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_adc esp_timer driver
)
