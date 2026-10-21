# AutoCoop Mini

**A Small-Scale Modular IoT Poultry Automation and Environmental Monitoring System**

AutoCoop Mini is a low-cost, scalable, and modular telemetry ecosystem built specifically for small-scale poultry pens, backyard coops, and hobbyist farmers. Designed to alleviate the heavy physical labor and constant manual supervision required in traditional poultry keeping, AutoCoop Mini automates microclimate control, feeding/watering alerts, and coop security. 

The system operates on an offline-first architecture powered by an **ESP32 Microcontroller running native ESP-IDF v6.1**, featuring local Non-Volatile Storage (NVS) persistence, an on-board Wi-Fi Access Point configuration portal, and **Node-RED & MQTT** integration for real-time telemetry and remote control.

---

## Problem Statement

Small-scale and backyard poultry farmers face significant challenges in maintaining optimal coop conditions manually. Lack of continuous monitoring can lead to critical failures such as:
- Irregular feeding and watering schedules.
- Rapidly deteriorating air quality (ammonia buildup) and extreme temperature/humidity fluctuations.
- High labor intensity and requirement for constant physical presence on-site.

Existing commercial automation systems target large-scale industrial poultry operations, making them far too expensive, overly complex, and impractical for small coops or micro-scale operations. **AutoCoop Mini** bridges this gap by offering a scalable, affordable, and easy-to-deploy hardware/software solution tailored for micro-farms.

---

## Key Features & Architecture

- **Environmental & Air Quality Telemetry:** Continuous tracking of ambient temperature, humidity, and hazardous gas/ammonia concentration levels via MQ-135 and digital sensors.
- **Automated Door Management:** Servo-driven coop door control triggered by real-time clock (RTC) schedules or manual overrides.
- **Smart Ventilation & Lighting:** Relay-controlled 12V DC ventilation fans and RTC-scheduled artificial lighting loops.
- **Feed & Water Level Monitoring:** Ultrasonic range sensors track food and water reservoir levels, triggering real-time local warnings.
- **Offline-First Local Fail-Safe:** Smart local FreeRTOS C++ threshold loops on the ESP32 ensure critical actuators (fans, heaters, doors) continue running safely even during local network or Wi-Fi outages.
- **Dual Configuration & Control:** 
  - **AP Mode Portal:** Pressing the physical AP button (GPIO 15) broadcasts an `AutoCoop_Config` Wi-Fi network with an embedded web server (`192.168.4.1`) for direct threshold adjustments saved to NVS.
  - **Node-RED & MQTT:** Real-time telemetry streaming and remote control via local MQTT brokers and Node-RED dashboards.

---

## Tech Stack & Architecture

### **Hardware & Sensors**
- **Primary Gateway / Core MCU:** ESP32 Microcontroller (ESP-IDF v6.1)
- **Temperature & Humidity:** DHT22 & DS18B20 (Waterproof digital probe)
- **Air Quality & Gas Sensing:** MQ-135 Gas Sensor (Ammonia / Air Quality via ADC1 Channel 6)
- **Level Measurement:** HC-SR04 Ultrasonic Distance Sensors (Food & Water reservoirs)
- **Timekeeping:** DS1307 / DS3231 RTC Module (I2C Master Bus API)
- **Actuators & Controls:** 5V/12V Relay Modules, 12V DC High-CFM Fans, High-Torque Servo Motor (Door Actuator via LEDC PWM 50Hz)

### **Firmware & Embedded Software**
- **Language / Framework:** C / C++ (ESP-IDF v6.1 Framework)
- **FreeRTOS Tasks:** Button debouncing, RTC scheduling, Gas/Air quality monitoring, Ultrasonic distance checks, and AP server routines.
- **Storage:** NVS (Non-Volatile Storage) for power-off threshold persistence.

### **Local Server & Dashboard Stack**
- **MQTT Broker:** Eclipse Mosquitto
- **Dashboard & Automation:** Node-RED (Local telemetry visualization and command routing)

---

## System Architecture & Flow
