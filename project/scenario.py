
import time
import carla


## it might be necessary to add timeouts while setting up stuff
class ScenarioSetup():
    Y1_POSITION = 237.5
    Y2_POSITION = 244.5
    Y3_POSITION = 251.5
    X_POSITION = 106
    Z_POSITION = 0.3
    PITCH = 0.0
    YAW = 0.02  #0.019546 
    ROLL = 0.0
    X_OFFSETS = [0, 5, 12, 20, 25, 33, 48, 55]
    
    def __init__(self, host_simulator, port_simulator, gnss_handler, radar_handler):
        self.client = carla.Client(host_simulator, port_simulator)
        self.world = self.client.get_world()
        self.map_name = self.world.get_map().name
        self.blueprint_library = self.world.get_blueprint_library()
        self.vehicle_blueprint = self.blueprint_library.filter('model3')[0]
        self.gnss_handler = gnss_handler
        self.radar_handler = radar_handler
        self.main_vehicle = None
        self.sensors = []
        self.vehicles = []
    
      
       

    def get_world(self):
        return self.world
    
    def clean_up(self):
        for actor in self.sensors:
            actor.destroy()
        for actor in self.vehicles:
            actor.destroy()
    
    def change_map(self, map_name):
        if map_name != 'Town06':
            raise ValueError("Invalid map name. Only 'Town06' is supported.")
        
        if (self.map_name != 'Town06'):
            print(f"The loaded map is {map_name}.\n Loading Town06...")
            self.client.set_timeout(30)
            self.client.load_world('TOWN06')
            time.sleep(5)
            
        else:
            print("The loaded map is Town06.")
            time.sleep(5)
            self.client.set_timeout(5.0)
    def sensor_setup(self):
        gnss_blueprint = self.blueprint_library.find('sensor.other.gnss')
        gnss_blueprint.set_attribute('sensor_tick', '0.1')
        gnss_sensor = self.world.spawn_actor(gnss_blueprint, carla.Transform(), attach_to=self.main_vehicle)

        radar_blueprint = self.blueprint_library.find('sensor.other.radar')
        radar_blueprint.set_attribute('horizontal_fov', '35.0')
        radar_blueprint.set_attribute('vertical_fov', '25.0')
        radar_blueprint.set_attribute('points_per_second', '1500')
        radar_blueprint.set_attribute('sensor_tick', '0.1')
        radar_blueprint.set_attribute('range', '10.0')
        radar_transform = carla.Transform(carla.Location(x=2, z=2), carla.Rotation(pitch=5, yaw=90, roll=0))
        radar = self.world.spawn_actor(radar_blueprint, radar_transform, attach_to=self.main_vehicle)

        self.sensors.append(gnss_sensor)
        self.sensors.append(radar)
        
        radar.listen(lambda data: self.radar_handler.radar_callback(data))
        gnss_sensor.listen(lambda data: self.gnss_handler.gnss_callback(data))   
        
        print( "Sensors have been set up.")
             
    def setup_vehicles(self):

        
        print("Setting Spectator Tansform...")
        spectator = self.world.get_spectator()
        transform = carla.Transform(carla.Location(x=130, y=self.Y2_POSITION, z=62), carla.Rotation(pitch=-90, yaw=0, roll=0))
        spectator.set_transform(transform)
        print(f"Spectator Transform set to {transform}.")
        
        print("Setting up vehicles on the side of the road...")
        for x_offset in self.X_OFFSETS:
            transform = carla.Transform(
            carla.Location(x=self.X_POSITION + x_offset, y=self.Y3_POSITION, z=self.Z_POSITION),
            carla.Rotation(pitch=self.PITCH, yaw=self.YAW, roll=self.ROLL)
            )
            vehicle = self.world.spawn_actor(self.vehicle_blueprint, transform)
            self.vehicles.append(vehicle)
        print("Vehicles on the side have been set up.")    
        
        print("Setting main vehicle...")
        
        main_transform = carla.Transform(carla.Location(x=self.X_POSITION - 6, y=self.Y2_POSITION, z=self.Z_POSITION),
                                carla.Rotation(pitch=self.PITCH, yaw=self.YAW, roll=self.ROLL))
        
        main_vehicle = self.world.spawn_actor(self.vehicle_blueprint, main_transform)
        
        self.vehicles.append(main_vehicle)
        
        self.main_vehicle = main_vehicle
        
        print("Main vehicle has been set up.")
        #print(f"The length of the vehicle is {main_vehicle.bounding_box.extent.x * 2}.")
        #print(f"The width of the vehicle is {main_vehicle.bounding_box.extent.y * 2}.")