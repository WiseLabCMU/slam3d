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
SCENE = 'gts'
TOPIC_DETECT = REALM + '/g/a/#'
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


def obj_to_sns(o):
    return SimpleNamespace(**o)


def dict_to_sns(d):
    return json.loads(json.dumps(d), object_hook=obj_to_sns)


TAG_1_POSE = dict_to_sns({'pose': {'elements': [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]}})
TAG_2_POSE = dict_to_sns({'pose': {'elements': [-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1]}})


class SyncUser:
    def __init__(self, scene, arenaname):
        self.hud = arena.Circle(parent=arenaname,
                                position=(0, 0, -0.5),
                                rotation=(0, 0, 0, 1),
                                scale=(0.02, 0.02, 0.02)
                                )
        self.scene = scene
        self.scene.add_object(self.hud)
        self.arenaname = arenaname
        self.reset()

    def reset(self):
        self.pose = None
        self.last_time = datetime.min
        self.state = STATE_WALK
        self.scene.update_object(self.hud, color=COLOR_WALK)

    def on_tag_detect(self, cam_pose, vio, time, debug):
        self.pose = cam_pose
        self.last_vio = vio
        self.last_time = time
        self.debug = debug
        if self.state == STATE_FINDTAG:
            self.state = STATE_WAIT
            self.scene.update_object(self.hud, color=COLOR_WAIT)

    def on_vio(self, vio, time):
        if self.state == STATE_WAIT:
            pos_diff, rot_diff = pose.pose_diff(vio, self.last_vio)
            time_diff = (time - self.last_time).total_seconds()
            if pos_diff > MOVE_THRESH or rot_diff > ROT_THRESH or time_diff > TIME_THRESH:
                self.state = STATE_FINDTAG
                self.scene.update_object(self.hud, color=COLOR_FINDTAG)

    def on_timer(self):
        if self.state == STATE_WALK:
            self.state = STATE_FINDTAG
            self.scene.update_object(self.hud, color=COLOR_FINDTAG)


class StaticUser(SyncUser):
    def __init__(self, arenaname, pose):
        self.arenaname = arenaname
        self.pose = np.array(pose)
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


def on_tag_detect(client, userdata, msg):
    global users
    json_msg = json.loads(msg.payload.decode('utf-8'), object_hook=obj_to_sns)
    client_id = msg.topic.split('/')[-1]
    if client_id not in users:
        return
    if hasattr(json_msg, 'detections') and len(json_msg.detections) > 0:
        if json_msg.detections[0].id == 1:
            json_msg.detections[0].refTag = TAG_1_POSE
        elif json_msg.detections[0].id == 2:
            json_msg.detections[0].refTag = TAG_2_POSE
        else:
            print('Unknown tag:', json_msg.detections[0].id)
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
            for u in users:
                users[u].reset()


def on_vio(msg):
    global users
    client_id = msg.object_id
    if client_id not in users:
        return
    vio_pose = pose.get_vio_pose(msg)
    time = datetime.strptime(msg.timestamp, TIME_FMT)
    users[client_id].on_vio(vio_pose, time)
    data = {'timestamp': time.strftime(TIME_FMT_UWB), 'type': 'vio', 'user': users[client_id].arenaname, 'pose': vio_pose.tolist()}
    with open(OUTFILE, 'a') as outfile:
        outfile.write(json.dumps(data))
        outfile.write(',\n')


def on_user_join(scene, cam, msg):
    global users
    global arenanames
    if cam.object_id in arenanames.values():
        print(cam.object_id + ' joined.')
        users[cam.object_id] = SyncUser(scene, cam.object_id)
    else:
        print(cam.object_id + ' not in userfile.')


def on_cam_msg(scene, obj, msg_dict):
    msg = dict_to_sns(msg_dict)
    on_vio(msg) 


def printhelp():
    print('gt-sync.py -u <userfile>')
    print('   ex: python gt-sync.py -u users.json')


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

scene = arena.Scene(host=HOST, realm=REALM, scene=SCENE)
for user in config:
    arenanames[user.uwbname] = user.arenaname
    if user.static:
        users[user.client_id] = StaticUser(user.arenaname, user.pose)
    else:
        print('Go to URL: https://arenaxr.org/' + USERNAME + '/' + SCENE + '?fixedCamera=' + user.arenaname + '&networkedTagSolver=true')

scene.mqttc.subscribe(TOPIC_DETECT)
scene.message_callback_add(TOPIC_DETECT, on_tag_detect)
scene.user_join_callback = on_user_join
scene.on_msg_callback = on_cam_msg

@scene.run_forever(interval_ms=TIME_INTERVAL*1000)
def prompt_users():
    global users
    for u in users:
        users[u].on_timer()

scene.run_tasks() # will block
