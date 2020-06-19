import paho.mqtt.client as PahoMQTT
import time
from datetime import datetime
from useful_functions import *
import pymongo as pm


class MySubscriber:
	def __init__(self, clientID):
		self.clientID = clientID
		# create an instance of paho.mqtt.client
		self._paho_mqtt = PahoMQTT.Client(clientID, False)

		# register the callback
		self._paho_mqtt.on_connect = self.myOnConnect
		self._paho_mqtt.on_message = self.myOnMessageReceived

		# Subscribe to a topic
		self.topic = '#'
		self.messageBroker = 'localhost'  # Message broker IP

		# Connection to MongoDB
		client = pm.MongoClient("localhost", 27017)
		print("Connected to the database")
		db = client["Patients"]
		self.activePatients = db["RTPatients"]
		self.permanentPatients = db["permanentPatients"]
		self.mac_ids = db["MAC_IDs"]
		# self.activePatients.delete_many({})
		# self.permanentPatients.delete_many({})

	def start(self):
		# manage connection to broker
		self._paho_mqtt.connect(self.messageBroker, 1883)
		self._paho_mqtt.loop_start()
		# subscribe for a topic
		self._paho_mqtt.subscribe(self.topic, 2)

	def stop(self):
		self._paho_mqtt.unsubscribe(self.topic)
		self._paho_mqtt.loop_stop()
		self._paho_mqtt.disconnect()

	def myOnConnect(self, paho_mqtt, userdata, flags, rc):
		print("Connected to %s with result code: %d" % (self.messageBroker, rc))

	def myOnMessageReceived(self, paho_mqtt, userdata, msg):
		# A new message is received
		message = json.loads(msg.payload)
		# Insert the current time
		now = round(time.time())
		print("New message received!: ", message)

		try:
			# Associate the MAC address with the patient unique id
			unique_id = self.mac_ids.find({"MAC": message["data"]["MAC"]}).next()["_id"]
			# Add the current time and date
			current_doc = {"patient_id": unique_id, "room": message["data"]["room"], "time": now, "date": datetime.fromtimestamp(now)}
			# If the beacon is detected, try to insert the detected patient in the active patients
			if message["data"]["detected"] == "1":
				# Check if that same patient is not present in other rooms
				already_active = list(self.activePatients.find({"patient_id": current_doc["patient_id"]}))

				# if the patient is already present then check the room, if the room is the same do nothing,
				# otherwise remove that patient from active and add it to permanent
				if len(already_active) >= 1 and already_active[0]["room"] != current_doc["room"]:
					print("The patient was active in another room.")
					self.activePatients.delete_many({"patient_id": current_doc["patient_id"]})
					print("Previous patient deleted.")
					permanent_doc = already_active[0]
					permanent_doc["duration"] = current_doc["time"] - permanent_doc["time"]
					self.permanentPatients.insert_one(permanent_doc)
					print("New permanent patient added.")
					self.activePatients.insert_one(current_doc)
					print("New active patient added.")

				elif len(already_active) == 0:
					self.activePatients.insert_one(current_doc)
					print("New active patient added.")
				else:
					print("Patient left as it was")

			# If the detected fiels is 0, then remove the patient from active patients and insert a new permanent.
			else:
				# Remove that patient from active db
				# If the patient is already absent from active db than do nothing
				# Add the new instance on permanent db
				already_active = list(self.activePatients.find({"patient_id": current_doc["patient_id"],
																"room": current_doc["room"]}))

				if len(already_active) >= 1:
					print("Patient left the current room.")
					self.activePatients.delete_many({"patient_id": current_doc["patient_id"]})

					permanent_doc = already_active[0]
					permanent_doc["duration"] = current_doc["time"] - permanent_doc["time"]
					self.permanentPatients.insert_one(permanent_doc)
		except StopIteration:
			print("MAC address not present in the database!", message["data"]["MAC"])


if __name__ == "__main__":
	test = MySubscriber("RaspberrySubscriberIP")
	test.start()

	a = 0
	while a < 200:
		time.sleep(10)
		a += 1

	test.stop()
