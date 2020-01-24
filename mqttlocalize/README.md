# MQTT Localize (tested on Ubuntu 18.04.1)

1. Install dependencies:
  * Paho c library: https://github.com/eclipse/paho.mqtt.c
  * Mosquitto clients: ```sudo apt-get install mosquitto-clients```
  
2. Compile (inside the mqttlocalize folder): ```cmake .``` and then ```make```

3. Edit the script ```fixed_camera_ipad_pot_solver.sh``` to adjust for the camera name and scene.

4. Run script to start requesting uwb ranges and the solver: ```./fixed_camera_ipad_pot_solver.sh```

