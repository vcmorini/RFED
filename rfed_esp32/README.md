# RFED PROJECT - MESH MQTT BLE example on ESP32

## Overview
Keywords: ESP32, ESP_MDF; BLE; MQTT;

This example shows how to exchange messages via MQTT MESH network among the ESP32 devices and the gateway (MQTT broker and AP). 
The messages are configured to be sent whenever a BLE beacon of a specific UUID is detected.
For deeper technical information, please check Espressif documentation such as: https://github.com/espressif/esp-mdf

## Hardware
Requirements:
1. At least two ESP32 development board;
2. One 2.4 G gateway running MQTT broker (e.g. Raspberry Pi);
3. Eddystone UID beacons.


### Hardware ESP32-DevKitC
The ESP32-DevKitC development board from Espressif was the chosen hardware to be the bridge between the wearable BLE beacons and the gateway. This low-footprint low cost hardware is powered by the SoC ESP32, which provides 2.4 GHz Wi-Fi (full IEEE 802.11 b/g/n Wi-Fi MAC protocol), BLE specifications complient and security due to the embedded ATECC608A chip. 
The last can be used for enhanced security to connect to IoT cloud services that use X.509 based mutual authentication and TLS based secure communication.

On the software side, Espressif provides the ESP-IDF and the ESP-MDF which ease the prototype development.

Therefore, taking into consideration the technical requirements of the project, time to market, prototype design and cost, this uC was a great fit to detect the BLE beacons broadcasted by wearable devices, filter, and relay information to the gateway via Wi-Fi mesh network. 

### ESP32 Development Framework

In order to implement the communication between ESP32 devices and the gateway, RPi, the ESP-MESH network protocol is used. The software stack of this protocol is built atop the Wi-Fi Driver/FreeRTOS [ESP-MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-reference/network/esp_mesh.html), as depicted in the software stack diagram.

ESP-MESH is a multiple hop (multi-hop) network, allowing several ESP32 devices to be connected under the same WLAN even though if not connected directly nor in range of the AP, Figure. In addition, the ESP-MESH is self-organizing and self-healing, meaning the network can be built and maintained autonomously [MESH](https://docs.espressif.com/projects/esp-idf/en/stable/api-guides/mesh.html). Therefore, exploiting ESP-MESH, the system covers large areas maintaining its independence from the client's facility and avoiding network maintenance.

The implementation is done by exploiting ESP-MDF, based on the ESP-MESH network protocol software stack. ESP-MDF is built on top of ESP-IDF that essentially contains API (software libraries and source code) for ESP32 and scripts to operate the toolchain. ESP-IDF API is mainly composed of: Bluetooth (Classic, BLE, BLE-MESH), Networking (Wi-Fi, Ethernet, IP and Application Layer), Peripherals (I2C, SPI, ADC, DAC, CAN, UART, etc) and System (FreeRTOS, Watchdogs, Sleep Modes, OTA, etc) [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/index.html). 

The ESP-MDF contains the Mwifi API, a networking API, which encapsulates the Wi-Fi and ESP-MESH API, and add other functionalities [ESP-MDF](https://docs.espressif.com/projects/esp-mdf/en/latest/api-reference/mwifi/index.html). Mwifi is the main driver of the RFED project.

