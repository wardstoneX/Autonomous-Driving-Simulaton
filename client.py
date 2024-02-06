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
import pdb

import socket
import time
import concurrent.futures
import sys

import struct

import threading
#LISTENER_ADDRESS = "172.17.0.1"
#LISTENER_PORT = 6000

#connection_socket1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#connection_socket1.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

#server_address = (LISTENER_ADDRESS, LISTENER_PORT)

#connection_socket1.bind(server_address)
#c#onnection_socket1.listen(5)

#print(f"Server listening on {server_address}...")

#connection_socket, client_address = connection_socket1.accept()
#connection_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)


id = 0

def send_data(data):
    #connection_socket.sendall(data)
    pass



def send_gnss_data(data):
    send_data_to_server(f"{round(data[0], 3)},{round(data[1], 3)},{round(data[2], 3)}", 0)


    
def send_radar_data(data):
    data_str = b""
    for  x, y, z in data:
        tmp = f"{round(x, 3)},{round(y, 3)},{round(z, 3)}"
        encoded_data = tmp.encode('utf-8')
        packed_size = struct.pack('>i', len(encoded_data))
        data_to_send = b"\x14" + packed_size + encoded_data
        data_str += data_to_send
    send_data_to_server(data_str + b"\x15", 1)
 

def send_data_to_server(data, type):
    if type == 0:
        encoded_data = data.encode('utf-8')
        #print(encoded_data, len(encoded_data))
        packed_size = struct.pack('>i', len(encoded_data))
        #print(packed_size)
        data_to_send = b"\x13" + packed_size + encoded_data
        #print(data_to_send)
        send_data(data_to_send)
    elif type == 1:
        send_data(data)


def gnss_callback(data, gnss_list):
    coordinates = convert_gps_to_carla(data)
    gnss_list.append(coordinates)
    #send_gnss_data(coordinates)
    
def process(radar_list, gnss_list, last_detected_vehicle):
    current_location = carla.Location(gnss_list[-1][0], gnss_list[-1][1], gnss_list[-1][2])
    last_list = radar_list[-1]
    if not (radar_list and radar_list[-1][0] != (0,0,0)):
        return
    
    if last_detected_vehicle['main'] == (0,0,0) and last_detected_vehicle['other'] == (0,0,0):
        last_detected_vehicle['other'] = last_list[0]
        last_detected_vehicle['main'] = gnss_list[-1]
        #print("vehicle detected")
        return
    location1 = carla.Location(last_detected_vehicle['other'][0], last_detected_vehicle['other'][1], last_detected_vehicle['other'][2])
    location2 = carla.Location(last_list[0][0], last_list[0][1], last_list[0][2])
    
    distance = location1.distance(location2)
    #print(f"GNSS difference: {distance}")
    
    world.debug.draw_line(current_location, location2, thickness=0.1, color=carla.Color(0,0,0), life_time=5)
    
    
    if distance > 4:
        if distance > 12: 
            vehicle = world.get_actors().find(id)
            vehicle.apply_control(carla.VehicleControl(throttle=0.0, steer=0, brake = 1))
            #print("vehcle stopped")
            sys.exit()
            
        last_detected_vehicle['other'] = last_list[0]
        last_detected_vehicle['main'] = gnss_list[-1]
        #print("new vehicle detected")
        return
     

    

def radar_callback(radar_data, radar_list, gnss_list, last_detected_vehicle):
    global radar_tick
    current_rot = radar_data.transform.rotation
    list = []
    current_location = carla.Location(gnss_list[-1][0], gnss_list[-1][1], gnss_list[-1][2])

    for detect in radar_data:
        
        azi = math.degrees(detect.azimuth)
        alt = math.degrees(detect.altitude)
        # The 0.25 adjusts a bit the distance so the dots can
        # be properly seen
        fw_vec = carla.Vector3D(x=detect.depth - 0.25)
    
        location_ = radar_data.transform.location + fw_vec
    
        
        
    if list:
        #send_radar_data(list)
        # Assuming 'list_of_tuples' is your list of tuples
        rounded_list = [(round(x, 2), round(y, 2), round(z, 2)) for x, y, z in list]


        unique_set = set(map(str, rounded_list))


        list = [tuple(map(float, s.strip('()').split(', '))) for s in unique_set]        
        
    else:
        list = [(0,0,0)]
        #send_radar_data(empty_list)
        
        #loc =  carla.Location(x=120, y=220, z=0.3)
    
    radar_list.append(list)
    print(list)
    process(radar_list, gnss_list, last_detected_vehicle)
    

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
    # print(carla_coordinates[0])
    return (float(carla_coordinates[0]), float(carla_coordinates[1]), 0.0)


import time

client = carla.Client('localhost', 2000)

map_name = client.get_world().get_map().name

if (map_name != 'Town06'):
    print(f"The loaded map is {map_name}.")
    print("Loading Town06...")
    client.set_timeout(20.0)
    client.load_world('TOWN06')
    time.sleep(10)
else:
    print("The loaded map is Town06.")
    client.set_timeout(5.0)

world = client.get_world()
# print(world.get_map().name)
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
    
    transform = carla.Transform(carla.Location(x=x_position- 6, y=y2_position, z=z_position),
                                carla.Rotation(pitch=pitch, yaw=yaw, roll=roll))

    # [0, 5, 12, 20, 25, 33, 43, 49]
    x_offsets = [0, 5, 12, 20, 25, 33, 48, 55]
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
    id = vehicle.id
    print(f"The length of the vehicle is {vehicle.bounding_box.extent.x*2}.")

    gnss_bp = bp_lib.find('sensor.other.gnss')
    gnss_bp.set_attribute('sensor_tick', '0.1')
    gnss_sensor = world.spawn_actor(gnss_bp, carla.Transform(), attach_to=vehicle)

    radar_bp = bp_lib.find('sensor.other.radar')
    radar_bp.set_attribute('horizontal_fov', '35.0')
    radar_bp.set_attribute('vertical_fov', '25.0')
    radar_bp.set_attribute('points_per_second', '1500')
    radar_bp.set_attribute('sensor_tick', '0.01')
    radar_bp.set_attribute('range', '10.0')

    radar_rotation = carla.Rotation(pitch=5, yaw=90, roll=0)
    radar_init_trans = carla.Transform(carla.Location(x=2, z=2), radar_rotation)
    radar = world.spawn_actor(radar_bp, radar_init_trans, attach_to=vehicle)

    
    sensor_data = {'gnss': [(0,0,0)], 'radar': [[(0,0,0)]]}
    last_detected_vehicle = {'main': (0,0,0), 'other': (0,0,0)}
    list = []

    radar.listen(lambda data: radar_callback(data, sensor_data['radar'], sensor_data['gnss'], last_detected_vehicle))
    gnss_sensor.listen(lambda data: gnss_callback(data, sensor_data['gnss']))
    
    vehicle.apply_control(carla.VehicleControl(throttle=0.2, steer=0))

    time.sleep(5)
    
    time.sleep(45)


finally:
    for actor in world.get_actors().filter('*vehicle*'):
        actor.destroy()
    for actor in world.get_actors().filter('*sensor*'):
        actor.destroy()




