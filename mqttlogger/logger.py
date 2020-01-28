#!/usr/bin/python3.7
import argparse
import paho.mqtt.client as mqtt
import logging
import time
import topic_handlers # handlers for topics are in topic_handlers.py

# default logfile; if not given as argument
DFT_LOGFILE = "data.log"

# default mqtt server; if not given as argument
DFT_MQTT_SERVER = "oz.andrew.cmu.edu"

# default topic list; if not given as argument; format: ['topic1', 'topic2', ...]
DFT_TOPICS = ['/topic/vio/camera_pixel_pixel']

def on_connect(mqttc, obj, flags, rc):
    #print("rc: " + str(rc))
    if rc==0:
        printf("Connected.")
    else:
        printf("Error Connecting")

def on_message(mqttc, obj, msg):
    # get handler to call from topic name; rplaces '/' by '_'. E.g.:
    #    handler for topic '/topic/vio/camera_pixel_pixel' is _topic_vio_camera_pixel_pixel()
    topic_handler_name = msg.topic.replace('/', '_')
    try:
        topic_handler = getattr(topic_handlers, topic_handler_name)
    except:
        print("Error: Could not find handler for topic ", msg.topic, "(is ", topic_handler_name, " defined in topic_handlers.py ?)")
    topic_handler(logger, msg)

#def on_publish(mqttc, obj, mid):
    #print("mid: " + str(mid))

#def on_subscribe(mqttc, obj, mid, granted_qos):
    #print("Subscribed.")

def on_log(mqttc, obj, level, string):
    print(string)

class MyFormatter(logging.Formatter):
    def formatTime(self, record, datefmt=None):
        s = "%s" % str(time.time_ns()/1000000000)
        return s

# parse arguments
parser = argparse.ArgumentParser()

parser.add_argument('-s', action='store', dest='mqttserver', default=DFT_MQTT_SERVER, help='MQTT Server; default='+DFT_MQTT_SERVER)
                    
parser.add_argument('-f', action='store', dest='logfile', default=DFT_LOGFILE, help='Log file; default='+DFT_LOGFILE)

parser.add_argument('-t', action='append', dest='topics', default=[], help='Subscribe topics; multiple -t are allowed; default='+str(DFT_TOPICS))

args = parser.parse_args()

# setup logging
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
fh = logging.FileHandler(args.logfile)
#logger.addHandler(ch) # uncomment to print log messages to console
logger.addHandler(fh)

formatter = MyFormatter(fmt='%(asctime)s,%(message)s',datefmt='%Y-%m-%d,%H:%M:%S.%f')
ch.setFormatter(formatter)
fh.setFormatter(formatter)

# setup mqtt client
mqttc = mqtt.Client()
mqttc.on_message = on_message
mqttc.on_connect = on_connect
#mqttc.on_publish = on_publish
#mqttc.on_subscribe = on_subscribe
# Uncomment to enable debug messages
# mqttc.on_log = on_log

mqttc.connect(args.mqttserver, 1883, 60)

if len(args.topics) > 0:
    topic_list=args.topics
else:
    topic_list=DFT_TOPICS

for t in topic_list: 
	print("Subscribing to: ",t)
	mqttc.subscribe(t, qos = 1)

mqttc.loop_forever()
