import numpy as np
import carla
from collections import namedtuple
import time
from scenario import ScenarioSetup
from arg_parser import parse_arguments
from communication import ControlDataReceiver,  DataSender
from utils import GNSSHandler, RadarHandler

args = parse_arguments()

HOST = args.hostserver
PORT = args.port
host_simulator = args.hostsimulator
port_simulator = args.portsimulator
map = args.map


import socket

def start_server(host, port):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(1)
    print(f"Server listening on {host}:{port}")
    client_socket, client_address = server_socket.accept()
    return client_socket

try:
    # !!! call setup_crypto_config here !!! #
    gnss_handler = GNSSHandler()
    radar_handler = RadarHandler()
    
    #connection_socket = connect_to_server(HOST,PORT)
    connection_socket = start_server(HOST,PORT)
    
    scenario = ScenarioSetup(host_simulator, port_simulator, gnss_handler,radar_handler)
    scenario.change_map(map)
    scenario.setup_vehicles()
    scenario.sensor_setup()
    
    data_sender = DataSender(connection_socket, gnss_handler, radar_handler)
    receiver = ControlDataReceiver(connection_socket)
    
    time.sleep(5)
    ## may need to add wait here
    receiver.start()
    data_sender.start()
    
    
    while True:
        if receiver.control_data_queue:
            control_data = receiver.control_data_queue.get()
            print(f"Applying control data with values: {control_data}.")

            scenario.main_vehicle.apply_control(carla.VehicleControl(throttle=control_data.throttle,
                                                       steer=control_data.steer,
                                                       brake=control_data.brake,
                                                       reverse=control_data.reverse))
            
            if all(value == 0 for value in control_data):
                print("Stop event received.")
                break
                
            if control_data.time > 0:
                print(f"Sleeping for {control_data.time} seconds.")
                time.sleep(control_data.time)
                print("Applying brake.") # this might be unnecessary
                scenario.main_vehicle.apply_control(carla.VehicleControl(throttle=0,
                                                           steer=0,
                                                           brake=1))

finally:
    print("Cleaning up...")
    receiver.stop_thread()
    time.sleep(2)
    data_sender.stop_thread()
    time.sleep(2)

    scenario.clean_up()
    time.sleep(3)
    
    

    

