import os
import subprocess
import time

mosquitto_path = r"C:\Program Files\mosquitto\mosquitto.exe"
mosquitto_conf = r"C:\Program Files\mosquitto\mosquitto.conf"


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


if __name__ == '__main__':
    iniciar_broker()