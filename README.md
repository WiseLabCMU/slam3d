# slam3d

## MQTT Localize

1. Install paho library: https://github.com/eclipse/paho.mqtt.c

2. Compile: ```cmake .``` and then ```make```

2. Start script to request uwb ranges: ```./uwb_ctrl_publish.sh```

3. Start mqqtlocalize:
``` mqttlocalize [VIO input topic] [UWB input topic] [location updates output topic]```
> Example: ```./mqttlocalize /topic/render/camera_8034_nuno topic/uwb_range_mud /topic/render/camera_8034_nuno/rig```
