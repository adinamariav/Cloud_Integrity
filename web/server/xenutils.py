import subprocess

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