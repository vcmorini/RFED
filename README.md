# RFED Project
Indoor Real-Time Patient Location System designed for health facilities. The goal was to track patients in real-time within the infrastructure thus detecting its bottlenecks.

[YouTube - RFED Teaser](https://youtu.be/SeuhSV6cX2k)

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
* [Lorenzo Bellone](https://github.com/LorenzoBellone)
* [Victor de Castro Morini](https://github.com/vcmorini)

# Repositories
* rfed_esp32, contains the codes for the ESP-32 mesh network implementation and the beacons detection.
* rfed_gateway, contains the codes for the interface between the messages sent by the mesh network and the gateway that acts like a subscriber, filling the database (MongoDB).

