from flask import Flask, request
from flask_restful import Api
from flask_cors import CORS
import json
from xenutils import get_active_VMs

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

@app.route("/analyze")
def analyze():
    name = request.form['name']
    time = request.form['time']
    learn(name, time)

@app.route("/")
def serve2():
    return "Tralala"
                
if __name__ == '__main__':
    app.run()