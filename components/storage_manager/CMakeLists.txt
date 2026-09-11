idf_component_register(
    SRCS "src/storage_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash esp_timer
)
