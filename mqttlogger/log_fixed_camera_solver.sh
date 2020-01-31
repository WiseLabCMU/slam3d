#!/bin/bash

DEVNAME="pixel"
SCENENAME="earth"

eval CAMERAID="camera_${DEVNAME}_${DEVNAME}"
eval DEVVIO="/topic/vio/${CAMERAID}"
eval DEVUWB="topic/uwb_range_mud"
eval DEVVIZSCENE="realm/s/$SCENENAME/"

# copy previous logs
mkdir previous_logs 2>/dev/null || true
cp -f *log previous_logs || true

# delete previous logs
rm -f *log || true

#start a logger for each topic
echo -e "\nStarting logger instances... \n"

python3.7 logger.py -f 'vio.csv' -t $DEVVIO & echo $! > pids.txt
python3.7 logger.py -f 'uwb.csv' -t $DEVUWB & echo $! >> pids.txt
python3.7 logger.py -f 'viz_result.csv' -t $DEVVIZSCENE & echo $! >> pids.txt

sleep 1

echo -e "\nLogger instances started.\n\n"
read -p "** Press enter to finish logging **"

# kill logger instances
while read pid; do
  #echo "Killing "$pid
  kill -KILL $pid
done <pids.txt

