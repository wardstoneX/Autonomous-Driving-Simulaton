import socket
import os
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.asymmetric.padding import OAEP, MGF1
from cryptography.hazmat.primitives.serialization import load_ssh_public_key, PublicFormat, Encoding
from cryptography.hazmat.primitives.hashes import HashAlgorithm

HOST = "10.0.0.10"      #arbitrary, make sure to configure on the computer when running the python client
CRYPTO_PORT = 65432     #arbitrary, add later to system_config.h
COMM_PORT = 1234        #arbitrary, add later to system_config.h

#TODO: add a hash of EK_pub for comparison




#please run this function before performing any secure send/recv operations
def setup_crypto_config():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, CRYPTO_PORT))
        s.listen()
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            #3.- receive EK_pub and SRK_pub from app
            data = conn.recv(1024)
            
            ptr = 0
            EK_size = int.from_bytes(data[ptr : ptr+4], "big")
            ptr += 4

            #the following format check is not redundant, the serialization would not throw an error if the key was from a different protocol
            ssh_rsa_beginning = b"\x73\x73\x68\x2d\x72\x73\x61"
            if (data[ptr : ptr+7] != ssh_rsa_beginning):
                print(f"The format of EK_pub is wrong.\nReceived {data[ptr : ptr+7]}, expected {ssh_rsa_beginning}")
            EK_pub = load_ssh_public_key(data[ptr : ptr+EK_size])

            ptr += EK_size
            if (data[ptr : ptr+7] != ssh_rsa_beginning):
                print(f"The format of SRK_pub is wrong.\nReceived {data[ptr : ptr+7]}, expected {ssh_rsa_beginning}")
            SRK_pub = load_ssh_public_key(data[ptr : ])


            #4.- compare received EK_pub with stored hash
            #TODO
            print("PRINTING THE RECEIVED VALUE OF EK")
            print(f"OpenSSH cEK: {data[4:4+EK_size]}")


            #5.- generate K_sym and encrypt: ciphertext = encrypt(EK_pub, encrypt(SRK_pub, K_sym))
            global key, iv
            key = os.urandom(32)
            iv = os.urandom(12)

            print("PRINTING DETAILS OF THE GENERATED AES KEY")
            print(f"Key: {key}")
            print(f"IV: {iv}")


            #the TestApp is already expecting a 256 bit key and 12 byte IV, so we can send the key concatenated with the IV, without extra length tags
            k_sym = key + iv
            
            inner_cipher = SRK_pub.encrypt(
                k_sym,
                OAEP(
                    mgf=MGF1(algorithm=HashAlgorithm.SHA256()),
                    algorithm=HashAlgorithm.SHA256(),
                    label=None
                )
            )
            ciphertext = EK_pub.encrypt(
                inner_cipher,
                OAEP(
                    mgf=MGF1(algorithm=HashAlgorithm.SHA256()),
                    algorithm=HashAlgorithm.SHA256(),
                    label=None
                )
            )

            #6.- send ciphertext to app
            conn.sendall(ciphertext)








def send_encrypt(connection, data):
    #encrypt the data using AES in GCM mode
    cipher = Cipher(algorithms.AES(key), modes.GCM(iv))
    encryptor = cipher.encryptor()
    ciphertext = encryptor.update(data) + encryptor.finalize()
    connection.sendall(ciphertext)

def recv_decrypt(connection):
    ciphertext = connection.recv(1024)
    #decrypt the data
    cipher = Cipher(algorithms.AES(key), modes.GCM(iv))
    decryptor = cipher.decryptor()
    return decryptor.update(ciphertext) + decryptor.finalize()




#DUMMY SERVER IS HERE
print("python dummy server is running")
setup_crypto_config()
print("key exchange was successful")
#any other code using send_encrypt() or recv_decrypt() should be sent after calling setup_crypto_config()
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, COMM_PORT))
        s.listen()
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr}")
            data = conn.recv(1024)
            print(f"Received the following message: {data}")
            print(f"Sending back a message")
            #TODO: try to send more data, less data, and data in parts
            conn.sendall(b"Hi TestApp!!")
            print(f"Reply sent")













        



        















