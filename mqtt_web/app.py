import json
import paho.mqtt.client as mqtt
import time
from flask import Flask, send_from_directory,jsonify, request, Response, session, redirect, url_for
from dotenv import load_dotenv
from datetime import timedelta
import os
import queue

load_dotenv()
app = Flask(__name__)
DATA_FILE = "data.json"

app.secret_key = "your_secret_key_here"
app.permanent_session_lifetime = timedelta(minutes=10)

BROKER = "mqtt3.thingspeak.com"
PORT = 8883
CHANNEL_ID = os.getenv("CHANNEL_ID")
USERNAME = os.getenv("MQTT_USERNAME")
PASSWORD = os.getenv("MQTT_PASSWORD")
CLIENT_ID = os.getenv("CLIENT_ID")
SUB_TOPIC = f"channels/{CHANNEL_ID}/subscribe"
PUB_TOPIC = f"channels/{CHANNEL_ID}/publish"

alert_queues: list[queue.Queue] = []


@app.get("/add")
def add_page():
    return send_from_directory(".", "message.html")


@app.get("/events")
def events():
    def stream():
        q = queue.Queue()
        alert_queues.append(q)
        try:
            while True:
                try:
                    msg = q.get(timeout=25)
                    yield f"data: {msg}\n\n"
                except queue.Empty:
                    yield ": heartbeat\n\n"
        except GeneratorExit:
            alert_queues.remove(q)

    return Response(stream(), mimetype="text/event-stream",
                    headers={"Cache-Control": "no-cache", "X-Accel-Buffering": "no"})


@app.post("/setpoint")
def setpoint():
    body = request.get_json(force=True)
    sp = str(body["setpoint"])

    publish_topic = f"channels/{CHANNEL_ID}/publish"
    payload = f"field3={sp}&status=MQTTPUBLISH"

    print("Payload:",payload)

    mqtt_client.publish(publish_topic, payload)
    return ("OK", 200)

@app.get("/getsetpoint")
def getsetpoint():
    return jsonify({"setpoint": current_setpoint})

def broadcast_alert(alert_value: int):
    if alert_value == 1:
        text = "Water Sensor Alarm ON - Check your device!"
    else:
        text = ""

    msg = json.dumps({"alert": alert_value, "text": text, "t": int(time.time() * 1000)})
    for q in list(alert_queues):
        q.put(msg)

def on_connect(client, userdata, flags, rc):
    print("MQTT connected ",rc)
    client.subscribe(SUB_TOPIC)

current_setpoint = "--"
def on_message(client, userdata, msg):
    global current_setpoint
    payload = msg.payload.decode()
    print("MQTT received: ",payload)
    try:
        data_json = json.loads(payload)

        #field 3 set_rh display
        field3 = data_json.get("field3")
        if field3 is not None and str(field3).strip() != "":
            current_setpoint = field3
            print(f"Setpoint updated: {current_setpoint}")

        #field 4 alarm handling
        alarm = data_json.get("field4")
        if alarm is not None and str(alarm).strip() != "":
            try:
                alert_val = int(float(alarm))
                print(f"Alert field4={alert_val}")
                broadcast_alert(alert_val)
            except ValueError:
                pass

        #field 1 & 2 temp and hum sensor value
        if is_sensor_update(data_json):
            temp = float(data_json.get("field1", 0))
            hum  = float(data_json.get("field2", 0))

            with open(DATA_FILE, "r", encoding="utf-8") as f:
                data = json.load(f)

            data.append({
                "t": int(time.time()*1000),
                "temp": temp,
                "hum": hum,
                "alarm": alarm
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



VALID_USERNAME = os.getenv("VALID_USERNAME")
VALID_PASSWORD = os.getenv("VALID_PASSWORD")

@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "POST":
        username = request.form.get("username")
        password = request.form.get("password")

        if username == VALID_USERNAME and password == VALID_PASSWORD:
            session.permanent = True
            session["user"] = username
            return redirect(url_for("home"))
        else:
            return "Invalid credentials", 401

    return send_from_directory(".", "login.html")

@app.route('/icons/<path:filename>')
def serve_icons(filename):
    return send_from_directory('icons', filename)

@app.get("/logout")
def logout():
    session.clear()
    return redirect(url_for("login"))

@app.get("/")
def home():
    if "user" not in session:
        return redirect(url_for("login"))
    return send_from_directory(".", "index.html")

@app.get("/messages")
def get_messages():
    if "user" not in session:
        return ("Unauthorized", 401)

    if not os.path.exists(DATA_FILE):
        return jsonify([])

    with open(DATA_FILE, "r", encoding="utf-8") as file:
        data = json.load(file)

    return jsonify(data)
if __name__ == "__main__":
    app.run(host="127.0.0.1", port=3000, debug=False)

