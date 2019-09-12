# slam3d

1. Start script to request uwb ranges: ```./uwb_ctrl_publish.sh```

2. Start mqqtlocalize:
``` mqttlocalize [VIO input topic] [UWB input topic] [location updates output topic]```
> Example: ```./mqttlocalize /topic/render/camera_8034_nuno topic/uwb_range_mud /topic/render/camera_8034_nuno/rig```
