#include "dht22.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"

static const char *TAG = "DHT22";

static dht22_config_t sensor_cfg;
static bool initialized = false;

// Wait for pin state with timeout
static esp_err_t wait_for_state(int state, uint32_t timeout_us) {
    uint32_t start = esp_timer_get_time();
    while(gpio_get_level(sensor_cfg.gpio_num) != state) {
        if((esp_timer_get_time() - start) > timeout_us) {
            return ESP_ERR_TIMEOUT;
        }
    }
    return ESP_OK;
}

esp_err_t dht22_init(const dht22_config_t *config) {
    if(config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    sensor_cfg = *config;

    // Configure GPIO as open-drain (can be input or output)
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << sensor_cfg.gpio_num),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t ret = gpio_config(&io_conf);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed");
        return ret;
    }

    // Set high initially
    gpio_set_level(sensor_cfg.gpio_num, 1);

    initialized = true;
    ESP_LOGI(TAG, "Initialized on GPIO %d", sensor_cfg.gpio_num);
    return ESP_OK;
}

esp_err_t dht22_read(dht22_data_t *data) {
    if(!initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if(data == NULL) {
        ESP_LOGE(TAG, "Data pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t raw_data[5] = {0};

    // Disable interrupts for timing-critical section
    portDISABLE_INTERRUPTS();

    // Step 1: Send start signal (LOW for 1ms)
    gpio_set_level(sensor_cfg.gpio_num, 0);
    ets_delay_us(3000);

    // Step 2: Release and wait for DHT22 response
    gpio_set_level(sensor_cfg.gpio_num, 1);
    ets_delay_us(40);

    // Step 3: Wait for DHT22 to pull LOW
    if(wait_for_state(0, 200) != ESP_OK) {
        portENABLE_INTERRUPTS();
        ESP_LOGE(TAG, "No response from sensor");
        data->valid = false;
        return ESP_ERR_TIMEOUT;
    }

    // Step 4: Wait for DHT22 to pull HIGH
    if(wait_for_state(1, 200) != ESP_OK) {
        portENABLE_INTERRUPTS();
        ESP_LOGE(TAG, "Response timeout");
        data->valid = false;
        return ESP_ERR_TIMEOUT;
    }

    // Step 5: Wait for data transmission start
    if(wait_for_state(0, 200) != ESP_OK) {
        portENABLE_INTERRUPTS();
        ESP_LOGE(TAG, "Data start timeout");
        data->valid = false;
        return ESP_ERR_TIMEOUT;
    }

    // Step 6: Read 40 bits (5 bytes)
    for(int i = 0; i < 40; i++) {
        // Wait for bit start (LOW to HIGH)
        if(wait_for_state(1, 100) != ESP_OK) {
            portENABLE_INTERRUPTS();
            ESP_LOGE(TAG, "Bit start timeout at bit %d", i);
            data->valid = false;
            return ESP_ERR_TIMEOUT;
        }

        // Measure HIGH duration to determine bit value
        uint32_t start = esp_timer_get_time();
        if(wait_for_state(0, 100) != ESP_OK) {
            portENABLE_INTERRUPTS();
            ESP_LOGE(TAG, "Bit end timeout at bit %d", i);
            data->valid = false;
            return ESP_ERR_TIMEOUT;
        }
        uint32_t duration = esp_timer_get_time() - start;

        // Bit is 1 if HIGH duration > 40μs, else 0
        if(duration > 40) {
            raw_data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    portENABLE_INTERRUPTS();

    // Step 7: Verify checksum
    uint8_t checksum = raw_data[0] + raw_data[1] + raw_data[2] + raw_data[3];
    if(checksum != raw_data[4]) {
        ESP_LOGE(TAG, "Checksum failed: calculated=0x%02X, received=0x%02X",
                 checksum, raw_data[4]);
        data->valid = false;
        return ESP_ERR_INVALID_CRC;
    }

    // Step 8: Convert to actual values
    uint16_t humidity_raw = (raw_data[0] << 8) | raw_data[1];
    uint16_t temperature_raw = (raw_data[2] << 8) | raw_data[3];

    data->humidity = humidity_raw / 10.0;
    data->temperature = (temperature_raw & 0x7FFF) / 10.0;

    // Check sign bit for negative temperature
    if(temperature_raw & 0x8000) {
        data->temperature = -data->temperature;
    }

    data->valid = true;

    ESP_LOGI(TAG, "Read: %.1f°C, %.1f%%", data->temperature, data->humidity);
    return ESP_OK;
}

void dht22_deinit(void) {
    if(initialized) {
        gpio_reset_pin(sensor_cfg.gpio_num);
        initialized = false;
        ESP_LOGI(TAG, "Deinitialized");
    }
}
