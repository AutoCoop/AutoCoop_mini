idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_gpio 
                             esp_driver_i2c 
                             esp_driver_ledc 
                             esp_adc 
                             nvs_flash 
                             esp_wifi 
                             esp_netif 
                             esp_http_server
                             esp_timer
                             dht22
                             mqtt)
