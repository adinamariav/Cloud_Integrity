from subprocess import Popen, PIPE, STDOUT
import threading
import shlex
import subprocess
import os

from click import command

def get_active_VMs():
    names = []

    result = subprocess.run(['sudo', 'xl', 'list'], stdout=subprocess.PIPE)
    output = result.stdout.decode('utf-8').split('\n')
    output = output[2:]
    output = output[:-1]

    for line in output:
        split = line.split( )
        names.append(split[0])
    
    return names

def run_command(command):
    subprocess.run(command, shell=1)

def start_educational(vmname, time):
    print(time)
    os.chdir('../../')
    command = "sudo ./vmi -m educational -v " + vmname + " -t " + time
    subprocess.run(command, shell=1)
    # p = Popen(command, stdout = PIPE, stderr = STDOUT, shell = True)

    # while True:
    #     line = p.stdout.readline()
    #     print("LINE " + line)
    #     if not line: break

    # thread = threading.Thread(target=run_command, args=(command, ))
    # thread.start()
    
    os.chdir('web/server/')

def start_iftop():
    command = "sudo iftop -i virbr -t"
    subprocess.run(command, shell=1)
    p = Popen(command, stdout = PIPE, stderr = STDOUT, shell = True)
    lines = []
    while True:
        lines = []
        while True:
            line = p.stdout.readline().decode("utf-8")
            if "=" in line:
                break
            if line != "" and "-" not in line:
                print("aici")
                lines.append(line)
        send_data = "".join(lines)
        if not line: break

    # thread = threading.Thread(target=run_command, args=(command, ))
    # thread.start()

if __name__ == "__main__":
    start_iftop()