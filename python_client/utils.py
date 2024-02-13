import numpy as np
import threading
import math
import carla            


def read_offsets_from_file(filename):
    """
    This method reads the vehicle offsets from a file and returns them as a list.
    """
    print("Scenario is loaded from file: ", filename)
    with open(filename, 'r') as f:
        line = f.readline()
        offsets = list(map(float, line.split('-')))
    return offsets


# The (modified) method is taken from this repository:
# https://github.com/autonomousvision/carla_garage/
# The method is under MIT License. See the above link for more information.
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
    
    return (float(carla_coordinates[0]), float(carla_coordinates[1]), 0.0)


def radar_measurement_to_cartesian(radar_position, detection_tuple):
    x0, y0, z0 = radar_position
    altitude, azimuth, depth = detection_tuple

    // we need to rotate the forward vector correctly here...its not working currently atm

    fw_vec = carla.Vector3D(x=depth - 0.25)
    radar_rotation = carla.Rotation(pitch=5, yaw=90, roll=0)
    sensor_transform = carla.Transform(location=carla.Location(x0, y0, 2), rotation=radar_rotation)

    location = sensor_transform.location + fw_vec

    x = location.x
    y = location.y
    z = location.z

    return (x, y, z)



class GNSSHandler:
    """
    This class is used to handle the GNSS data. It is used to store the GNSS data points into a list 
    and to lock the list when it is being accessed."""
    def __init__(self):
        self.gnss_data_points = [] 
        self.gnss_data_lock = threading.Lock()


    def gnss_callback(self, data):
        """
        This method is the callback function for the GNSS sensor on the main vehicle.
        It converts the GNSS data point to CARLA coordinates and stores it in the class list."""
        coordinates = convert_gps_to_carla(data)
        with self.gnss_data_lock:
            self.gnss_data_points.append(coordinates)
            
            
# https://github.com/carla-simulator/carla/blob/master/Docs/tuto_G_retrieve_data.md was used.

class RadarHandler:
    """
    This class is used to handle the Radar data. It is used to store the Radar data points into a list 
    and to lock the list when it is being accessed."""
    def __init__(self):
        self.radar_data_points = [] 
        self.radar_data_lock = threading.Lock()
     
    def radar_callback(self, radar_data):
        """
        This method is the callback function for the Radar sensor on the main vehicle. 
        It converts the Radar data points to Cartesian coordinates and stores them in the class list.
        Only the first detection is stored.
        """
        list = []
        current_rot = radar_data.transform.rotation
        
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
            list.append((location_.x, location_.y, location_.z))

        list = list[0] if list else (0, 0, 0)
        with self.radar_data_lock:
            self.radar_data_points.append(list)
                
                
               
