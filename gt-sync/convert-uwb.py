''' Convert UWB log to gt format and append to gt trace
'''
from datetime import datetime
import getopt
import json
import numpy as np
import os
import sys
from types import SimpleNamespace

TIME_FMT = '%Y-%m-%dT%H:%M:%S.%fZ'
TIME_FMT_UWB = '%Y-%m-%dT%H:%M:%S.%f'


def printhelp():
    print('convert-uwb.py -u <userfile> -g <gtfile> -r <rangefile>')
    print('   ex: python3 convert-uwb.py -u users.json -g gt-trace.json -r ranges.json')


def dict_to_sns(d):
    return SimpleNamespace(**d)


def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'hu:g:r:', ['userfile=', 'gtfile=', 'rangefile='])
    except getopt.GetoptError:
        printhelp()
        sys.exit(1)

    userfile = None
    gtfile = None
    rangefile = None
    for opt, arg in opts:
        if opt == '-h':
            printhelp()
            sys.exit(1)
        elif opt in ('-u', '--userfile'):
            userfile = arg
        elif opt in ('-g', '--gtfile'):
            gtfile = arg
        elif opt in ('-r', '--rangefile'):
            rangefile = arg
        else:
            printhelp()
            sys.exit(1)
    if userfile is None or gtfile is None or rangefile is None:
        printhelp()
        sys.exit(1)

    with open(userfile, 'r') as f:
        config = json.load(f, object_hook=dict_to_sns)

    arenanames = {}
    for user in config:
        arenanames[user.uwbname] = user.arenaname

    with open(rangefile, 'r') as f:
        ranges = json.load(f, object_hook=dict_to_sns)

    for r in ranges:
        time = datetime.strptime(r.timestamp, TIME_FMT_UWB)
        if not r.src in arenanames:
            print('User not tracked:', r.src)
            continue
        if not r.dst in arenanames:
            print('User not tracked:', r.dst)
            continue
        src = arenanames[r.src]
        dst = arenanames[r.dst]
        rng = float(r.distance)
        rssi = int(r.ble_rssi)
        data = {'timestamp': time.strftime(TIME_FMT_UWB), 'type': 'uwb', 'src': src, 'dst': dst, 'range': rng, 'ble_rssi': rssi}
        with open(gtfile, 'a') as outfile:
            outfile.write(json.dumps(data))
            outfile.write(',\n')


if __name__ == '__main__':
    try:
        main()
    except SystemExit:
        pass
