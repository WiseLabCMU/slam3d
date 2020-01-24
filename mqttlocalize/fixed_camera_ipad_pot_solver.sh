#!/bin/bash

IPADNAME="pixel"
SCENENAME="earth"

eval CAMERAID="camera_${IPADNAME}_${IPADNAME}"
eval IPADVIO="/topic/vio/${CAMERAID}"
eval IPADUWB="topic/uwb_range_mud"
eval IPADLOCATION="/topic/loc/${CAMERAID}"
eval IPADVIZ="realm/s/$SCENENAME/"

# check if we requesting ranges from tags
if ! pidof -x "uwb_ctrl_publish.sh" >/dev/null; then
    echo "Start requesting ranges from tags"
    ./uwb_ctrl_publish.sh >/dev/null & # script that periodically tells the tags to publish ranges
fi

./mqttlocalize ${IPADVIO} ${IPADUWB} ${IPADLOCATION} ${IPADVIZ} ${CAMERAID}
