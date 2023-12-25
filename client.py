import glob
import os
import sys

import math
import random
import time
import numpy as np
import cv2
import open3d as o3d
from matplotlib import cm
import numpy as np
import carla
import math

def gnss_callback(data, name):
    if name == "vehicle":
        main_vehicle_coordinates = convert_gps_to_carla(data)
        rounded_main_vehicle_coordinates = np.round(main_vehicle_coordinates, 1)
        print(f"Main Vehicle: {rounded_main_vehicle_coordinates}")
    else:
        vehicle1_coordinates = convert_gps_to_carla(data)
        rounded_vehicle1_coordinates = np.round(vehicle1_coordinates, 1)
        print(f"Vehicle1: {rounded_vehicle1_coordinates}")


 
# The (modified) method is taken from this repository:
#       https://github.com/autonomousvision/carla_garage/
def convert_gps_to_carla(gnss_measurement):
    # for CARLA 9.10
    mean = np.array([0.0, 0.0])
    scale = np.array([111324.60662786, 111319.490945])

    # Extract relevant components from GnssMeasurement
    lat = gnss_measurement.latitude
    lon = gnss_measurement.longitude

    # Normalize and scale GPS coordinates
    normalized_gps = (np.array([lat, lon]) - mean) * scale

    # Convert from GPS to CARLA coordinates (90° rotation)
    carla_coordinates = np.array([normalized_gps[1], -normalized_gps[0]])

    return carla_coordinates

import time

client = carla.Client('localhost', 2000)
client.set_timeout(1.0)
#client.load_world('TOWN06')
time.sleep(1)
world = client.get_world()
bp_lib = world.get_blueprint_library()
spawn_points = world.get_map().get_spawn_points()

try:

    y1_position = 237.5
    y2_position = 244.5
    y3_position = 251.5
    x_position = 106
    z_position = 0.300000
    pitch = 0.000000
    yaw = 0.019546
    roll = 0.000000
    vehicle_bp = bp_lib.filter('model3')[0]
    spectator = world.get_spectator()
    transform = carla.Transform(carla.Location(x=130, y=y2_position, z=62), carla.Rotation(pitch=-90, yaw=0, roll=0))
    spectator.set_transform(transform)

    transform = carla.Transform(carla.Location(x=x_position, y=y2_position, z=z_position),
                                carla.Rotation(pitch=pitch, yaw=yaw, roll=roll))

    x_offsets = [0, 5, 12, 20, 25, 33, 43, 49]
    vehicle_list = []
    for x_offset in x_offsets:
        current_transform = carla.Transform(
            carla.Location(x=x_position + x_offset, y=y3_position, z=z_position),
            carla.Rotation(pitch=pitch, yaw=yaw, roll=roll)
        )
        vehicle = world.spawn_actor(vehicle_bp, current_transform)
        vehicle_list.append(vehicle)


    vehicle = world.spawn_actor(vehicle_bp, transform)

    gnss_bp = bp_lib.find('sensor.other.gnss')
    gnss_bp.set_attribute('sensor_tick', '1.0')
    
    gnss_sensor = world.spawn_actor(gnss_bp, carla.Transform(), attach_to=vehicle)
    gnss_sensor1 = world.spawn_actor(gnss_bp, carla.Transform(), attach_to=vehicle_list[0])
   
   # vehicle_list[0].set_attribute("name", "vehicle")
    gnss_sensor.listen(lambda data: gnss_callback(data, "vehicle"))
    gnss_sensor1.listen(lambda data: gnss_callback(data, "vehicle1"))
 
    vehicle.apply_control(carla.VehicleControl(throttle=1.0, steer=0.0))


    time.sleep(10)



finally:
    for actor in world.get_actors().filter('*vehicle*'):
        actor.destroy()
    for actor in world.get_actors().filter('*sensor*'):
        actor.destroy()




