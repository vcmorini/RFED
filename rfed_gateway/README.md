# rfed_gateway

## Overview
Keywords: Raspberry Pi 3B+; Gateway; MQTT Broker; MongoDB

This example shows how to exchange messages between the gateway(MQTT broker and AP) and the MQTT MESH network root ESP32 node.

## Hardware

1. At least two ESP32 development boards
2. One 2.4 G gateway running MQTT broker (e.g. Raspberry Pi)

## Available Codes

The codes in this folder represent functions that will run on the raspberry pi in order to fill and update the database.
The scripts work in the following way:

- "MQTT_RPI/RPI_Publisher_SIM.py" publishes messages to the broker simulating patients that move inside the ER.
   The messages have this format: {"MAC_ADDRESS": ###, "room": ###, "time": ##, "detected": True/False}.
   Obviously the MAC_ADDRESS will be randomly taken from a list of possible addresses. These addresses should
   be present in the MAC_IDs collection, otherwise the subscriber will not recognize the patient.

- "MQTT_RPI/RPI_Subscriber.py" is the subscriber that will run on the rpi, receiving messages from the ESP-mesh network.
   Each received message must have the format: 
   {"MAC_ADDRESS": ###, "room": ###, "time": ##, "detected": True/False}, according to the mac address the
   patient id will be identified and some control functions will be performed to correctly fill and update the database.
   
- "WebInterface/Server.py" is the Web Interface between the final user (the hospital personnel) and the database, in order to add new patients in the database or to remove patients that are no longer in the health facility.
