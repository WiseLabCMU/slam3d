#!/bin/bash

while true; do 
	echo "Publishing..."
	mosquitto_pub -t 'topic/uwb_ctrl' -h oz.andrew.cmu.edu -m "mud.local"
	mosquitto_pub -t 'topic/uwb_ctrl' -h oz.andrew.cmu.edu -m "pot.local"
	sleep 10
done

