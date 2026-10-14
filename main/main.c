#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
// Food ultrasound sensor threshold
#define ULTRASOUND_THRESHOLD 20.0

//Water level sensor threshold
#define WATER_LEVEL_THRESHOLD 10.0

// Gas sensor threshold
#define GAS_THRESHOLD 300

// Temperature & Humidity warning threshold
#define TEMP_THRESHOLD 30.0
#define HUMIDITY_THRESHOLD 70.0

// Light on time
#define LIGHT_ON_HOUR 18
#define LIGHT_ON_MINUTE 0
#define LIGHT_ON_SECOND 0

// Light off time
#define LIGHT_OFF_HOUR 18
#define LIGHT_OFF_MINUTE 0
#define LIGHT_OFF_SECOND 0

// Door open time
#define DOOR_OPEN_HOUR 6
#define DOOR_OPEN_MINUTE 0
#define DOOR_OPEN_SECOND 0  
// Door close time
#define DOOR_CLOSE_HOUR 18
#define DOOR_CLOSE_MINUTE 0
#define DOOR_CLOSE_SECOND 0

// Pin Definitions
#define LIGHT_BTN       13
#define LIGHT_LED       19
#define FAN_BTN         32
#define FAN_PIN         18
#define DOOR_BTN        26
#define DHT_PIN         14
#define SERVO_PIN       27

// I2C & RTC Definitions
#define I2C_MASTER_SCL_IO    22
#define I2C_MASTER_SDA_IO    21
#define I2C_MASTER_NUM       I2C_NUM_0
#define I2C_MASTER_FREQ_HZ   100000
#define DS1307_ADDR          0x68

// Servo Definitions

#define LEDC_TIMER           LEDC_TIMER_0
#define LEDC_MODE            LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL         LEDC_CHANNEL_0
#define LEDC_DUTY_RES        LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY       50

static QueueHandle_t gpio_evt_queue = NULL;

// States tracking
bool is_light_on = false;
bool is_fan_on = false;
bool is_door_open = false;

// 1. Light Function
void light_on_off(void) {
    if (is_light_on) {
        gpio_set_level(LIGHT_LED, 0);
        is_light_on = false;
        printf("Light is turned OFF\n");
    } else {
        gpio_set_level(LIGHT_LED, 1);
        is_light_on = true;
        printf("Light is turned ON\n");
    }
}

// 2. Fan Function 
void fan_on_off(void) {
    if (is_fan_on) {
        gpio_set_level(FAN_PIN, 0);
        is_fan_on = false;
        printf("Fan is turned OFF\n");
    } else {
        gpio_set_level(FAN_PIN, 1);
        is_fan_on = true;
        printf("Fan is turned ON\n");
    }
}

// Servo Helper Functions
uint32_t servo_angle_to_duty(int angle) {
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    return 410 + (angle * (819 - 410)) / 180;
}

void servo_set_angle(int angle) {
    uint32_t duty = servo_angle_to_duty(angle);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

// 3. Door Function (Servo)
void door_open_close(void) {
    if (is_door_open) {
        is_door_open = false;
        printf("Door is CLOSED (0 degrees)\n");
        servo_set_angle(0);
    } else {
        is_door_open = true;
        printf("Door is OPENED (90 degrees)\n");
        servo_set_angle(90);
    }
}

// Servo Initialization
void servo_init(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE, // FIXED: Changed from LEDC_INT_DISABLE
        .gpio_num       = SERVO_PIN,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Button Task with Debouncing
static void button_task(void* arg) {
    uint32_t io_num;
    static TickType_t last_light_tick = 0;
    static TickType_t last_fan_tick = 0;
    static TickType_t last_door_tick = 0;
    const TickType_t debounce_delay = pdMS_TO_TICKS(250);

    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            TickType_t current_tick = xTaskGetTickCount();

            if (io_num == LIGHT_BTN) {
                if ((current_tick - last_light_tick) > debounce_delay) {
                    last_light_tick = current_tick;
                    printf("Light button pressed\n");
                    light_on_off();
                }
            } else if (io_num == FAN_BTN) {
                if ((current_tick - last_fan_tick) > debounce_delay) {
                    last_fan_tick = current_tick;
                    printf("Fan button pressed\n");
                    fan_on_off();
                }
            } else if (io_num == DOOR_BTN) {
                if ((current_tick - last_door_tick) > debounce_delay) {
                    last_door_tick = current_tick;
                    printf("Door button pressed\n");
                    door_open_close();
                }
            }
        }
    }
}
// TO DO :MQ-138 gas sensor initialization and reading function
// TODO: Ultrasonic sensor initialization and reading function for food level
// TODO: Water level sensor initialization and reading function


