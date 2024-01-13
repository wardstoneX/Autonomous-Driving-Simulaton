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

park_spot_found = False
def draw_on_location(location, time):
    world.debug.draw_string(location, 'O', draw_shadow=False,
                                    color=carla.Color(r=255, g=255, b=255), life_time=time,
                                    persistent_lines=True)

def gnss_callback(data, name):
    if name == "vehicle":
        main_vehicle_coordinates = convert_gps_to_carla(data)
        rounded_main_vehicle_coordinates = np.round(main_vehicle_coordinates, 1)
       # print(f"Main Vehicle: {rounded_main_vehicle_coordinates}")
    else:
        vehicle1_coordinates = convert_gps_to_carla(data)
        rounded_vehicle1_coordinates = np.round(vehicle1_coordinates, 1)
        #print(f"Vehicle1: {rounded_vehicle1_coordinates}")

detected_vehicles = {}

def obstacle_callback(event, dict):
    vehicle = event.actor.parent
    other_vehicle = event.other_actor
    distance = event.distance
    #print(event.distance)

    prev_distance = dict["distance"][-1]

    distance_difference = distance - prev_distance
   # print(event.distance, dict["distance"][-1] )
    #print(prev_distance, distance)
    dt = round(abs(distance_difference), 3)
    if dt > 0.1:
        print(round(abs(distance_difference), 3), other_vehicle.attributes['role_name'], distance_difference, event.distance, dict["distance"][-1])
        world.debug.draw_string(vehicle.get_location(), str(dt), color = carla.Color(r=0, g=0, b=0), life_time=60)
        print(event.timestamp)
        print("\n")
    
    #if(distance_difference )



    world.debug.draw_line(vehicle.get_location(), other_vehicle.get_location(), color=carla.Color(r=255, g=255, b=255), life_time=60)

    #world.debug.draw_string(vehicle.get_location(), str(event.distance), color = carla.Color(r=0, g=0, b=0), life_time=5)
    dict["distance"].append(distance)
       
       

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

map_name = client.get_world().get_map().name

if( map_name != 'Town06'):
    print(f"The loaded map is {map_name}.")
    print("Loading Town06...")
    client.set_timeout(20.0)
    client.load_world('TOWN06')
    time.sleep(10)
else:
    print("The loaded map is Town06.")
    client.set_timeout(5.0)



world = client.get_world()
#print(world.get_map().name)
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
    
    
    # [0, 5, 12, 20, 25, 33, 43, 49]
    x_offsets = [0, 5, 12, 20, 25, 33, 43, 49]
    vehicle_list = []
    vehicle_index = 0
    for x_offset in x_offsets:
        current_transform = carla.Transform(
            carla.Location(x=x_position + x_offset, y=y3_position, z=z_position),
            carla.Rotation(pitch=pitch, yaw=yaw, roll=roll)
        )
        vehicle_bp.set_attribute('role_name', f'vehicle_{vehicle_index}')
        vehicle = world.spawn_actor(vehicle_bp, current_transform)
        vehicle_list.append(vehicle)
        vehicle_index += 1

    
    vehicle_bp.set_attribute('role_name', 'main_vehicle')
    vehicle = world.spawn_actor(vehicle_bp, transform)
  #  vehicle1 = world.spawn_actor(vehicle_bp, transform1)
   # print(f"The length of the vehicle is {vehicle.bounding_box.extent.x*2}.")
    
    
    gnss_bp = bp_lib.find('sensor.other.gnss')
    gnss_bp.set_attribute('sensor_tick', '0.2')
    
    gnss_sensor = world.spawn_actor(gnss_bp, carla.Transform(), attach_to=vehicle)
    gnss_sensor1 = world.spawn_actor(gnss_bp, carla.Transform(), attach_to=vehicle_list[0])
   
   # vehicle_list[0].set_attribute("name", "vehicle")
    obstacle_bp = bp_lib.find('sensor.other.obstacle')
    obstacle_bp.set_attribute('hit_radius','5.0')
    obstacle_bp.set_attribute('distance','20.0')
    obstacle_bp.set_attribute("only_dynamics",str(True))

    #obstacle_bp.set_attribute('debug_linetrace',str(True))
    obstacle_bp.set_attribute('sensor_tick','0.07')
    
    obs_location = carla.Location(0,0,0)
    obs_rotation = carla.Rotation(pitch=0,yaw=90,roll=0)
    obs_transform = carla.Transform(obs_location,obs_rotation) 
    obstacle_sensor = world.spawn_actor(obstacle_bp, obs_transform, attach_to=vehicle)




    sensor_data = {'distance': [0]}
                

    obstacle_sensor.listen(lambda event: obstacle_callback(event, sensor_data))

   # gnss_sensor.listen(lambda data: gnss_callback(data, "vehicle"))
    #gnss_sensor1.listen(lambda data: gnss_callback(data, "vehicle1"))

    vehicle.apply_control(carla.VehicleControl(throttle=0.5, steer=0))
 
   # while not park_spot_found:
      #  
      #  vehicle.apply_control(carla.VehicleControl(throttle=0.3, steer=0.0))

   # print(park_spot_found, "Applying break")
    #vehicle.apply_control(carla.VehicleControl(throttle=0.0,brake=1.0))

    print(sensor_data)

    time.sleep(60)



finally:
    for actor in world.get_actors().filter('*vehicle*'):
        actor.destroy()
    for actor in world.get_actors().filter('*sensor*'):
        actor.destroy()




