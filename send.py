import cv2
import numpy as np
import socket

# Define the host and port for the server (your laptop)
HOST = '192.168.0.9'
PORT = 5555

# Start the video capture from the USB camera
cap = cv2.VideoCapture(0)

# Set the resolution of the video
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

# Create a socket object and connect to the server
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect((HOST, PORT))

# Start sending the video frames to the server
while True:
    # Capture a frame from the USB camera
    ret, frame = cap.read()

    # Convert the frame to a byte array
    data = cv2.imencode('.jpg', frame)[1].tostring()

    # Send the length of the frame data and then the frame data itself
    client_socket.sendall((str(len(data))).encode().ljust(16) + data)

# Close the client socket and release the video capture
client_socket.close()
cap.release()
