import json
import paho.mqtt.client as mqtt
import time
from flask import Flask, send_from_directory,jsonify, request
from dotenv import load_dotenv
import os

load_dotenv()
app = Flask(__name__)
DATA_FILE = "data.json"

BROKER = "mqtt3.thingspeak.com"
PORT = 8883
CHANNEL_ID = os.getenv("CHANNEL_ID")
USERNAME = os.getenv("MQTT_USERNAME")
PASSWORD = os.getenv("MQTT_PASSWORD")
CLIENT_ID = os.getenv("CLIENT_ID")
SUB_TOPIC = f"channels/{CHANNEL_ID}/subscribe"
PUB_TOPIC = f"channels/{CHANNEL_ID}/publish"


@app.get("/")
def home():
    return send_from_directory(".", "index.html")

@app.get("/add")
def add_page():
    return send_from_directory(".", "message.html")

@app.get("/messages")
def get_messages():
    with open(DATA_FILE, "r", encoding="utf-8") as file:
        data = json.load(file)
    return jsonify(data)

@app.post("/setpoint")
def setpoint():
    body = request.get_json(force=True)
    sp = str(body["setpoint"])

    publish_topic = f"channels/{CHANNEL_ID}/publish"
    payload = f"field3={sp}&status=MQTTPUBLISH"

    print("Payload:",payload)

    mqtt_client.publish(publish_topic, payload)
    return ("OK", 200)

def on_connect(client, userdata, flags, rc):
    print("MQTT connected ",rc)
    client.subscribe(SUB_TOPIC)

def on_message(client, userdata, msg):
    payload = msg.payload.decode()
    print("MQTT received: ",payload)
    try:
        data_json = json.loads(payload)

        if is_sensor_update(data_json):
            temp = float(data_json.get("field1", 0))
            hum  = float(data_json.get("field2", 0))

            with open(DATA_FILE, "r", encoding="utf-8") as f:
                data = json.load(f)

            data.append({
                "t": int(time.time()*1000),
                "temp": temp,
                "hum": hum
            })

            data = data[-30:]

            with open(DATA_FILE, "w", encoding="utf-8") as f:
                json.dump(data, f)

            print("Saved to JSON:", temp, hum)

        else:
            return

    except Exception as e:
        print("Error:", e)

def is_sensor_update(d: dict) -> bool:
    f1, f2 = d.get("field1"), d.get("field2")
    if f1 is None or f2 is None:
        return False
    if str(f1).strip() == "" or str(f2).strip() == "":
        return False
    try:
        float(f1); float(f2)
        return True
    except ValueError:
        return False

mqtt_client = mqtt.Client(client_id=CLIENT_ID)
mqtt_client.username_pw_set(USERNAME, PASSWORD)
mqtt_client.tls_set()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(BROKER, PORT, 60)
mqtt_client.loop_start()


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=3000, debug=True)