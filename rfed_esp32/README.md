# MESH MQTT BLE Example

## Overview
Keywords: ESP32, ESP_MDF; BLE; MQTT;

This example shows how to exchange messages via MQTT MESH network among the ESP32 devices and the gateway(MQTT broker and AP). 
The messages are configured to be sent whenever a BLE beacon of a specific UUID is detected.

## Hardware

1. At least two ESP32 development boards
2. One 2.4 G gateway running MQTT broker (e.g. Raspberry Pi)
