

import numpy as np
import threading
import math

import carla            




def read_offsets_from_file(filename):
    print("scenario is loaded from file: ", filename)
    with open(filename, 'r') as f:
        line = f.readline()
        offsets = list(map(int, line.split('-')))
    return offsets


# The (modified) method is taken from this repository:
# https://github.com/autonomousvision/carla_garage/
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


def radar_measurement_to_cartesian(radar_position, detection_tuple):
    x0, y0, z0 = radar_position
    altitude, azimuth, depth = detection_tuple

    # Calculate Cartesian coordinates
    # x = depth * math.sin(altitude) * math.cos(azimuth) + x0
    # y = depth * math.sin(altitude) * math.sin(azimuth) + y0
    # z = depth * math.cos(altitude) + z0

    fw_vec = carla.Vector3D(x=depth - 0.25)
    radar_rotation = carla.Rotation(pitch=5, yaw=90, roll=0)
    sensor_transform = carla.Transform(location=carla.Location(x0, y0, 2), rotation=radar_rotation)

    location = sensor_transform.location + fw_vec

    x = location.x
    y = location.y
    z = location.z

    return (x, y, z)



class GNSSHandler:
    def __init__(self):
        self.gnss_data_points = [] 
        self.gnss_data_lock = threading.Lock()


    def gnss_callback(self, data):

        coordinates = convert_gps_to_carla(data)
        with self.gnss_data_lock:
            self.gnss_data_points.append(coordinates)
            
            


class RadarHandler:
    def __init__(self):
        self.radar_data_points = [] 
        self.radar_data_lock = threading.Lock()
     
    def radar_callback(self, radar_data):
        list = []
        current_rot = radar_data.transform.rotation
        #world = get_world()
        
        for detect in radar_data:
            azi = math.degrees(detect.azimuth)
            alt = math.degrees(detect.altitude)
            
            fw_vec = carla.Vector3D(x=detect.depth - 0.25)
            carla.Transform(
                carla.Location(),
                carla.Rotation(
                    pitch=current_rot.pitch + alt,
                    yaw=current_rot.yaw + azi,
                    roll=current_rot.roll)).transform(fw_vec)

            location_ = radar_data.transform.location + fw_vec

           # world.debug.draw_point(
               # radar_data.transform.location + fw_vec,
             #   size=0.075,
             #   life_time=0.06,
               # persistent_lines=False,
               # color=carla.Color(0, 0, 0))
            
            list.append((location_.x, location_.y, location_.z))

        list = list[0] if list else (0, 0, 0)
        #print(list)
        with self.radar_data_lock:
            self.radar_data_points.append(list)
            #print(list)
                
                
               