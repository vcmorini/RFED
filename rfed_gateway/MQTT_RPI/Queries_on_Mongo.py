import pymongo as pm
# from matplotlib import pyplot as plt

client = pm.MongoClient("localhost", 27017)
db = client["Patients"]
perm = db["permanentPatients"]

tot_duration = perm.aggregate([
    {"$group" : {"_id":"$room", "duration":{"$avg":"$duration"}}}
])

tot_duration_array = list(tot_duration)
x = []
y = []

for i in tot_duration_array:
    x.append(i["_id"])
    y.append(round(i["duration"]/60))

print(x, y)
# plt.figure(1, figsize=(8, 5))
# plt.bar(x, y, width=0.6)
# plt.grid()
# plt.xlabel("Rooms")
# plt.ylabel("Average Waiting Times [minutes]")
# plt.show()