import queue
import threading
import struct
import socket
import time
from collections import namedtuple
import struct
from secureClient import setup_crypto_config, send_encrypt, recv_decrypt

# ControlData is a named tuple that contains the control data received from the client.
ControlData = namedtuple('ControlData', 'throttle steer brake reverse time')

# MAX_DATA_POINTS is the number of data points that can be sent to the server at once.
MAX_DATA_POINTS = 8

# SEND_INTERVAL is the interval at which the thread tries to send the data to the server.
SEND_INTERVAL = 0.4     

def start_server(host, port):
    """
    This function starts the server and listens for a connection from the TestApp client.
    TcpNoDelay is set to 1 to disable Nagle's algorithm.
    The key exchange is done using the secureClient.py file.
    """
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(1)
    print(f"Server listening on {host}:{port}")
    setup_crypto_config()
    client_socket, client_address = server_socket.accept()
    client_socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    return client_socket

def send_to_server(connection_socket,data1, data2):
    """
    This function sends the data to the server. The data is sent in the following format:
    - 1 byte: Start of data pair (0x13)
    - 4 bytes: Length of the data pair size( "-" included)
    - n bytes: Data pair encoded in utf-8
    - 1 byte: End of data pair (0x14)
    If the TestApp client finished sending the commands
    and we try to send data to the server, a BrokenPipeError is raised.
    We can just ignore it.
    The code is inspired from the following links:
    https://code.activestate.com/recipes/408859-socketrecv-three-ways-to-turn-it-into-recvall/
    https://thoslin.github.io/build-a-simple-protocol-over-tcp/"""
    
    data_str = b""
    for i in range(len(data1)):
        radar = data1[i]
        gnss = data2[i]

        combined_data = f"{radar}-{gnss}"
        combined_data = combined_data.replace('(', '').replace(')', '')

        encoded_data = combined_data.encode('utf-8')
        packed_size = struct.pack('>i', len(encoded_data))
        data_to_send = b"\x13" + packed_size + encoded_data + b"\x14"
        data_str += data_to_send
        
    try:
        send_encrypt(connection_socket, data_str)  
    except BrokenPipeError:
        pass
        

class ControlDataReceiver(threading.Thread):
    """
    This thread class is used to receive the control data from the TestApp client.
    It runs alongside the main thread and listens for the control data.
    The control data is received in the following format:
    - 4 floats: throttle, steer, brake, time
    - 1 int: reverse
    The control data is then put into a queue.
    """
    def __init__(self, connection_socket):
        super().__init__()
        self.connection_socket = connection_socket
        self.control_data_queue = queue.Queue()
        self.format = 'fffif'  
        self.tuple_size = struct.calcsize(self.format)
        self.leftover_data = b''
        self.stop_event = threading.Event()
        
    def run(self):
        while not self.stop_event.is_set():
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

class SensorDataSender(threading.Thread):
    """
    This thread class is used to send the sensor data to the server.
    It runs alongside the main thread and sends the sensor data to the server.
    At each interval, it locks the sensor lists and checks if the lists have enough data in it.
    If the lists have enough data, it sends the data to the server.
    """
    def __init__(self,connection_socket, gnss_handler, radar_handler):
        super().__init__()
        self.connection_socket = connection_socket
        self.gnss_handler = gnss_handler
        self.radar_handler = radar_handler
        self.stop_event = threading.Event()

    def send_accumulated_data(self):
        with self.radar_handler.radar_data_lock, self.gnss_handler.gnss_data_lock:
            if len(self.radar_handler.radar_data_points) >= MAX_DATA_POINTS and len(self.gnss_handler.gnss_data_points) >= MAX_DATA_POINTS:
                data1_chunk = self.radar_handler.radar_data_points[:MAX_DATA_POINTS]
                data2_chunk = self.gnss_handler.gnss_data_points[:MAX_DATA_POINTS]
                
                self.radar_handler.radar_data_points = self.radar_handler.radar_data_points[MAX_DATA_POINTS:]
                self.gnss_handler.gnss_data_points = self.gnss_handler.gnss_data_points[MAX_DATA_POINTS:]
                
                send_to_server(self.connection_socket, data1_chunk, data2_chunk)
                
    def run(self):
        while not self.stop_event.is_set():
            time.sleep(SEND_INTERVAL)
            self.send_accumulated_data()

    def stop_thread(self):
        self.stop_event.set()
        if self.is_alive():
            self.join()          
            
            

            
