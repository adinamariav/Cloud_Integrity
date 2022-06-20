from BoSC import BoSC_Creator
from _thread import *
from analyse_trace import create_lookup_table

import subprocess
import socket
import sys
import os
import shlex

bosc = BoSC_Creator(10)
anomalyCount = 0
    
def learn(name, time):
    os.chdir('../../')
    command = "sudo ./vmi -m learn -v " + name + " -t " + time
    subprocess.run(shlex.split(command))
    create_lookup_table()
    bosc.create_learning_db()
    os.chdir("web/server/")

def analysis_client(connection):
    global anomalyCount
    global bosc
    bosc.load_lookup_table()
    run = True
    while run == True:
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
        print("Anomalies ", anomalyCount)
    connection.close()

def analyze(name, time):
    os.chdir('../../')
    command = "sudo ./vmi -m analyze -v " + name + " -t " + time
    subprocess.Popen(command, shell=True)
    os.chdir("web/server/")

    ServerSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    host = '127.0.0.1'
    port = 1233
    try:
        ServerSocket.bind((host, port))
    except socket.error as e:
        print(str(e))
    print('Waiting for a connection (analysis server)')
    ServerSocket.listen(5)
    Client, address = ServerSocket.accept()
    print('Connected to: ' + address[0] + ':' + str(address[1]))
    analysis_client(Client)
    ServerSocket.close()


if len(sys.argv) > 1:
    if sys.argv[1] == "--learn":
        os.chdir('../')
        command = "sudo ./vmi -m learn -v " + sys.argv[2] + " -t " + sys.argv[3]
        subprocess.run(shlex.split(command))
        os.chdir("server/")
        create_lookup_table()
        bosc.create_learning_db()
  
    elif (sys.argv[1] == "--analyze"):
            os.chdir('../')
            command = "sudo ./vmi -m analyze -v " + sys.argv[2] + " -t " + sys.argv[3]
            subprocess.Popen(command, shell=True)
            os.chdir("server/")

            ServerSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            host = '127.0.0.1'
            port = 1233
            try:
                ServerSocket.bind((host, port))
            except socket.error as e:
                print(str(e))
            print('Waiting for a connection (analysis server)')
            ServerSocket.listen(5)
            Client, address = ServerSocket.accept()
            print('Connected to: ' + address[0] + ':' + str(address[1]))
            analysis_client(Client)
            ServerSocket.close()
    else:
        print("Syntax: server.py --learn [name] [time/s]")   
else:
    print("Not enough arguments")