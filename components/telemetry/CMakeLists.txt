idf_component_register(
    SRCS "src/telemetry.c"
    INCLUDE_DIRS "include"
    REQUIRES posture_core esp_timer
)
