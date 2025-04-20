#!/usr/bin/env python3
import zmq
import msgpack
import time

context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.connect("tcp://127.0.0.1:6600")
i = 0
while True:
    
    msg = [
        "front_cam",
        180000000000+i,
        (i%10)/10,
        (i%10)/10,
        1.0,
        1.0,
        0,
        0.5,
    ]
    i+=1
    packed = msgpack.packb(msg)
    socket.send(packed)
    print("Sent:", msg)
    time.sleep(0.5)
