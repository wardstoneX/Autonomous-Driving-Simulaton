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


import time

client = carla.Client('localhost', 2000)

world = client.get_world()

original_settings = world.get_settings()

settings = world.get_settings()
settings.substepping = True
settings.max_substep_delta_time = 0.001
settings.max_substeps = 100


settings.fixed_delta_seconds = 0.01
world.apply_settings(settings)

# We set CARLA syncronous mode
#ettings.fixed_delta_seconds = 0.05
#ettings.synchronous_mode = True
#world.apply_settings(settings


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

    
    x_offsets = [-4,  9]
    vehicle_list = []
    vehicle_index = 0
    for x_offset in x_offsets:
        current_transform = carla.Transform(
            carla.Location(x=x_position + x_offset, y=248, z=z_position),
            carla.Rotation(pitch=pitch, yaw=yaw, roll=roll)
        )
        vehicle_bp.set_attribute('role_name', f'vehicle_{vehicle_index}')
        vehicle = world.spawn_actor(vehicle_bp, current_transform)
        vehicle_list.append(vehicle)
        vehicle_index += 1

    vehicle_bp.set_attribute('role_name', 'main_vehicle')
    vehicle = world.spawn_actor(vehicle_bp, transform)
    
    time.sleep(10)
    
    #vehicle.apply_control(carla.VehicleControl(manual_gear_shift=True, gear=1))
    #vehicle.apply_control(carla.VehicleControl(manual_gear_shift=False))
    #print(vehicle.get_location())
    vehicle.apply_control(carla.VehicleControl(throttle=0.4, steer=0.0))
   
    time.sleep(14)
                    
    vehicle.apply_control(carla.VehicleControl(throttle=0.0, brake=1))
    time.sleep(1)
    loc1 = vehicle.get_location().x
    
    ### lane change begin
    #vehicle.apply_control(carla.VehicleControl(throttle=0.4, brake=0, steer=0.3))
    
    #time.sleep(6)
   
    #vehicle.apply_control(carla.VehicleControl(throttle=0.4, brake=0, steer=-0.3))
    #time.sleep(4)
    #vehicle.apply_control(carla.VehicleControl(throttle=0.0, brake=0, steer=1.0))
    
    vehicle.apply_control(carla.VehicleControl(throttle=0.3, brake=0, steer=0.6,reverse=True))
    time.sleep(8.25)
    vehicle.apply_control(carla.VehicleControl(throttle=0.2, brake=0, steer=-0.6,reverse=True))
    time.sleep(5)
    vehicle.apply_control(carla.VehicleControl(throttle=0.0, brake=1, steer=0.0,reverse=True))

    time.sleep(15)
    print(vehicle.get_location())



finally:
  
    world.apply_settings(original_settings)
    for actor in world.get_actors().filter('*vehicle*'):
        actor.destroy()
   



