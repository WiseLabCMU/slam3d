# gt-sync.py
''' Synchronize users for ground truth collection with apriltags
'''

import arena
from datetime import datetime
import getopt
import json
import numpy as np
import os
import pose
from types import SimpleNamespace
import sys

HOST = 'arenaxr.org'
USERNAME = 'john'
REALM = 'realm'
SCENE = 'gt'
TOPIC_DETECT = REALM + '/g/a/#'
TOPIC_VIO = '/topic/vio/#'
TOPIC_UWB = REALM + '/g/uwb/#'
TIME_FMT = '%Y-%m-%dT%H:%M:%S.%fZ'
TIME_FMT_UWB = '%Y-%m-%dT%H:%M:%S.%f'
OUTFILE = os.path.join('gt', datetime.now().strftime('%Y-%m-%d_%H_%M_%S') + '.json')

STATE_WALK = 0
STATE_FINDTAG = 1
STATE_WAIT = 2
COLOR_WALK = (0, 255, 0)
COLOR_FINDTAG = (255, 0, 0)
COLOR_WAIT = (0, 0, 255)
MOVE_THRESH = .05   # 5cm
ROT_THRESH = .087   # 5deg
TIME_THRESH = 3     # 3sec
DTAG_ERROR_THRESH = 5e-6    # tag detection error units?
TIME_INTERVAL = 5           # 5sec

users = {}
arenanames = {}
last_detection = datetime.min


class SyncUser:
    def __init__(self, config):
        self.hud = arena.Circle(parent=config.client_id,
                                position=(0, 0, -0.5),
                                rotation=(0, 0, 0, 1),
                                scale=(0.02, 0.02, 0.02)
                                )
        self.arenaname = config.arenaname
        self.reset()

    def reset(self):
        self.pose = None
        self.last_time = datetime.min
        self.state = STATE_WALK
        self.hud.update(color=COLOR_WALK)

    def on_tag_detect(self, cam_pose, vio, time, debug):
        self.pose = cam_pose
        self.last_vio = vio
        self.last_time = time
        self.debug = debug
        if self.state == STATE_FINDTAG:
            self.state = STATE_WAIT
            self.hud.update(color=COLOR_WAIT)

    def on_vio(self, vio, time):
        if self.state == STATE_WAIT:
            pos_diff, rot_diff = pose.pose_diff(vio, self.last_vio)
            time_diff = (time - self.last_time).total_seconds()
            if pos_diff > MOVE_THRESH or rot_diff > ROT_THRESH or time_diff > TIME_THRESH:
                self.state = STATE_FINDTAG
                self.hud.update(color=COLOR_FINDTAG)

    def on_timer(self):
        if self.state == STATE_WALK:
            self.state = STATE_FINDTAG
            self.hud.update(color=COLOR_FINDTAG)


class StaticUser(SyncUser):
    def __init__(self, config):
        self.arenaname = config.arenaname
        self.pose = np.array(config.pose)
        self.state = STATE_WAIT
        self.debug = 'static'

    def reset(self):
        pass

    def on_tag_detect(self, cam_pose, vio, time):
        pass

    def on_vio(self, vio, time):
        pass

    def on_timer(self):
        pass


def printhelp():
    print('gt-sync.py -s <scene> -u <userfile>')
    print('   ex: python3 gt-sync.py -s myScene -u users.json')


def dict_to_sns(d):
    return SimpleNamespace(**d)


