import os
import subprocess
import time
import paho.mqtt.client as mqtt

mosquitto_path = r"C:\Program Files\mosquitto\mosquitto.exe"
mosquitto_conf = r"C:\Program Files\mosquitto\mosquitto.conf"

topicReceive = "esp32/hz"
broker_IP = "10.74.94.63"
broker_PORT = 1883

client = None
Hz = None


# ---------------------------------------------------
# 1. Iniciar Mosquitto si no está ejecutándose
# ---------------------------------------------------
def iniciar_broker():
    try:
        procesos = os.popen('tasklist').read().lower()
        if "mosquitto.exe" not in procesos:
            subprocess.Popen(
                f'"{mosquitto_path}" -v -c "{mosquitto_conf}"',
                shell=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            )
            print("Broker Mosquitto iniciado en segundo plano.")
            time.sleep(3)
        else:
            print("Broker Mosquitto ya está ejecutándose.")
    except Exception as e:
        print(f"Error al iniciar el broker: {e}")


# ---------------------------------------------------
# 2. Callback de recepción
# ---------------------------------------------------
def on_message(client, userdata, msg):
    global Hz
    try:
        Hz = float(msg.payload.decode())
    except ValueError:
        Hz = None


# ---------------------------------------------------
# 3. Iniciar cliente MQTT
# ---------------------------------------------------
def iniciar_mqtt():
    global client
    try:
        client = mqtt.Client()
        client.on_message = on_message

        print("Conectando al broker MQTT...")
        client.connect(broker_IP, broker_PORT)

        client.subscribe(topicReceive)
        client.loop_start()
        print("Cliente MQTT conectado.")
    except Exception as e:
        print(f"Error al conectar MQTT: {e}")


# ---------------------------------------------------
# 4. Programa principal
# ---------------------------------------------------
if __name__ == '__main__':
    iniciar_broker()
    iniciar_mqtt()

    print("\nEscuchando mensajes...\n")

    last = None

    while True:
        if Hz != last:
            print(f"Hz recibido: {Hz}")
            last = Hz
        time.sleep(0.05)
