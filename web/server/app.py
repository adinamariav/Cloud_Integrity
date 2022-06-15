from flask import Flask, request, copy_current_request_context
from flask_restful import Api
from flask_cors import CORS
from flask_socketio import *
from eventlet.green import subprocess
import time
from types import SimpleNamespace
import threading
import os
import json
import socket

from xenutils import get_active_VMs, start_educational
import sys

import sys
sys.path.append('../../server/')

from server import learn, analyze

app = Flask(__name__, static_url_path='')

CORS(app) 
api = Api(app)

UPLOAD_FOLDER = './files'

socketIO = SocketIO(app, cors_allowed_origins=[], engineio_logger=True, logger=True, async_mode='eventlet')
#socketIO = SocketIO(app, cors_allowed_origins=[],async_mode='eventlet')
end_client = 0

import eventlet
eventlet.monkey_patch()

@app.route("/list/vm")
def serve():
    json_data = ""

    try:
        vms = get_active_VMs()
        ids = []

        for i in range(len(vms)):
            ids.append(i)

        return_vms = [{"name" : n, "id" : i} for n, i in zip(vms, ids)]
        json_data = str(json.dumps(return_vms))
    except Exception as e:
        print(e)

    print("listvm")
    return json_data

@app.route("/")
def serve2():
    return "Tralala"

@socketIO.on("connect")
def connect():
    print("connected!")

def run_command(command):
    p = subprocess.Popen(command, stdout = subprocess.PIPE, stderr = subprocess.STDOUT, shell = True)

@socketIO.on("startEducational")
def start(vm, time):
    os.chdir('../../')
    command = "sudo ./vmi -m educational -v " + vm + " -t " + time
    thread = threading.Thread(target=run_command, args=(command, ))
    thread.start()
    os.chdir('web/server/')

@socketIO.on("startLearn")
def startLearn(vm, time):
    thread = threading.Thread(target=learn, args=(vm, time, ))
    thread.start()

@socketIO.on("startAnalyze")
def startAnalyze(name, vm):
    thread = threading.Thread(target=analyze, args=(name, vm, ))
    thread.start()
    @copy_current_request_context
    def check_thread_status(thread):
        while(True):
            if not thread.is_alive():
                emit("anomaly")
    thread_check = threading.Thread(target=check_thread_status, args=(thread, ))
    thread_check.start()

@socketIO.on("startSandbox")
def startSandbox(vm, time, syscalls):
    syscalls = json.loads(syscalls, object_hook=lambda d: SimpleNamespace(**d))
    syscall_list = syscalls.syscalls
    f = open("../../data/syscalls.txt", "w")
    for sys in syscall_list:
        f.write(str(sys) + "\n")
    os.chdir('../../')
    command = "sudo ./vmi -m sandbox -v " + vm + " -t " + time
    thread = threading.Thread(target=run_command, args=(command, ))
    thread.start()
    os.chdir('web/server/')

@app.route('/uploadfile', methods=['POST'])
def fileUpload():
    target = UPLOAD_FOLDER

    if not os.path.isdir(target):
        os.mkdir(target)

    file = request.files['file'] 
    filename = file.filename
    vm = request.form["vmName"]
    destination= "/".join([target, filename])
    file.save(destination)
    hostname = "adina@" + vm + ":~"
    filepath = target + "/" + filename
    subprocess.run(['scp', filepath, hostname], stdout=subprocess.PIPE)
    return "Success"
                

