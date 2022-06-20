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
import csv
import datetime
import pandas

from xenutils import get_active_VMs, start_educational
import sys

import sys
sys.path.append('../../server/')

from server import learn, analyze

app = Flask(__name__, static_url_path='')

CORS(app) 
api = Api(app)

UPLOAD_FOLDER = './files'
fname = ""

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
    p = subprocess.Popen(command, shell = True)

@socketIO.on("startEducational")
def start(vm, set_time):
    date = datetime.datetime.now()
    fname = "files/sessions/educational/" + str(date.day) + "-" + str(date.month) + "-" + str(date.year) + "-" + str(date.hour) + "-" + str(date.minute) + ".csv"
    @copy_current_request_context
    def thread_serve(port, filename):
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
        educational_client(Client, filename)
        ServerSocket.close()

    thread = threading.Thread(target=thread_serve, args=(1234, fname, ))
    thread.start()

    @copy_current_request_context
    def send_events(filename):
        print(filename)
        time.sleep(2)
        prev_df = pandas.read_csv(filename)
        while True:
            df = pandas.read_csv(filename)
            len_prev = len(prev_df.index)
            len_curr = len(df.index)
            diff_df = df.iloc[len_prev:len_curr]
            diff_df.fillna('null', inplace=True)
            data = diff_df.to_dict('records')
            for item in data[:]:
                if item["name"] == "clock_gettime":
                    data.pop(data.index(item))
                if item["name"] == "pselect6":
                    data.pop(data.index(item))
                if item["name"] == "inotify_add_watch":
                    data.pop(data.index(item))

                item["details"] = ""
                
                if len(str(item["rsi"])) > 10:
                    item["details"] += item["rsi"]
                    item["rsi"] = item["rsi"][:10]
                    item["rsi"] = item["rsi"] + "..."

                if len(str(item["rdi"])) > 10:
                    item["details"] += item["rdi"]
                    item["rdi"] = item["rdi"][:10]
                    item["rdi"] = item["rdi"] + "..."

                if len(str(item["rdx"])) > 10:
                    item["rdx"] = item["rdx"][:10]

                if len(str(item["r10"])) > 10:
                    item["r10"] = item["r10"][:10]

                if len(str(item["r8"])) > 10:
                    item["r8"] = item["r8"][:10]

                if len(str(item["r9"])) > 10:
                    item["r9"] = item["r9"][:10]
                
            if data != []:
                len_data = len(data)
                for i in range(0, len_data, 200):
                    json_data = json.dumps(data[i : i + 200])
                    emit("syscall", { 'data' : json_data }, broadcast=True)
            prev_df = df
            time.sleep(2)

    os.chdir('../../')
    command = "sudo ./vmi -m educational -v " + vm + " -t " + set_time
    thread = threading.Thread(target=run_command, args=(command, ))
    thread.start()
    os.chdir('web/server/')
    thread_send = threading.Thread(target=send_events, args=(fname, ))
    thread_send.start()

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
            line = p.stdout.readline().decode("utf-8")
            if vmname not in line:
                continue
            if "ARP" in line:
                continue
            split = line.split()
            send_data = {}
            send_data["time"] = split[0]
            send_data["from"] = split[2]
            send_data["to"] = split[4][:-1]
            send_data["len"] = split[-1]
            send_data["details"] = " ".join(split[1:])
            if send_data["len"][-1] == ")":
                send_data["len"] = send_data["len"]
            emit("net", { 'data' : json.dumps(send_data)})
            if not line: break

    command = "sudo tcpdump -i virbr --immediate-mode"
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

    return data

def educational_client(connection, fname):
    try:
        start = "**"
        end = 0
        buffer = []
        csv_columns = ['id', 'pid','name','rdi','rsi','rdx','r10','r8','r9']
        csv_file = open(fname, "w")
        csv_writer = csv.DictWriter(csv_file, delimiter=",", fieldnames=csv_columns)
        csv_writer.writeheader()
        id = 0
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
                    data["id"] = id
                    id = id + 1
                    buffer.append(data)
                    # if len(buffer) == 5:
                    #     json_data = json.dumps(buffer)
                    #     emit("syscall", { 'data' : json_data }, broadcast=True)
                    #     buffer = []
                    csv_writer.writerow(data)
                    csv_file.flush()
                    syscall = []
                else:
                    syscall.append(piece)
    except RuntimeError as e:
        print(e)
        return
                
if __name__ == '__main__':
    socketIO.run(app, debug=True)
    #socketIO.run(app)