def on_tag_detect(msg):
    global users
    global last_detection
    json_msg = json.loads(msg.payload.decode('utf-8'), object_hook=dict_to_sns)
    client_id = msg.topic.split('/')[-1]
    if client_id not in users:
        return
    if hasattr(json_msg, 'detections'):
        dtag = json_msg.detections[0]
        if not hasattr(dtag, 'refTag'):
            print('tag not in atlas:', dtag.id)
            return
        cam_pose, dtag_error = pose.get_cam_pose(json_msg)
        if dtag_error > DTAG_ERROR_THRESH:
            return
        vio_pose = pose.get_vio_pose(json_msg)
        time = datetime.strptime(json_msg.timestamp, TIME_FMT)
        users[client_id].on_tag_detect(cam_pose, vio_pose, time, msg.payload.decode('utf-8'))
        if all(users[u].state == STATE_WAIT for u in users):
            poselist = [
                {
                    'user': users[u].arenaname,
                    'pose': users[u].pose.tolist(),
                    'dbg':  users[u].debug
                }
                for u in users
            ]
            data = {
                'timestamp': time.strftime(TIME_FMT_UWB),
                'type':      'gt',
                'poses':     poselist
            }
            print(data)
            with open(OUTFILE, 'a') as outfile:
                outfile.write(json.dumps(data))
                outfile.write(',\n')
            last_detection = time
            for u in users:
                users[u].reset()


def on_vio(msg):
    global users
    json_msg = json.loads(msg.payload.decode('utf-8'), object_hook=dict_to_sns)
    client_id = msg.topic.split('/')[-1]
    if client_id not in users:
        return
    if hasattr(json_msg, 'object_id') and json_msg.object_id.endswith('_local'):
        vio_pose = pose.get_vio_pose(json_msg)
        time = datetime.strptime(json_msg.timestamp, TIME_FMT)
        users[client_id].on_vio(vio_pose, time)
        data = {'timestamp': time.strftime(TIME_FMT_UWB), 'type': 'vio', 'user': users[client_id].arenaname, 'pose': vio_pose.tolist()}
        with open(OUTFILE, 'a') as outfile:
            outfile.write(json.dumps(data))
            outfile.write(',\n')


def on_uwb(msg):
    json_msg = json.loads(msg.payload.decode('utf-8'), object_hook=dict_to_sns)
    time = datetime.strptime(json_msg.timestamp, TIME_FMT_UWB)
    if not json_msg.src in arenanames:
        print('User not tracked:', json_msg.src)
        return
    if not json_msg.dst in arenanames:
        print('User not tracked:', json_msg.dst)
        return
    src = arenanames[json_msg.src]
    dst = arenanames[json_msg.dst]
    rng = float(json_msg.distance)
    rssi = int(json_msg.ble_rssi)
    data = {'timestamp': time.strftime(TIME_FMT_UWB), 'type': 'uwb', 'src': src, 'dst': dst, 'range': rng, 'ble_rssi': rssi}
    with open(OUTFILE, 'a') as outfile:
        outfile.write(json.dumps(data))
        outfile.write(',\n')


try:
    opts, args = getopt.getopt(sys.argv[1:], 'hu:', ['userfile='])
except getopt.GetoptError:
    printhelp()
    sys.exit(1)
userfile = None
for opt, arg in opts:
    if opt == '-h':
        printhelp()
        sys.exit(1)
    elif opt in ('-u', '--userfile'):
        userfile = arg
    else:
        printhelp()
        sys.exit(1)
if userfile is None:
    printhelp()
    sys.exit(1)
with open(userfile, 'r') as f:
    config = json.load(f, object_hook=dict_to_sns)
for user in config:
    arenanames[user.uwbname] = user.arenaname
    if user.static:
        users[user.client_id] = StaticUser(user)
    else:
        users[user.client_id] = SyncUser(user)
        print('Go to URL: ' + HOST + '/' + USERNAME + '/' + SCENE + "/?networkedTagSolver=true")

scene = arena.Scene(host=HOST, realm=REALM, scene=SCENE)
@scene.run_forever(interval_ms=TIME_INTERVAL*1000)
def prompt_users():
    global users
    for u in users:
        users[u].on_timer()

scene.add_topic(TOPIC_DETECT, on_tag_detect)
scene.add_topic(TOPIC_VIO, on_vio)
scene.add_topic(TOPIC_UWB, on_uwb)

scene.run_tasks() # will block
