import pymongo as pm
from useful_functions import *


class DatabaseHandler(object):
    # This class present a set of methods that allow to create or remove patients from the database "Patients",
    # especially interacting with the collections "MAC_IDs" and "Patient_ID".
    def __init__(self):
        # Whenever the object is called, it creates a connection with the MongoDB database.
        mongo_client = pm.MongoClient("localhost", 27017)
        print("Connected to the database")
        db = mongo_client["Patients"]
        self.patient_id = db["Patient_ID"]
        self.mac_ids = db["MAC_IDs"]

    def generate(self, fname, lname, birth, mac):
        # The function generates allows to insert a new patient in the collection Patient_ID, by associating his
        # non-secure ID (obtained from name, surname and date of birth) with a very complex ID.
        outcome = {}
        fname = fname.lower()
        lname = lname.lower()
        mac = mac.lower()
        day = int(birth.split("/")[0])
        month = int(birth.split("/")[1])
        year = int(birth.split("/")[2])
        id = fname[0] + lname[0] + str(day + month) + str(year % month) + str(year % day)  # Easy ID

        # If the patient is already associated with an ID, then nothing is updated, since the same patient will always
        # be associated with the same id.
        if self.patient_id.find({"Patient": id}).count() != 0:
            unique_id = self.patient_id.find({"Patient" : id}).next()["_id"]
            print("Patient already present in our database. Unique ID is: " + unique_id)
            outcome["result"] = "Patient already present with Unique ID: " + unique_id
            outcome["code1"] = 200

        # In the case in which a new patient arrives, then the unique id is generated.
        else:
            unique_id = generate_unique_id()
            while self.patient_id.find({"_id" : unique_id}).count() != 0:
                unique_id = generate_unique_id()
            doc = {"_id" : unique_id, "Patient" : id}
            self.patient_id.insert_one(doc)
            outcome["result"] = "New patient correctly added with Unique ID: " + unique_id
            outcome["code"] = 200
            print("New patient detected, new unique ID generated: " + unique_id)
        if mac == "?":
            mac = generate_mac()

        # The patient now must be associated also with a MAC address, that will change every time (according to the
        # beacon provided)
        if len(mac) == 12:  # a check on the mac
            if len(list(self.mac_ids.find({"_id": unique_id}))) != 0:
                self.mac_ids.delete_many({"_id": unique_id})
            elif len(list(self.mac_ids.find({"MAC": mac}))) != 0:
                self.mac_ids.delete_many({"MAC": mac})
            doc = {"MAC": mac, "_id": unique_id}
            self.mac_ids.insert_one(doc)
        else:
            outcome["result"] = "Input mac format error"
            outcome["code"] = 500
        return outcome

    def delete(self, fname, lname, birth):
        # In this method, a patient is de-associated with a MAC address, when he leaves the hospital. The corresponding
        # document will be totally canceled. The collection "Patiend_ID" will never be changed in this case, since the
        # patient will always be associated with the same unique id.

        # Firstly, we obtain the easy id in order to derive the complex unique id.
        outcome = {}
        fname = fname.lower()
        lname = lname.lower()
        day = int(birth.split("/")[0])
        month = int(birth.split("/")[1])
        year = int(birth.split("/")[2])
        id = fname[0] + lname[0] + str(day + month) + str(year % month) + str(year % day)
        # When the complex unique id is found, we can erase the association between unique id and MAC address from the
        # colleciton "MAC_IDs".
        if len(list(self.patient_id.find({"Patient": id}))) != 0:
            unique_id = self.patient_id.find({"Patient": id}).next()["_id"]
            self.mac_ids.delete_one({"_id": unique_id})
            outcome["result"] = "Patient correctly deleted"
            outcome["code"] = 200
        else:
            outcome["result"] = "No patients detected with this information"
            outcome["code"] = 200
        return outcome
