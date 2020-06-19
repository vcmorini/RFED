set(COMPONENT_SRCS "esp_eddystone_api.c")

set(COMPONENT_ADD_INCLUDEDIRS "include")

# requirements can't depend on config
set(COMPONENT_REQUIRES esp32 bt newlib)

register_component()
