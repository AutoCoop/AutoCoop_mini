#ifndef DHT22_H
#define DHT22_H

#include "driver/gpio.h"
#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief DHT22 sensor data
 */
typedef struct {
    float temperature;    // Celsius
    float humidity;       // Percentage (0-100)
    bool valid;          // Data validity flag
} dht22_data_t;

/**
 * @brief DHT22 configuration
 */
typedef struct {
    gpio_num_t gpio_num;    // GPIO pin for data
} dht22_config_t;

/**
 * @brief Initialize DHT22 sensor
 *
 * @param config Pointer to configuration
 * @return ESP_OK on success
 */
esp_err_t dht22_init(const dht22_config_t *config);

/**
 * @brief Read temperature and humidity
 *
 * @param data Pointer to store results
 * @return ESP_OK on success, ESP_ERR_TIMEOUT or ESP_ERR_INVALID_CRC on failure
 */
esp_err_t dht22_read(dht22_data_t *data);

/**
 * @brief Deinitialize DHT22
 */
void dht22_deinit(void);

#endif // DHT22_H
