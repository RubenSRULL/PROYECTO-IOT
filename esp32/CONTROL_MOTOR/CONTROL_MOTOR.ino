// --- LIBRERÍAS --- //
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>

// --- ESTADOS MOTOR --- //
#define PARADA     0
#define GIRO_CW    1
#define GIRO_CCW   2

int estado = PARADA;
int estadoAnterior = PARADA;

// --- ETAPAS DE ARRANQUE --- //
#define ETAPA_IDLE        0
#define ETAPA_ESTRELLA    1
#define ETAPA_TRIANGULO   2

int etapa = ETAPA_IDLE;
unsigned long etapaStart = 0;

// --- PINES RELES --- //
const int rele1 = 26;
const int rele2 = 27;
const int rele3 = 12;
const int rele4 = 14;

// --- WIFI --- //
const char* ssid = "OPPO A53";
const char* password = "611b10a883c5";
const char* mqtt_server = "10.74.94.63";

// --- OBJETOS --- //
WiFiClient espClient;
PubSubClient client(espClient);
WebServer server(80);

// --- VELOCIDAD --- //
float velocidadNominal = 50.00;
float velocidad = 0.0;

// --- CALLBACK MQTT --- //
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en: ");
  Serial.println(topic);

  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  velocidad = msg.toFloat();
  Serial.println(velocidad);
  Serial.print("Velocidad actualizada: ");
  Serial.println(velocidad);
}

// --- WIFI --- //
void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// --- MQTT --- //
void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT... ");

    if (client.connect("ESP32_MOTOR")) {
      Serial.println("Conectado");
      client.subscribe("esp32/hz");  
    } else {
      Serial.print("Error: ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// --- WEB PAGE --- //
String generarPagina() {
  String h = "<!DOCTYPE html><html><head>";
  h += "<meta charset='UTF-8'>";
  h += "<meta http-equiv='refresh' content='1'>"; 
  h += "<title>Control Motor</title>";
  h += "<style>button{width:120px;height:40px;margin:10px;font-size:18px;}</style>";
  h += "</head><body><h2>Control Motor</h2>";
  h += "<p>Velocidad actual: " + String(velocidad) + " Hz</p>";
  h += "<button onclick=\"location.href='/stop'\">STOP</button>";
  h += "<button onclick=\"location.href='/cw'\">CW</button>";
  h += "<button onclick=\"location.href='/ccw'\">CCW</button>";
  h += "</body></html>";
  return h;
}

void handleRoot() { server.send(200, "text/html", generarPagina()); }

void stopMotor() {
  estado = PARADA;
  server.sendHeader("Location", "/");
  server.send(303);
}

void cwMotor() {
  estado = GIRO_CW;
  server.sendHeader("Location", "/");
  server.send(303);
}

void ccwMotor() {
  estado = GIRO_CCW;
  server.sendHeader("Location", "/");
  server.send(303);
}

// --- ACCIONES MOTOR --- //
void parar() {
  digitalWrite(rele1, HIGH);
  digitalWrite(rele2, HIGH);
  digitalWrite(rele3, HIGH);
  digitalWrite(rele4, HIGH);
}

void iniciarArranque() {
  etapa = ETAPA_ESTRELLA;
  Serial.println("Estrella");
  digitalWrite(rele3, LOW);  
  digitalWrite(rele4, HIGH);
  actualizarArranque();
}

void actualizarArranque() {
  if (etapa == ETAPA_IDLE) return;
  if (etapa == ETAPA_TRIANGULO) return;
  if ((etapa == ETAPA_ESTRELLA) && (velocidad >= 40.00)) {
    etapa = ETAPA_TRIANGULO;
    digitalWrite(rele3, HIGH);
    digitalWrite(rele4, LOW);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(rele1, OUTPUT);
  pinMode(rele2, OUTPUT);
  pinMode(rele3, OUTPUT);
  pinMode(rele4, OUTPUT);

  parar();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  server.on("/", handleRoot);
  server.on("/stop", stopMotor);
  server.on("/cw", cwMotor);
  server.on("/ccw", ccwMotor);
  server.begin();
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  server.handleClient();
  actualizarArranque();

  if (estado == PARADA) {
    parar();
    etapa = ETAPA_IDLE;
  }

  else if (estado == GIRO_CW) {
    if (estadoAnterior != GIRO_CW) {
      parar();
      etapa = ETAPA_ESTRELLA;
      digitalWrite(rele1, LOW);
      digitalWrite(rele2, HIGH);
      iniciarArranque();
    }
  }

  else if (estado == GIRO_CCW) {
    if (estadoAnterior != GIRO_CCW) {
      parar();
      etapa = ETAPA_ESTRELLA;
      digitalWrite(rele1, HIGH);
      digitalWrite(rele2, LOW);
      iniciarArranque();
    }
  }

  estadoAnterior = estado;
}