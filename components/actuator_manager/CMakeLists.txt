idf_component_register(
    SRCS "src/actuator_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_timer
)
