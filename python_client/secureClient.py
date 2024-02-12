import socket
from Crypto import Random
from Crypto.Hash import SHA256
from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP, AES

HOST = "10.0.0.10"      #arbitrary, make sure to configure on the computer when running the python client
CRYPTO_PORT = 65432     #arbitrary, add later to system_config.h

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

            EK_exp = int.from_bytes(data[0:4], "little")
            EK_nLen = int.from_bytes(data[4:8], "little")
            EK_n = int.from_bytes(data[8:8+EK_nLen], "big")
            EK = RSA.construct((EK_n, EK_exp))
            SRK_exp = int.from_bytes(data[8+EK_nLen:12+EK_nLen], "little")
            SRK_nLen = int.from_bytes(data[12+EK_nLen:16+EK_nLen], "little")
            SRK_n = int.from_bytes(data[16+EK_nLen:16+EK_nLen+SRK_nLen], "big")
            SRK = RSA.construct((SRK_n, SRK_exp))
            
            #4.- compare received EK_pub with stored hash
            rec_ek_hash = SHA256.new(data[8:8+EK_nLen])
            rec_digest = rec_ek_hash.hexdigest()
            f = open("EK_hash.txt", "r")
            og_digest = f.readline().strip()
            if rec_digest != og_digest:
                print(f"The received EK is not correct, rec_digest: {rec_digest}")
                exit(1)
            print("Verified cEK.")
            #5.- generate K_sym and encrypt: ciphertext = encrypt(EK_pub, encrypt(SRK_pub, K_sym))
            global K_sym
            K_sym = Random.get_random_bytes(32)

            cipher_EK = PKCS1_OAEP.new(EK, SHA256)
            cipher_SRK = PKCS1_OAEP.new(SRK, SHA256)

            ciphertext = cipher_EK.encrypt(cipher_SRK.encrypt(K_sym))

            #6.- send ciphertext to app
            conn.sendall(ciphertext)
            print("key sent")








def send_encrypt(connection, data):
    nonce = Random.get_random_bytes(12)
    cipher = AES.new(K_sym, AES.MODE_GCM, nonce)
    ciphertext = cipher.encrypt(data)
    payload = nonce + ciphertext
    #print(f"Sending {len(ciphertext)} bytes of data.")
    connection.sendall(payload)

def recv_decrypt(connection):
    data = connection.recv(1024)
    if len(data)<12:
        return b""
    print(f"received {len(data)} bytes of payload: {data.hex()}")
    nonce = data[0:12]
    cipher = AES.new(K_sym, AES.MODE_GCM, nonce)
    return cipher.decrypt(data[12:])
