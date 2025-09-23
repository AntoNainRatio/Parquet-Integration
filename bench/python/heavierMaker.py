import os
import csv
import random
import pandas as pd
import pyarrow.parquet as pa
from khiops import core as kh

categories = ["Passenger", "Pedestrian"]
genders = ["Male", "Female"]
trip_reasons = ["Leisure", "Work", "None"]
safety_devices = ["SeatBelt", "Helmet", "ChildrenDevice"]
safety_used = ["Yes", "No", ""]
pedestrian_locations = ["OnLane<=OnSidewalk0mCrossing", "None"]
pedestrian_actions = ["Crossing", "Standing", "Walking", "None"]
pedestrian_companies = ["Alone", "Group", "Unknown"]

def generate_user(accident_id, vehicle_id):
    #category = random.choice(categories)
    category = categories[0]
    #gender = random.choice(genders)
    gender = genders[0]
    #trip_reason = random.choice(trip_reasons)
    trip_reason = trip_reasons[0]
    #device = random.choice(safety_devices)
    device = safety_devices[0]
    #device_used = random.choice(safety_used)
    device_used = safety_used[0]
    #birth_year = random.randint(1940, 2015)
    birth_year = 1940

    if category == "Passenger":
        return {
            "AccidentId": accident_id,
            "VehicleId": vehicle_id,
            "Seat": 4,
            "Category": category,
            "Gender": gender,
            "TripReason": trip_reason,
            "SafetyDevice": device,
            "SafetyDeviceUsed": device_used,
            "PedestrianLocation": "None",
            "PedestrianAction": "None",
            "PedestrianCompany": "Unknown",
            "BirthYear": birth_year,
        }
    else:  # Pedestrian
        return {
            "AccidentId": accident_id,
            "VehicleId": vehicle_id,
            "Seat": "",
            "Category": category,
            "Gender": gender,
            "TripReason": "None",
            "SafetyDevice": "Helmet",
            "SafetyDeviceUsed": safety_used[0],
            "PedestrianLocation": pedestrian_locations[0],
            "PedestrianAction": pedestrian_actions[0],
            "PedestrianCompany": pedestrian_companies[0],
            "BirthYear": birth_year,
        }

def generate_user_rdm(accident_id, vehicle_id):
    category = random.choice(categories)
    gender = random.choice(genders)
    trip_reason = random.choice(trip_reasons)
    device = random.choice(safety_devices)
    device_used = random.choice(safety_used)
    birth_year = random.randint(1940, 2015)

    if category == "Passenger":
        return {
            "AccidentId": accident_id,
            "VehicleId": vehicle_id,
            "Seat": random.randint(2, 9),
            "Category": category,
            "Gender": gender,
            "TripReason": trip_reason,
            "SafetyDevice": device,
            "SafetyDeviceUsed": device_used,
            "PedestrianLocation": "None",
            "PedestrianAction": "None",
            "PedestrianCompany": "Unknown",
            "BirthYear": birth_year,
        }
    else:  # Pedestrian
        return {
            "AccidentId": accident_id,
            "VehicleId": vehicle_id,
            "Seat": "",
            "Category": category,
            "Gender": gender,
            "TripReason": "None",
            "SafetyDevice": random.choice(["Helmet", "None"]),
            "SafetyDeviceUsed": random.choice(safety_used),
            "PedestrianLocation": random.choice(pedestrian_locations),
            "PedestrianAction": random.choice(pedestrian_actions),
            "PedestrianCompany": random.choice(pedestrian_companies),
            "BirthYear": birth_year,
        }

def makeHeavier(csv_file, toAddPerAccidentVehicle, output_file, chunksize=10000):
    """
    Make a csv file heavier by adding users into it
    
    Args:
        csv_file (str): path to the csv file
        toAddPerAccidentVehicule (int): number of user to create foreach vehicule in an accident
        chunksize (int, default=10 000): size of the chunk handled one after an other
    """
    first_chunk = True
    for chunk in pd.read_csv(csv_file, sep="\t", chunksize=chunksize, dtype_backend="pyarrow"):
        rows_to_write = []

        # Conserver les lignes originales
        for _, row in chunk.iterrows():
            rows_to_write.append(row.to_dict())

            # Générer n nouveaux utilisateurs
            for _ in range(toAddPerAccidentVehicle):
                new_user = generate_user(row["AccidentId"], row["VehicleId"])
                rows_to_write.append(new_user)

        # Écrire dans le fichier de sortie
        mode = "w" if first_chunk else "a"
        header = first_chunk
        with open(output_file, mode, newline="", encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=rows_to_write[0].keys(), delimiter="\t")
            if header:
                writer.writeheader()
            writer.writerows(rows_to_write)

        first_chunk = False

if __name__ == "__main__":
    csv_file = os.path.join(kh.get_samples_dir(),"AccidentsVeryHeavy","Users.txt")
    output_file = os.path.join(kh.get_samples_dir(),"AccidentsVeryHeavy","Users_veryheavy.txt")

    toAddPerAccidentVehicule = 1000
    chunksize = 10000

    makeHeavier(csv_file,
                toAddPerAccidentVehicule,
                output_file,
                chunksize,
                )