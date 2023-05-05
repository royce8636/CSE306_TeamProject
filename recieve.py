import cv2
import numpy as np
import socket

# Define the host and port for the server (your laptop)
HOST = '192.168.0.9'
PORT = 5555

# Create a socket object and bind it to the host and port
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((HOST, PORT))
server_socket.listen(1)

# Accept a client connection
print('Waiting for connection...')
client_socket, address = server_socket.accept()
print(f'Connected to {address[0]}')

# Start receiving and printing the video frames
while True:
    # Receive the length of the frame data and then the frame data itself
    data = b''
    while len(data) < 16:
        data += client_socket.recv(16 - len(data))
    frame_length = int(data)
    data = b''
    while len(data) < frame_length:
        data += client_socket.recv(frame_length - len(data))

    # Convert the received data back into an OpenCV frame
    frame = cv2.imdecode(np.frombuffer(data, dtype=np.uint8), cv2.IMREAD_COLOR)
    # Print the frame
    cv2.imshow('Frame', frame)
    cv2.waitKey(1)

# Close the client and server sockets
client_socket.close()
server_socket.close()
