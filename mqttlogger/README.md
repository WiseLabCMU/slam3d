# MQTT Logger (tested on Ubuntu 18.04.1)

## Install dependencies
  * Python 3.7 (needed for ```time.time_ns()```); make sure this is the default python, or start env with it
  * Paho python library: ```pip3 install paho-mqtt```
  
## Create the topic handlers

Edit ```topic_handlers``` to add the functions that will receive the messages on the subscribed topics and convert them to the format you want to log.

The function handler name for a topic is derived from the topic name by replacing '/' by '_'. E.g.:
  * topic ```/topic/vio/camera_pixel_pixel``` should have a handler named ```_topic_vio_camera_pixel_pixel()```

Message topic handlers receive two arguments: the ```logger``` handle and the ```message``` received (see example handlers).

## Start the logger

To start the logger, make sure to give execute permissions to ```logger.py```.

### Usage: 

```
./logger.py [-h] [-s MQTTSERVER] [-f LOGFILE] [-t TOPICS]

optional arguments:
  -h, --help     show this help message and exit
  -s MQTTSERVER  MQTT Server; default=oz.andrew.cmu.edu
  -f LOGFILE     Log file; default=vio.log
  -t TOPICS      Subscribe topics; multiple -t are allowed;
                 default=['/topic/vio/camera_pixel_pixel']
```

Note: multiple topics can be given as argument. e.g.: ```-t topic1 -t topic2 -t topic3```

### Startup script

The script ```log_fixed_camera_solver.sh```
