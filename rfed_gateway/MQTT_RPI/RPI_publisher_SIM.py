import paho.mqtt.client as PahoMQTT
from MQTT_RPI.useful_functions import *
import time


class MyPublisher:
    def __init__(self, clientID):
        self.clientID = clientID

        # create an instance of paho.mqtt.client
        self._paho_mqtt = PahoMQTT.Client(self.clientID, False)
        # register the callback
        self._paho_mqtt.on_connect = self.myOnConnect

        # self.messageBroker = 'iot.eclipse.org'
        self.messageBroker = 'localhost'

    def start(self):
        # manage connection to broker
        self._paho_mqtt.connect(self.messageBroker, 1883)
        self._paho_mqtt.loop_start()

    def stop(self):
        self._paho_mqtt.loop_stop()
        self._paho_mqtt.disconnect()

    def myPublish(self, topic, message):
        # publish a message with a certain topic
        self._paho_mqtt.publish(topic, message, 2)

    def myOnConnect(self, _paho_mqtt, userdata, flags, rc):
        print("Connected to %s with result code: %d" % (self.messageBroker, rc))


if __name__ == "__main__":
    test = MyPublisher("MyPublisher")
    test.start()
    room_list = ["Triage", "Surgery", "Labs", "Examination", "Rianimation", "Pediatric", "Waiting"]
    MACS = ["60b943ec7821", "725d5e0e07ed", "49f8721e9ae1", "b43969f89e21"]
    T_F = ["True", "False"]
    t = 0
    while (t <= 200):
        msg = {"MAC_ADDRESS": MACS[random.randint(0, len(MACS) - 1)].upper(),
               "room": room_list[random.randint(0, len(room_list) - 1)],
               "time": t,
               "detected": T_F[random.randint(0, len(T_F) - 1)]}
        msg = json.dumps(msg)
        print("Publishing %s" % msg)
        # test.myPublish('Room1/gas_sensor', message)
        test.myPublish("Interdisciplinary", msg)
        t += 1
        time.sleep(1)

    test.stop()
