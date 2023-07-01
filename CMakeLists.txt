idf_component_register(
    SRCS "crc16.c"
         "eros.c"
         "drv_esp/eros_esp_udp.c"
         "drv_esp/eros_esp_logging.c"
         "drv_esp/eros_esp_uart.c"
         "drv_esp/eros_esp_tcp.c"
    INCLUDE_DIRS "."
                 "drv_esp/"

    REQUIRES nanocobs driver
)

# 