@socketIO.on("startvm")
def startvm(vmname):
    print("startvm")
    @copy_current_request_context
    def thread_serve(port):
        ServerSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        host = '127.0.0.1'
        try:
            ServerSocket.bind((host, port))
        except socket.error as e:
            print(str(e))
        print('Waiting for a connection..')
        ServerSocket.listen(5)
        Client, address = ServerSocket.accept()
        print('Connected to: ' + address[0] + ':' + str(address[1]))
        educational_client(Client)
        ServerSocket.close()

    thread = threading.Thread(target=thread_serve, args=(1234, ))
    thread.start()

    @copy_current_request_context
    def get_mem_cpu_load(vm):
        command = "sudo xentop -b"
        p = subprocess.Popen(command, stdout = subprocess.PIPE, stderr = subprocess.STDOUT, shell = True)
        while True:
            line = p.stdout.readline().decode("utf-8")
            splits = line.split()
            if len(splits) > 0:
                if (splits[0] == vm):
                    emit("cpu", { 'data' : splits[3] }, broadcast=True)
                    emit("mem", { 'data' : splits[5] }, broadcast=True)
            if not line: break

    thread = threading.Thread(target=get_mem_cpu_load, args=(vmname, ))
    thread.start()

    @copy_current_request_context
    def get_net_status(p, vmname):
        while True:
            lines = ""
            while True:
                line = p.stdout.readline().decode("utf-8")
                if line[0] == "=":
                    break
                if "Host" in line:
                    continue
                if "---" in line:
                    continue
                if "Total" in line:
                    continue
                if "Peak" in line:
                    continue
                if "Cumulative" in line:
                    continue
                if "address" in line:
                    continue
                if "interface" in line:
                    continue
                if "Listening" in line:
                    continue
                lines += line
            lines = lines.split()
            pos = 1
            while pos < len(lines):
                send_data = {}
                recv_data = {}
                if (lines[pos].isnumeric()):
                    pos = pos + 1
                send_data["host"] = lines[pos]
                pos = pos+1
                send_data["dir"] = lines[pos]
                pos = pos+1
                send_data["last2"] = lines[pos]
                pos = pos+1
                send_data["last10"] = lines[pos]
                pos = pos+1
                send_data["last40"] = lines[pos]
                pos = pos+1
                send_data["cumul"] = lines[pos]
                pos = pos+1
                json_send_data = json.dumps(send_data)
                if (lines[pos].isnumeric()):
                    pos = pos + 1
                recv_data["host"] = lines[pos]
                pos = pos+1
                recv_data["dir"] = lines[pos]
                pos = pos+1
                recv_data["last2"] = lines[pos]
                pos = pos+1
                recv_data["last10"] = lines[pos]
                pos = pos+1
                recv_data["last40"] = lines[pos]
                pos = pos+1
                recv_data["cumul"] = lines[pos]
                pos = pos+1
                json_recv_data = json.dumps(recv_data)
                if (send_data["host"] == vmname or recv_data["host"] == vmname):
                    emit("net", { 'data' : json_send_data }, broadcast=True)
                    emit("net", { 'data' : json_recv_data }, broadcast=True)
            if not line: break    

    command = "sudo iftop -t -i virbr"
    p = subprocess.Popen(command, stdout = subprocess.PIPE, stderr = subprocess.STDOUT, shell = True)
    thread = threading.Thread(target=get_net_status, args=(p, vmname, ))
    thread.start()

    print(vmname)

def process_syscall(list):
    data = {}

    data['pid'] = "0"
    data['name'] = "0"
    data['rdi'] = "0"
    data['rsi'] = "0"
    data['rdx'] = "0"
    data['r10'] = "0"
    data['r8'] = "0"
    data['r9'] = "0"

    try:
        data['pid'] = list[0]
        data['name'] = list[1]
        data['rdi'] = list[2]
        data['rsi'] = list[3]
        data['rdx'] = list[4]
        data['r10'] = list[5]
        data['r8'] = list[6]
        data['r9'] = list[7]
    except IndexError as e:
        None

    if data["name"] == "execve":
        print(data)
    return data

def educational_client(connection):
    try:
        start = "**"
        end = 0
        buffer = []
        while end == 0:
            bytes_recd = 0
            chunk = start

            chunk = connection.recv(1024)
            bytes_recd = bytes_recd + len(chunk)

            str_chunk = chunk.decode("utf-8")
            split = str_chunk.split('*')
            syscall = []
            for piece in split:
                if piece == '':
                        continue
                if piece == 'finished':
                        end = 1
                if piece == 'end':
                    data = process_syscall(syscall)
                    buffer.append(data)
                    if len(buffer) == 5:
                        json_data = json.dumps(buffer)
                        emit("syscall", { 'data' : json_data }, broadcast=True)
                        buffer = []
                    syscall = []
                else:
                    syscall.append(piece)
    except RuntimeError as e:
        print(e)
        return
                
if __name__ == '__main__':
    socketIO.run(app, debug=True)
    #socketIO.run(app)