from subprocess import Popen, PIPE, STDOUT
import threading
import shlex
import subprocess
import os
import pcap

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
    sniffer = pcap.pcap(name="virbr", promisc=True, immediate=True, timeout_ms=50)
    addr = lambda pkt, offset: '.'.join(str(ord(pkt[i])) for i in range(offset, offset + 4))
    for ts, pkt in sniffer:
        print('%d\tSRC %-16s\tDST %-16s' % (ts, addr(pkt, sniffer.dloff + 12), addr(pkt, sniffer.dloff + 16)))

if __name__ == "__main__":
    start_iftop()