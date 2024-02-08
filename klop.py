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
import select

import threading
HOST = "10.149.55.108"
PORT = 6061


import threading
import time

gnss_data_lock = threading.Lock()
radar_data_lock = threading.Lock()

gnss_data_points = []
radar_data_points = []

MAX_DATA_POINTS = 20
SEND_INTERVAL = 1






from collections import namedtuple

ControlData = namedtuple('ControlData', 'throttle steer brake reverse time')


print(f"Connected to server")





def radar_measurement_to_cartesian(radar_position, detection_tuple):
    x0, y0, z0 = radar_position
    altitude, azimuth, depth = detection_tuple

    # Calculate Cartesian coordinates
   # x = depth * math.sin(altitude) * math.cos(azimuth) + x0
   # y = depth * math.sin(altitude) * math.sin(azimuth) + y0
   #z = depth * math.cos(altitude) + z0
   
    fw_vec = carla.Vector3D(x=depth - 0.25)
    radar_rotation = carla.Rotation(pitch=5, yaw=90, roll=0)
    sensor_transform = carla.Transform(location=carla.Location(x0,y0,2), rotation=radar_rotation)

    location = sensor_transform.location + fw_vec

    x = location.x
    y = location.y
    z = location.z
    
    azi = math.degrees(detection_tuple[1])
    alt = math.degrees(detection_tuple[0])
        
    fw_vec = carla.Vector3D(x=detect.depth - 0.25)
    
    carla.Transform(
            carla.Location(),
            carla.Rotation(
                pitch=current_rot.pitch + alt,
                yaw=current_rot.yaw + azi,
                roll=current_rot.roll)).transform(fw_vec)

    location_ = radar_data.transform.location + fw_vec
       
    world.debug.draw_point(
            radar_data.transform.location + fw_vec,
            size=0.075,
            life_time=0.06,
            persistent_lines=False,
            color=carla.Color(0,0,0))

    return (x, y, z)

def send_to_server(data1, data2):
    #print("Sending data to server:", len(data1), len(data2)) 
        
    data_str = b""
    
    for i in range(len(data1)):
        radar = data1[i]
        gnss = data2[i]
        if data1[i] != (0,0,0):
            radar = radar_measurement_to_cartesian(gnss, radar)
          
        combined_data = f"{radar}-{gnss}"
        combined_data = combined_data.replace('(', '').replace(')', '') 
        
        encoded_data = combined_data.encode('utf-8')
        packed_size = struct.pack('>i', len(encoded_data))
        data_to_send = b"\x13" + packed_size + encoded_data + b"\x14"
        data_str += data_to_send
        
    print(data_str)
    
 

def gnss_callback(data):
    
    global gnss_data_points
    coordinates = convert_gps_to_carla(data)
    #print(coordinates)
    with gnss_data_lock:
        gnss_data_points.append(coordinates)
    
    #send_gnss_data(coordinates)
    
def process(radar_list, gnss_list, last_detected_vehicle):
    current_location = carla.Location(gnss_list[-1][0], gnss_list[-1][1], gnss_list[-1][2])
    last_list = radar_list[-1]
    if not (radar_list and radar_list[-1][0] != (0,0,0)):
        return
    
    if last_detected_vehicle['main'] == (0,0,0) and last_detected_vehicle['other'] == (0,0,0):
        last_detected_vehicle['other'] = last_list[0]
        last_detected_vehicle['main'] = gnss_list[-1]
        print("vehicle detected")
        return
    location1 = carla.Location(last_detected_vehicle['other'][0], last_detected_vehicle['other'][1], last_detected_vehicle['other'][2])
    location2 = carla.Location(last_list[0][0], last_list[0][1], last_list[0][2])
    
    distance = location1.distance(location2)
    print(f"GNSS difference: {distance}")
    
    world.debug.draw_line(current_location, location2, thickness=0.1, color=carla.Color(0,0,0), life_time=5)
    
    
    if distance > 4:
        if distance > 12: 
            print("vehcle stopped")
            return
            
            
        last_detected_vehicle['other'] = last_list[0]
        last_detected_vehicle['main'] = gnss_list[-1]
        print("new vehicle detected {}, {}".format(last_list[0], gnss_list[-1]))
        return
     

    

def radar_callback(radar_data):
    global radar_data_points
  
    list = []
    
    current_rot = radar_data.transform.rotation
    for detect in radar_data:
        
        list.append((detect.altitude, detect.azimuth, detect.depth))
    
    if list:
      list = list[0]
    else:
        list = (0,0,0)
        
    with radar_data_lock:
        radar_data_points.append(list)    
    


