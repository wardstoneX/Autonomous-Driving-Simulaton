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

CURRENT_ROTATION = carla.Rotation(pitch=5, yaw=90, roll=0 )

def radar_measurement_to_cartesian(gps_position, detection_tuple):
    """
    This method converts the radar detection to Cartesian coordinates. The calcultation takes into account the main vehicle position gained by the gps sensor and the angle and depth measurements from the radar sensor."""
    x0, y0, z0 = gps_position
    depth, azimuth, altitude = detection_tuple
        
    azi = math.degrees(azimuth)
    alt = math.degrees(altitude)
    
    
    fw_vec = carla.Vector3D(x=depth - 0.25)
    
    carla.Transform(
        carla.Location(),
        carla.Rotation(
            pitch=CURRENT_ROTATION.pitch + alt,
            yaw=CURRENT_ROTATION.yaw + azi,
            roll=CURRENT_ROTATION.roll)).transform(fw_vec)

    car_location = carla.Location(x=x0, y=y0, z=z0)
    
    location_ =  car_location+ fw_vec          

    x = location_.x
    y = location_.y
    z = location_.z

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
        
        for detect in radar_data:
            list.append((detect.depth, detect.azimuth, detect.altitude))
            
        list = list[0] if list else (0, 0, 0)
        with self.radar_data_lock:
            self.radar_data_points.append(list)
                
                
               
