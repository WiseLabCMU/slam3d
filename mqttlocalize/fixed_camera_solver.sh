#!/bin/bash

DEVNAME="pixel"
SCENENAME="earth"

eval CAMERAID="camera_${DEVNAME}_${DEVNAME}"
eval DEVVIO="/topic/vio/${CAMERAID}"
eval DEVUWB="topic/uwb_range_mud"
eval DEVVIZSCENE="realm/s/$SCENENAME/"

# check if we requesting ranges from tags
if ! pidof -x "uwb_ctrl_publish.sh" >/dev/null; then
    echo "Start requesting ranges from tags"
    ./uwb_ctrl_publish.sh >/dev/null & # script that periodically tells the tags to publish ranges
fi

./mqttlocalize ${DEVVIO} ${DEVUWB} ${DEVVIZSCENE} ${CAMERAID}
