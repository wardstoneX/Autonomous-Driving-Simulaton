import socket
from Crypto import Random
from Crypto.Hash import SHA256
from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP, AES, PKCS1_v1_5

HOST = "10.0.0.10"      #arbitrary, make sure to configure on the computer when running the python client
CRYPTO_PORT = 65432     #arbitrary, add later to system_config.h
COMM_PORT = 1234        #arbitrary, add later to system_config.h

#please run this function before performing any secure send/recv operations
def setup_crypto_config():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, CRYPTO_PORT))
        s.listen()
        conn, addr = s.accept()
        with conn:
            print(f"Connected by {addr} on crypto port")

            #3.- receive EK_pub and SRK_pub from app
            data = conn.recv(1024)
            print(f"DUMP: {data.hex()}")

            EK_exp = int.from_bytes(data[0:4], "little")
            EK_nLen = int.from_bytes(data[4:8], "little")
            EK_n = int.from_bytes(data[8:8+EK_nLen], "big")
            EK = RSA.construct((EK_n, EK_exp))
            SRK_exp = int.from_bytes(data[8+EK_nLen:12+EK_nLen], "little")
            SRK_nLen = int.from_bytes(data[12+EK_nLen:16+EK_nLen], "little")
            SRK_n = int.from_bytes(data[16+EK_nLen:16+EK_nLen+SRK_nLen], "big")
            SRK = RSA.construct((SRK_n, SRK_exp))
            
            #TODO: 4.- compare received EK_pub with stored hash
            rec_ek_hash = SHA256.new(data[8:8+EK_nLen])
            rec_digest = rec_ek_hash.hexdigest()
            f = open("EK_hash.txt", "r")
            og_digest = f.readline().strip()
            if rec_digest != og_digest:
                print("The received EK is not correct, rec_digest: {rec_digest}")


            #5.- generate K_sym and encrypt: ciphertext = encrypt(EK_pub, encrypt(SRK_pub, K_sym))
            global K_sym
#            K_sym = Random.get_random_bytes(32)
            K_sym = bytes([ i for i in range(0,32)])

            print(f"Generated key: {K_sym}")

            #cipher_EK = PKCS1_OAEP.new(EK)
            #cipher_SRK = PKCS1_OAEP.new(SRK)
            cipher_EK = PKCS1_v1_5.new(EK)
            cipher_SRK = PKCS1_v1_5.new(SRK)
            ciphertext = cipher_EK.encrypt(cipher_SRK.encrypt(K_sym))
            #ciphertext = cipher_SRK.encrypt(K_sym)
            print(f"Encrypted key: {ciphertext.hex()}")

            #6.- send ciphertext to app
            conn.sendall(ciphertext)
            print("key sent")
        s.shutdown(socket.SHUT_RDWR)
        s.close()








def send_encrypt(connection, data):
    nonce = Random.get_random_bytes(12)
    cipher = AES.new(K_sym, AES.MODE_GCM, nonce)
    ciphertext = cipher.encrypt(data)
    payload = nonce + ciphertext
    connection.sendall(payload)

    '''
    cipher = Cipher(algorithms.AES(key), modes.GCM(iv))
    encryptor = cipher.encryptor()
    ciphertext = encryptor.update(data) + encryptor.finalize()
    connection.sendall(ciphertext)
    '''

def recv_decrypt(connection):
    data = connection.recv(1024)
    nonce = data[0:12]
    cipher = AES.new(K_sym, AES.Mode_GCM, nonce)
    return cipher.decrypt(data[12:])

    '''
    cipher = Cipher(algorithms.AES(key), modes.GCM(iv))
    decryptor = cipher.decryptor()
    return decryptor.update(ciphertext) + decryptor.finalize()
    '''




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













        



        















