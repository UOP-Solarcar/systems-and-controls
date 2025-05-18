import paho.mqtt.client as mqtt
import time
import json

# MQTT Broker details
broker_address = "localhost"
port = 1883
topic = "solarcar/telematics"

# Create MQTT client with callback API version 2
client = mqtt.Client(
    client_id="SolarCarPublisher", protocol=mqtt.MQTTv5, callback_api_version=2
)

client.connect(broker_address, port)


# Function to read CAN data (placeholder)
def read_can_data():
    # Replace this with actual CAN data reading logic
    return {"speed": 60, "battery": 80}


# Publish data periodically
try:
    while True:
        data = read_can_data()
        client.publish(topic, json.dumps(data))
        time.sleep(1)  # Adjust the interval as needed
except KeyboardInterrupt:
    print("Publishing stopped.")
    client.disconnect()
