## Smart-HVAC-IoT-System
An Intelligent Temperature Control System with Hybrid Manual/Auto Logic and Cloud MQTT Integration. (Mockup system with Wokwi simulator)

## Project Demonstration

![Project_Demostration](docs/Hvac_simulation.gif)

## Overview
This project is Hvac control system using ESP32 as a processor. Hybird Control which can switch between automation and manual fan control from dashboard and button.


## Key Features
* Dual-Layer Control: 
    * Physical: Double Click on physical device (Debounced Logic).
    * Remote: Control through MQTT Dashboard on mobile phone.
* Hysteresis Algorithm: Automate fan control based on temperature (ON > 30°C, OFF < 28°C).
* Secure IoT Communication: HiveMQ Cloud MQTT Broker (SSL/TLS Encryption).
* Real-time Feedback: LCD blinking response to system feedback from variation.

## Hardware & Circuit
* MCU: ESP32 (NodeMCU)
* Sensor: DHT22 (Temperature & Humidity)
* Display: I2C LCD 1602
* Output: LED Indicator (Representing Fan Relay)

## Mobile Dashboard
Using [IoT MQTT Planel](https://play.google.com/store/apps/details?id=snr.lab.iotmqttpanel.prod) as a mobile dashboard.
* Gauge Widget: Temperature Real-time monitoring.
* Button: Toggle Mode switch between Manual and Automation.
* Fan Toggle: ON/OFF switch in manual mode.

## Tech Stack & Libraries
* Language: C++ (Arduino Framework)
* IDE: PlatformIO
* Libraries: 
    * PubSubClient (MQTT Communication)
    * ArduinoJson (Data Serialization)
    * DHT sensor library
    * LiquidCrystal I2C

##  Challenges & Solutions
* Challenge: LCD blinking without blocking while runing the system.
    * Solution: Change from delay() to machine state with millis(). Result, the LCD can blinking while reading data from sensors and sending to MQTT. 

* Challenge: Limitation on toggle switch (widgets) cause no realtime response on switch.
    * Solution: Adding Status Feedback System by sending realtime fan status in Topic status to display as LED light on dashboard.

##  Project Links
* Simulation: [Wokwi Project Link](https://wokwi.com/projects/459270536350682113)
* Broker: [HiveMQ Cloud](https://www.hivemq.com/agent/)