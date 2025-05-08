#!/usr/bin/env python3
import sys
import zmq
import msgpack
import time

context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.connect("tcp://127.0.0.1:6600")
i = 0
t = 0
while True:
    
    msg = [
        sys.argv[1],
        180000000000+t,
        (i%5)/10,
        (i%5)/10,
        0.5+(i%5)/10,
        0.5+(i%5)/10,
        i % 3,
        0.2+(i%7)*0.1,
    ]
    if i % 10 == 0:
        t+=1
    i+=1
    packed = msgpack.packb(msg)
    socket.send(packed)
    print("Sent:", msg)
    time.sleep(0.05)
