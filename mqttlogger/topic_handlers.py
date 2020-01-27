# function to handle a topic is derived from topic name by replacing '/' by '_'. E.g.:
# topic '/topic/vio/camera_pixel_pixel' needs a function named _topic_vio_camera_pixel_pixel()
import json

# this is an utility function to handle all ARENA JSON messages
def arena_message_handler(logger, msg):
    d_msg = str(msg.payload.decode("utf-8","ignore"))
    try:
        vio = json.loads(d_msg)
        logger.info(str(vio['data']['position']['x'])+','+str(vio['data']['position']['y'])+','+str(vio['data']['position']['z'])+','+
      str(vio['data']['rotation']['x'])+','+str(vio['data']['rotation']['y'])+','+str(vio['data']['rotation']['z'])+','+str(vio['data']['rotation']['w']))
    except Exception as e:
        print("Couldn't get JSON log data: %s" % d_msg, e)

# handler for messages published to topic: '/topic/vio/camera_pixel_pixel'
def _topic_vio_camera_pixel_pixel(logger, msg):
    arena_message_handler(logger, msg)

# handler for messages published to topic: '/topic/loc/camera_pixel_pixel'
def _topic_loc_camera_pixel_pixel(logger, msg):
    arena_message_handler(logger, msg)

# handler for messages published to topic: 'realm/s/earth/'
def realm_s_earth_(logger, msg):
    arena_message_handler(logger, msg) # message format is similar

# handler for messages published to topic: ' topic/uwb_range_mud'
def topic_uwb_range_mud(logger, msg):
    d_msg = str(msg.payload.decode("utf-8","ignore"))
    try:
        logger.info(d_msg)
    except Exception as e:
        print("Couldn't get log data: %s" % d_msg, e)


