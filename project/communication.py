import queue
import threading
import struct
import socket
from utils import ControlData, send_to_server
import time

MAX_DATA_POINTS = 20
SEND_INTERVAL = 0.5 ## what the fuck is this

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
            data = self.connection_socket.recv(1024)
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
            
            
class DataSender(threading.Thread):
    def __init__(self,connection_socket, gnss_handler, radar_handler):
        super().__init__()
        self.connection_socket = connection_socket
        self.gnss_handler = gnss_handler
        self.radar_handler = radar_handler
        self.stop_event = threading.Event()


    def send_accumulated_data(self):
        with self.radar_handler.radar_data_lock, self.gnss_handler.gnss_data_lock:
            
            data1_chunk = self.radar_handler.radar_data_points[:MAX_DATA_POINTS]
            radar_data = self.radar_handler.radar_data_points[MAX_DATA_POINTS:]

            data2_chunk = self.gnss_handler.gnss_data_points[:MAX_DATA_POINTS]
            gnss_data = self.gnss_handler.gnss_data_points[MAX_DATA_POINTS:]

            # Check if both data1_chunk and data2_chunk are full
            if data1_chunk and data2_chunk and len(data1_chunk) == len(data2_chunk):
                send_to_server(data1_chunk, data2_chunk)
            else:
                # If either chunk is empty, restore the data points lists
                self.radar_handler.radar_data_points = data1_chunk + radar_data
                self.gnss_handler.gnss_data_points = data2_chunk + gnss_data

    def run(self):
        while not self.stop_event.is_set():
            self.send_accumulated_data()
            time.sleep(SEND_INTERVAL)

    

    def stop_thread(self):
        self.stop_event.set()
        if self.is_alive():
            self.join()          