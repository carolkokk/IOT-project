# HUMIBOX(IOT-project):
Humibox is an IoT-based smart humidity control system to maintain optimal humidity levels for humidity-sensitive products such as musical instruments or cigars. The system automatically humidifies or dehumidifies the environment within the box to reach the target humidity level set by the users. The device is built with Raspberry Pi Pico W as the microcontroller, FreeRTOS, and C/C++. It integrates ThingSPEAK MQTT as the cloud service.

# Features of HUMIBOX:
- Humidity regulation (30% - 70 % relative humidity range)
- 4 preset humidity targets (for acoustic guitar, electric guitar, violin, cigars) available
- Water level detection from the water tank
- Real-time temperature and humidity monitoring
- Touch screen for displaying temperature and humidity data, setting humidity level, connecting to the internet, showing historical data graphs, and registering logs.
- IoT connectivity via MQTT
- Web interface for monitoring data and setting the relative humidity level

# System Operation:
Humidity Control Logic: 
If the humidity is below the set target, the humidifier activates.
If the humidity is above the set target, the dehumidifier activates. 
The system is in idle mode when the target is reached. To avoid frequent switching, a +-5% buffer zone is implemented. For instance, if the target humidity level is 50%, once the target is reached, the system remains in idle mode when the humidity is between 45% and 55%. 

# Humidifying system:
The humidifier uses a piezoelectric mist generator based on LC resonance. 

# Dehumidifying system: 
The dehumidifier uses a Peltier thermoelectric cooler. The cold side is placed inside the box with an aluminum radiator kit and a fan. Moisture condenses on the aluminum radiator. Water droplets drip into the water tank. The hot side of the Peltier is placed outside the box with a fan to dissipate heat. 

# Water management:
The system uses the same water tank both for the humidifier and for the dehumidifier. The water dripped from the dehumidifier could be gathered back to the water tank and reused for the humidifier. It uses two water level sensors. One for detecting water overflow and the other one for detecting low water level. If abnormal water levels are detected, the humidifying and dehumidifying are disabled, and the system sends the alarm both to the screen and to the internet (when connected) to inform the users. 

# System Architecture
The software runs on FreeRTOS and is divided into three main tasks. 
# Control Task:
- Reading from humidity sensors and send data to UI and network (if connected)
- Activating the humidifier or dehumidifier
- Checks water level sensors

# UI Task: 
- Display temperature and humidity
- Connect to the internet
- Set relative humidity level
- Show historical data
- Display system log

# Network Task:
- Wifi scanning and connectivity
- MQTT communication
- Send data to the cloud
- Receive data from the cloud

# Setup
Clone the repository and initialize submodules:
git submodule update --init --recursive

# Configuration: 
1. A file named configpass.h needs to be created under the src directory. It must contain the wifi credentials, thingspeak MQTT credentials for pico, and the thingspeak server certificate.

Example structure: 
#define TLS_THINGSPEAK_SERVER "thingspeak server certificate"
#define WIFI_SSID "your wifi ssid"
#define WIFI_PASSWORD "your wifi password"
#define MQTT_CLIENT_ID "the thingspeak mqtt client id created for pico"
#define MQTT_USERNAME "the thingspeak mqtt username created for pico"
#define MQTT_PASSWORD "the thingspeak mqtt password created for pico"

2. A file named .env needs to be created under the mqtt_web directory. It must contain the thingspeak MQTT credentials for the web, the thingspeak channel id, and the username and password for user logging in to the website.

Example structure:
CHANNEL_ID= your_channel_id
MQTT_USERNAME= thingspeak_mqtt_username_for_web
MQTT_PASSWORD= thingspeak_mqtt_password_for_web
CLIENT_ID= thingspeak_mqtt_client_id_for_web

VALID_USERNAME= username_for_logging_in
VALID_PASSWORD= password_for_logging_in

