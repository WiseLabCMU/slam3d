#!/bin/bash

DEVNAME="pixel"
SCENENAME="earth"

eval CAMERAID="camera_${DEVNAME}_${DEVNAME}"
eval DEVVIO="/topic/vio/${CAMERAID}"
eval DEVUWB="topic/uwb_range_mud"
eval DEVLOCATION="/topic/loc/${CAMERAID}"
eval DEVVIZSCENE="realm/s/$SCENENAME/"

rm -f vio.log uwb.log loc_result.log viz_result.log || true

# kill previous instances; ATTENTION: will kill other python 3.7 processes...
pkill -KILL python3.7

#start a logger for each topic
python3.7 logger.py -f 'vio.log' -t $DEVVIO &
python3.7 logger.py -f 'uwb.log' -t $DEVUWB &
python3.7 logger.py -f 'loc_result.log' -t $DEVLOCATION &
python3.7 logger.py -f 'viz_result.log' -t $DEVVIZSCENE &

