from flask import Flask, request
from flask_restful import Api
from flask_cors import CORS
from flask_socketio import SocketIO, send
import json
from _thread import *
import socket
from xenutils import get_active_VMs
import os

import sys
sys.path.append('../../server/')

from server import learn

app = Flask(__name__, static_url_path='')
CORS(app) 
api = Api(app)

@app.route("/list/vm")
def serve():
    vms = get_active_VMs()
    ids = []

    for i in range(len(vms)):
        ids.append(i)

    return_vms = [{"name" : n, "id" : i} for n, i in zip(vms, ids)]

    return str(json.dumps(return_vms))

@app.route("/start/vm")
def startvm():
    name = request.form['name']

    os.system("gnome-terminal -e 'bash -c \"gvncviewer localhost; exec bash\"'")

    return str("You will see the VM in another window")

@app.route("/analyze")
def analyze():
    name = request.form['name']
    time = request.form['time']
    learn(name, time)

@app.route("/")
def serve2():
    return "Tralala"

def process_syscall(list):
    print(list)

def educational_client(connection):
    start = "**"
    while True:
        bytes_recd = 0
        chunk = start
        end = 0

        chunk = connection.recv(1024)
        if chunk == b'':
            raise RuntimeError("socket connection broken")
        bytes_recd = bytes_recd + len(chunk)

        str_chunk = chunk.decode("utf-8")
        split = str_chunk.split('*')
        syscall = []

        for piece in split:
            if piece == '':
                    continue
            if piece == 'end':
                process_syscall(syscall)
                syscall = []
            else:
                syscall.append(piece)

def thread_server():
    ServerSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    host = '127.0.0.1'
    port = 1234
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
        start_new_thread(educational_client, (Client, ))
        ThreadCount += 1
        print('Thread Number: ' + str(ThreadCount))
        ServerSocket.close()
                
if __name__ == '__main__':
    start_new_thread(thread_server, ())
    app.run()