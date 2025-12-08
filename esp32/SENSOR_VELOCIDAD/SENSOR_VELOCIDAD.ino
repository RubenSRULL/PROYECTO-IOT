// --- LIBRERÍAS --- //
#include <PubSubClient.h>
#include <WiFi.h>

// --- PINES --- //
#define SENSOR_HALL 25

// --- CONFIGURACIÓN --- //
const char* ssid = "OPPO A53";
const char* password = "611b10a883c5";
const char* mqtt_server = "10.74.94.63";

// --- TIEMPOS --- //
const unsigned long SAMPLE_INTERVAL = 1000;
unsigned long lastSample = 0;

// --- CONTADOR DE PULSOS --- //
volatile int contador = 0;
volatile unsigned long lastPulseTime = 0;

// --- OBJETOS --- //
WiFiClient espClient;
PubSubClient client(espClient);
String ip = "";

// --- ISR --- //
void IRAM_ATTR pulseISR() {
  unsigned long now = micros();
  if (now - lastPulseTime > 500) {
    contador++;
    lastPulseTime = now;
  }
}

// --- LEER PULSOS --- //
int leerPulsos() {
  noInterrupts();
  int p = contador;
  contador = 0;
  interrupts();
  return p;
}

// --- CALLBACK --- //
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  ip = msg;
}

// --- WIFI --- //
void configWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Conectando");

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
}

// --- MQTT --- //
void MQTTreconnect() {

  while (!client.connected()) {
    if (WiFi.status() != WL_CONNECTED) configWifi();

    Serial.print("Intentando MQTT... ");

    if (client.connect("ESP32_HallSensor")) {
      Serial.println("OK");
      client.subscribe("esp32/ip");
    } else {
      Serial.print("Error: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void publishRPM(float rpm) {
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%.0f", rpm);
  client.publish("esp32/hz", buffer);
}

void setup() {
  Serial.begin(115200);

  configWifi();

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(SENSOR_HALL, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SENSOR_HALL), pulseISR, FALLING);

  lastSample = millis();
}

void loop() {
  if (!client.connected()) MQTTreconnect();
  client.loop();

  unsigned long now = millis();

  if (now - lastSample >= SAMPLE_INTERVAL) {

    float pulsos = float(leerPulsos()) / 2.00;

    publishRPM(pulsos);
    Serial.println(pulsos);

    lastSample = now;
  }
}

