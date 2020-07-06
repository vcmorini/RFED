# RFED Project
Indoor Real-Time Patient Location System designed for health facilities, in order to track patients in real-time within the infrastructure and detect the bottlenecks.

# Hardwares
* [Global Tag](https://www.global-tag.com/it/) BLE Development Kit 
* ESP32-DevKitC
* Raspberry Pi 3B+
# Software
* Python
* FreeRTOS
* ESP-IDF
* ESP-MDF
* MongoDB
# Authors
* Anastasya Isgandarova
* Cansu Ilter
* Lorenzo Bellone
* Victor de Castro Morini

# Repositories
* rfed_esp32, contains the codes for the ESP-32 mesh network implementation and the beacons detection.
* rfed_gateway, contains the codes for the interface between the messages sent by the mesh network and the gateway that acts like a subscriber, filling the database (MongoDB).
