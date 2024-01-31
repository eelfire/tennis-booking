#!/usr/bin/env python3
# python script to generate testcases

import random
import csv
import sys

# number of players
N = 100

Genders = ["M", "F"]
Preferences = ["S", "D", "b", "B"]

last_start_time = 1
player_id = 1

# get filename from command line argument
filename = sys.argv[1]

# open file for writing
f = open(filename, "w")

# create the csv writer
writer = csv.writer(f)
writer.writerow(["player_id", "arrival_time", "gender", "preference"])

while player_id <= N:
    arrival_time = last_start_time + random.randint(0, 3)
    gender = Genders[random.randint(0, 1)]
    preference = Preferences[random.randint(0, 3)]

    writer.writerow([player_id, arrival_time, gender, preference])

    player_id += 1
    last_start_time = arrival_time

f.close()