stop_thread = threading.Event()

def periodically_send_data():
    while not stop_thread.is_set():
        time.sleep(SEND_INTERVAL)
        send_accumulated_data()
        
def send_accumulated_data():
    global radar_data_points, gnss_data_points
    
    with radar_data_lock, gnss_data_lock:
        data1_chunk = radar_data_points[:MAX_DATA_POINTS]
        radar_data_points = radar_data_points[MAX_DATA_POINTS:]

        data2_chunk = gnss_data_points[:MAX_DATA_POINTS]
        gnss_data_points = gnss_data_points[MAX_DATA_POINTS:]

        # Check if both data1_chunk and data2_chunk are full
        if data1_chunk and data2_chunk and len(data1_chunk) == len(data2_chunk):
            send_to_server(data1_chunk, data2_chunk)
        else:
            # If either chunk is empty, restore the data points lists
            radar_data_points = data1_chunk + radar_data_points
            gnss_data_points = data2_chunk + gnss_data_points





send_data_thread = threading.Thread(target=periodically_send_data)
send_data_thread.start()


secondReceived = False
order = 0

class ControlDataReceiver(threading.Thread):
    def __init__(self, connection_socket, control_data_queue):
        super().__init__()
        self.connection_socket = connection_socket
        self.control_data_queue = control_data_queue
        self.format = 'fffif'  # 4 floats, 1 bool, 1 int
        self.tuple_size = struct.calcsize(self.format)
        self.leftover_data = b''
        self.stop_event = threading.Event()

    def run(self):
        global order
        while not self.stop_event.is_set():
            data = self.connection_socket.recv(1024)
            data = self.leftover_data + data
            self.leftover_data = b''

            while len(data) >= self.tuple_size:
                tuple_data = data[:self.tuple_size]
                data = data[self.tuple_size:]

                control_data = struct.unpack(self.format, tuple_data)
                control_data = ControlData(*control_data)

                self.control_data_queue.put(control_data)
                if order == 1:
                    stop_thread.set()
                order += 1

            if data:
                self.leftover_data = data

    def stop(self):
        self.stop_event.set()
import queue

control_data_queue = queue.Queue()

# Start a new thread to receive control data
receiver = ControlDataReceiver(connection_socket, control_data_queue)
receiver.start()

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

world = client.get_world()

settings = world.get_settings()



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
    print("setting spectator transform")
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
    
    print(f"The width of the vehicle is {vehicle.bounding_box.extent.y*2}.")


    gnss_bp = bp_lib.find('sensor.other.gnss')
    gnss_bp.set_attribute('sensor_tick', '0.1')
    gnss_sensor = world.spawn_actor(gnss_bp, carla.Transform(), attach_to=vehicle)

    radar_bp = bp_lib.find('sensor.other.radar')
    radar_bp.set_attribute('horizontal_fov', '35.0')
    radar_bp.set_attribute('vertical_fov', '25.0')
    radar_bp.set_attribute('points_per_second', '1500')
    radar_bp.set_attribute('sensor_tick', '0.1')
    radar_bp.set_attribute('range', '10.0')

    radar_rotation = carla.Rotation(pitch=5, yaw=90, roll=0)
    radar_init_trans = carla.Transform(carla.Location(x=2, z=2), radar_rotation)
    radar = world.spawn_actor(radar_bp, radar_init_trans, attach_to=vehicle)

    
    

    
    radar.listen(lambda data: radar_callback(data))
    gnss_sensor.listen(lambda data: gnss_callback(data))
    
    

    while True:
        
        if control_data_queue:
            control_data = control_data_queue.get()
            print(f"Applying control data with values: {control_data}")
            
            
            vehicle.apply_control(carla.VehicleControl(throttle=control_data.throttle,
                                                       steer=control_data.steer,
                                                       brake=control_data.brake,
                                                       reverse=control_data.reverse))
            if control_data.time > 0:
                print(f"Sleeping for {control_data.time} seconds")
                time.sleep(control_data.time)
                print("applying brake")
                vehicle.apply_control(carla.VehicleControl(throttle=0,
                                                       steer=0,
                                                       brake=1))
            
        
        
        

    


 
    
    time.sleep(50)


finally:
    stop_thread.set()  # Signal the thread to stop
    receiver.stop()
    send_data_thread.join()  # Wait for the thread to finish
    for actor in world.get_actors().filter('*vehicle*'):
        actor.destroy()
    for actor in world.get_actors().filter('*sensor*'):
        actor.destroy()



