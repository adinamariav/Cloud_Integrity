from BoSC import BoSC_Creator
from analyse_trace import create_lookup_table

import socket
import sys
from _thread import *

bosc = BoSC_Creator(10)
anomalyCount = 0
    

def threaded_client(connection):
    global anomalyCount
    while True:
        bytes_recd = 0
        chunks = []
        chunk = ",,"
        while True:
            chunk = connection.recv(2048)
            print(chunk)
            if chunk == b'':
                raise RuntimeError("socket connection broken")
            chunks.append(chunk)
            bytes_recd = bytes_recd + len(chunk)
            if chunk[-1] != '.':
                break
        chunks =  b''.join(chunks).decode("utf-8")
        chunks = chunks[:-2]
        syscalls = chunks.split(",")

        for syscall in syscalls:
            if bosc.append_BoSC(syscall) == 'Anomaly':
                anomalyCount = anomalyCount + 1
    connection.close()


if len(sys.argv) > 1:
    if sys.argv[1] == "--learn":
        create_lookup_table()
        bosc.create_learning_db()
  
    elif sys.argv[1] == "--analyze":
        ServerSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        host = '127.0.0.1'
        port = 1233
        ThreadCount = 0
        try:
            ServerSocket.bind((host, port))
        except socket.error as e:
            print(str(e))
        print('Waiting for a connection..')
        ServerSocket.listen(5)
        while True:
            Client, address = ServerSocket.accept()
            print('Connected to: ' + address[0] + ':' + str(address[1]))
            start_new_thread(threaded_client, (Client, ))
            ThreadCount += 1
            print('Thread Number: ' + str(ThreadCount))
        ServerSocket.close()
            
else:
    print("Not enough arguments")