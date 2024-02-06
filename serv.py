import socket

# Set up server socket

LISTENER_ADDRESS = "172.17.0.1"
LISTENER_PORT = 6000

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_address = (LISTENER_ADDRESS, LISTENER_PORT)
server_socket.bind(server_address)
server_socket.listen(5)

print(f"Server listening on {server_address}...")

while True:
    # Accept a connection from a client
    client_socket, client_address = server_socket.accept()
    print(f"Connection from {client_address}")

    # Send "Hello, World!" to the client
    message = "Hello, World!"
    client_socket.send(message.encode())

    # Close the connection
    client_socket.close()
