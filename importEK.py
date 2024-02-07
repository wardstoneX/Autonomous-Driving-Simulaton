from Crypto.Hash import SHA256

str = input("Copypaste key here:")
data = bytes([ int(b, 16) for b in str.split(" ") ])
hash_object = SHA256.new(data)
f = open("EK_hash.txt", "w")
f.write(hash_object.hexdigest())
f.close()
