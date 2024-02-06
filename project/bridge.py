import numpy as np
import carla
from collections import namedtuple
import time
from scenario import ScenarioSetup
from arg_parser import parse_arguments
from communication import ControlDataReceiver, connect_to_server, DataSender
from utils import GNSSHandler, RadarHandler

args = parse_arguments()

HOST = args.host
PORT = args.port
host_simulator = args.host_simulator
port_simulator = args.port_simulator
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
    
    data_sender = DataSender(connection_socket, gnss_handler, radar_handler)
    receiver = ControlDataReceiver(connection_socket)

    receiver.start()
    data_sender.start()

    scenario = ScenarioSetup(host_simulator, port_simulator, gnss_handler,radar_handler)
    scenario.change_map(map)
    scenario.setup_vehicles()
    scenario.sensor_setup()
    ## may need to add wait here
    
    while True:
        if receiver.control_data_queue:
            control_data = receiver.control_data_queue.get()
            print(f"Applying control data with values: {control_data}.")

            scenario.main_vehicle.apply_control(carla.VehicleControl(throttle=control_data.throttle,
                                                       steer=control_data.steer,
                                                       brake=control_data.brake,
                                                       reverse=control_data.reverse))
            if control_data.time > 0:
                print(f"Sleeping for {control_data.time} seconds.")
                time.sleep(control_data.time)
                print("Applying brake.") # this might be unnecessary
                scenario.main_vehicle.apply_control(carla.VehicleControl(throttle=0,
                                                           steer=0,
                                                           brake=1))

finally:
    receiver.stop()
    data_sender.stop()
    scenario.clean_up()
    

