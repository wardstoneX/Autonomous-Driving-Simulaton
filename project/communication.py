import queue
import threading
import struct
import socket
import time
from collections import namedtuple
import struct
from secureClient import setup_crypto_config, send_encrypt, recv_decrypt


ControlData = namedtuple('ControlData', 'throttle steer brake reverse time')
      
def start_server(host, port):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(1)
    print(f"Server listening on {host}:{port}")
    setup_crypto_config()
    client_socket, client_address = server_socket.accept()
    return client_socket

def send_to_server(connection_socket,data1, data2):
    data_str = b""

    for i in range(len(data1)):
        radar = data1[i]
        gnss = data2[i]
        # if data1[i] != (0,0,0):
        # radar = radar_measurement_to_cartesian(gnss, radar)

        combined_data = f"{radar}-{gnss}"
        combined_data = combined_data.replace('(', '').replace(')', '')

        encoded_data = combined_data.encode('utf-8')
        packed_size = struct.pack('>i', len(encoded_data))
        data_to_send = b"\x13" + packed_size + encoded_data + b"\x14"
        data_str += data_to_send
        
    # !!! call send_encrypt here !!! #
    #print(f"Sending {len(data_str)} data  to server.")
    try:
        send_encrypt(connection_socket, data_str)  
    except BrokenPipeError:
        pass
        
def connect_to_server(host, port):
    connection_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    connection_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    connection_socket.connect((host, port))
    print(f"Connected to server at {host}:{port}")
    return connection_socket

class ControlDataReceiver(threading.Thread):
    def __init__(self, connection_socket):
        super().__init__()
        self.connection_socket = connection_socket
        self.control_data_queue = queue.Queue()
        self.format = 'fffif'  # 4 floats, 1 bool, 1 int
        self.tuple_size = struct.calcsize(self.format)
        self.leftover_data = b''
        self.stop_event = threading.Event()
        

    def run(self):
        while not self.stop_event.is_set():
            # !!! call recv_decrypt here !!! #
            data = recv_decrypt(self.connection_socket)
            data = self.leftover_data + data
            self.leftover_data = b''

            while len(data) >= self.tuple_size:
                tuple_data = data[:self.tuple_size]
                data = data[self.tuple_size:]

                control_data = struct.unpack(self.format, tuple_data)
                control_data = ControlData(*control_data)
                self.control_data_queue.put(control_data)
                

                
            if data:
                self.leftover_data = data
   
    def stop_thread(self):
        self.stop_event.set()
        if self.is_alive():
            self.join()
            
MAX_DATA_POINTS = 20
SEND_INTERVAL = 1

class SensorDataSender(threading.Thread):
    def __init__(self,connection_socket, gnss_handler, radar_handler):
        super().__init__()
        self.connection_socket = connection_socket
        self.gnss_handler = gnss_handler
        self.radar_handler = radar_handler
        self.stop_event = threading.Event()


    def send_accumulated_data(self):
        with self.radar_handler.radar_data_lock, self.gnss_handler.gnss_data_lock:
            #print(self.radar_handler.radar_data_points)

            # Check if both data1_chunk and data2_chunk are full
            #print(len(self.radar_handler.radar_data_points), len(self.gnss_handler.gnss_data_points))
            if len(self.radar_handler.radar_data_points) >= MAX_DATA_POINTS and len(self.gnss_handler.gnss_data_points) >= MAX_DATA_POINTS:
                data1_chunk = self.radar_handler.radar_data_points[:MAX_DATA_POINTS]
                data2_chunk = self.gnss_handler.gnss_data_points[:MAX_DATA_POINTS]
                
                self.radar_handler.radar_data_points = self.radar_handler.radar_data_points[MAX_DATA_POINTS:]
                self.gnss_handler.gnss_data_points = self.gnss_handler.gnss_data_points[MAX_DATA_POINTS:]
                
                #print(data1_chunk)
                send_to_server(self.connection_socket, data1_chunk, data2_chunk)
                


    def run(self):
        print("DataSender thread started.")
        while not self.stop_event.is_set():
            time.sleep(SEND_INTERVAL)
            self.send_accumulated_data()

    

    def stop_thread(self):
        self.stop_event.set()
        if self.is_alive():
            self.join()          
            
            

            