// DHT22 Read Function
int read_dht22(float *temperature, float *humidity) {
    uint8_t data[5] = {0};
    
    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    esp_rom_delay_us(1000);
    gpio_set_level(DHT_PIN, 1);
    esp_rom_delay_us(30);
    
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);
    
    uint32_t timeout = 10000;
    while (gpio_get_level(DHT_PIN) == 1) if (--timeout == 0) return -1;
    timeout = 10000;
    while (gpio_get_level(DHT_PIN) == 0) if (--timeout == 0) return -1;
    timeout = 10000;
    while (gpio_get_level(DHT_PIN) == 1) if (--timeout == 0) return -1;

    for (int i = 0; i < 40; i++) {
        timeout = 10000;
        while (gpio_get_level(DHT_PIN) == 0) if (--timeout == 0) return -1;
        
        esp_rom_delay_us(30);
        
        if (gpio_get_level(DHT_PIN) == 1) {
            data[i / 8] |= (1 << (7 - (i % 8)));
            timeout = 10000;
            while (gpio_get_level(DHT_PIN) == 1) if (--timeout == 0) return -1;
        }
    }

    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) return -2;

    *humidity = ((data[0] << 8) + data[1]) * 0.1;
    int16_t raw_temp = ((data[2] & 0x7F) << 8) + data[3];
    *temperature = raw_temp * 0.1;
    if (data[2] & 0x80) *temperature = -(*temperature);

    return 0;
}

void dht_task(void *pvParameters) {
    float temperature = 0, humidity = 0;
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DHT_PIN),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    for (;;) {
        if (read_dht22(&temperature, &humidity) == 0) {
            printf("Humidity: %.1f%%, Temp: %.1f°C\n", humidity, temperature);
            if (temperature > TEMP_THRESHOLD && is_fan_on == false) {
                printf("Warning: High Temperature!\n");
                fan_on_off();
            }
            if (humidity > HUMIDITY_THRESHOLD && is_fan_on == false) {
                printf("Warning: High Humidity!\n");
                fan_on_off();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// RTC Functions
uint8_t bcd_to_dec(uint8_t val) {
    return (val / 16 * 10) + (val % 16);
}

void i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

void rtc_task(void *pvParameters) {
    int sec, min, hour, day, date, month, year;
    uint8_t write_buf = 0x00;
    uint8_t read_buf[7];

    for (;;) {
        i2c_master_write_to_device(I2C_MASTER_NUM, DS1307_ADDR, &write_buf, 1, pdMS_TO_TICKS(1000));
        if (i2c_master_read_from_device(I2C_MASTER_NUM, DS1307_ADDR, read_buf, 7, pdMS_TO_TICKS(1000)) == ESP_OK) {
            sec   = bcd_to_dec(read_buf[0] & 0x7F);
            min   = bcd_to_dec(read_buf[1]);
            hour  = bcd_to_dec(read_buf[2] & 0x3F);
            date  = bcd_to_dec(read_buf[4]);
            month = bcd_to_dec(read_buf[5]);
            year  = bcd_to_dec(read_buf[6]) + 2000;
            printf("Time: %02d:%02d:%02d | Date: %02d/%02d/%d\n", hour, min, sec, date, month, year);
            if (hour == DOOR_OPEN_HOUR && min == DOOR_OPEN_MINUTE && sec == DOOR_OPEN_SECOND && is_door_open == false) {
                printf("Door is now open!\n");
                door_open_close();
            } else if (hour == DOOR_CLOSE_HOUR && min == DOOR_CLOSE_MINUTE && sec == DOOR_CLOSE_SECOND && is_door_open == true) {
                printf("Door is now closed!\n");
                door_open_close();
            }
            if (hour == LIGHT_ON_HOUR && min == LIGHT_ON_MINUTE && sec == LIGHT_ON_SECOND && is_light_on == false) {
                printf("Light is now ON!\n");
                light_on_off();
            } else if (hour == LIGHT_OFF_HOUR && min == LIGHT_OFF_MINUTE && sec == LIGHT_OFF_SECOND && is_light_on == true) {
                printf("Light is now OFF!\n");
                light_on_off();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Main Function (Entry Point)
void app_main(void) {
    // 1. Outputs Setup
    gpio_set_direction(LIGHT_LED, GPIO_MODE_OUTPUT);
    gpio_set_direction(FAN_PIN, GPIO_MODE_OUTPUT);

    // 2. Buttons Setup
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << LIGHT_BTN) | (1ULL << FAN_BTN) | (1ULL << DOOR_BTN),
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);

    // 3. Servo, I2C & Queue Setup
    servo_init();
    i2c_master_init();
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // 4. Tasks Creation
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
    xTaskCreate(dht_task, "dht_task", 2048, NULL, 5, NULL);
    xTaskCreate(rtc_task, "rtc_task", 2048, NULL, 5, NULL);

    // 5. Interrupts Setup
    gpio_install_isr_service(0);
    gpio_isr_handler_add(LIGHT_BTN, gpio_isr_handler, (void*) LIGHT_BTN);
    gpio_isr_handler_add(FAN_BTN, gpio_isr_handler, (void*) FAN_BTN);
    gpio_isr_handler_add(DOOR_BTN, gpio_isr_handler, (void*) DOOR_BTN);

    printf("System Started Successfully!\n");
}