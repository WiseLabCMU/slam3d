# function to handle a topic is derived from topic name by replacing '/' by '_'. E.g.:
# topic '/topic/vio/camera_pixel_pixel' needs a function named _topic_vio_camera_pixel_pixel()
import json

def _topic_vio_camera_pixel_pixel(logger, msg):
    d_msg = str(msg.payload.decode("utf-8","ignore"))
    try:
        vio = json.loads(d_msg)
        logger.info(str(vio['data']['position']['x'])+','+str(vio['data']['position']['y'])+','+str(vio['data']['position']['z'])+','+
      str(vio['data']['rotation']['x'])+','+str(vio['data']['rotation']['y'])+','+str(vio['data']['rotation']['z'])+','+str(vio['data']['rotation']['w']))
    except Exception as e:
        print("Couldn't get JSON data: %s" % d_msg, e)
