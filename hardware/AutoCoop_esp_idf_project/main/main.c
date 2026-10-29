#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_rom_sys.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "mqtt_client.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "dht22.h"

static const char *TAG = "AUTOCOOP";

/* ------------------------------------------------------------------------------------------------------------- */
/* Pins & constants  using wokwi diagram but we use waterlevel sensor for water contaner not Ultra sonic one ;)  */
/* ------------------------------------------------------------------------------------------------------------- */
#define LIGHT_BTN        13
#define LIGHT_LED        19
#define FAN_BTN          32
#define FAN_PIN          18
#define DOOR_BTN         26
#define AP_BTN_PIN       15
#define DHT_PIN          14
#define SERVO_PIN        27
#define GAS_ADC_CH       ADC_CHANNEL_6   /* GPIO34 */
 
#define FOOD_TRIG_PIN    5
#define FOOD_ECHO_PIN    25
#define WATER_PWR_PIN    33              /* powers the water sensor only while reading */ 
#define WATER_ADC_CH     ADC_CHANNEL_7   /* GPIO35 (water level sensor)  */
 
#define I2C_SDA_PIN      21
#define I2C_SCL_PIN      22
#define I2C_FREQ_HZ      100000
#define DS1307_ADDR      0x68
#define I2C_TIMEOUT_MS   100
 
/* Setup hotspot (opened with the AP button) */
#define AP_SSID          "AutoCoop_Config"
#define AP_PASS          "12345678"      /* change this! :) */
 
/* Servo   */
#define SERVO_DUTY_BITS  LEDC_TIMER_13_BIT
#define SERVO_MIN_US     1000
#define SERVO_MAX_US     2000
#define DOOR_OPEN_ANGLE  90
#define DOOR_CLOSED_ANGLE 0
 
/* Fan hysteresis*/
#define GAS_HYST         100
#define TEMP_HYST        1.0f
 
#define ECHO_TIMEOUT_US  30000


/* --------------------------------- */
/*           NVS config              */
/* --------------------------------- */
#define COOP_CFG_VERSION 4
#define NVS_NAMESPACE    "storage"
#define NVS_KEY          "coop_cfg"
 
/* ------------------------------------------------------------------ */
/* Live state  struct                                                        */
/* ------------------------------------------------------------------ */
static struct {
    float temperature, humidity, food_cm;
    int   gas, water_raw;
    bool  food_low, water_low;
    bool  light_on, fan_on, door_open;
} st;
 


static void servo_set_angle(int angle)
{
    if (angle < 0)   angle = 0;
    if (angle > 180) angle = 180;
 
    uint32_t pulse_us = SERVO_MIN_US + (SERVO_MAX_US - SERVO_MIN_US) * angle / 180;
    uint32_t duty     = (pulse_us * (1u << SERVO_DUTY_BITS)) / 20000;  /* 20 ms period */
 
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
 
static void light_set(bool on)
{
    st.light_on = on;
    gpio_set_level(LIGHT_LED, on);
    ESP_LOGI(TAG, "Light %s", on ? "ON" : "OFF");
}
 
static void fan_set(bool on)
{
    st.fan_on = on;
    gpio_set_level(FAN_PIN, on);
    ESP_LOGI(TAG, "Fan %s", on ? "ON" : "OFF");
}
 
static void door_set(bool open)
{
    st.door_open = open;
    servo_set_angle(open ? DOOR_OPEN_ANGLE : DOOR_CLOSED_ANGLE);
    ESP_LOGI(TAG, "Door %s", open ? "OPEN" : "CLOSED");
}




void app_main(void)
{
    printf("Hello world!\n");
    
}
