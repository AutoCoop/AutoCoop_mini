#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/queue.h"

#define LIGHT_BTN   13
#define LIGHT_LED   19
#define FAN_BTN     32
#define FAN_PIN     18
#define DOOR_BTN    26
#define DHT_PIN 14 

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

// 3. Door Function
void door_open_close(void) {
    if (is_door_open) {
        is_door_open = false;
        printf("Door is CLOSED\n");
       // to do: code for servo angle to 0
    } else {
        is_door_open = true;
        printf("Door is OPENED\n");
        // to do: code for servo angle to 90
    }
}



void app_main(void) {

}