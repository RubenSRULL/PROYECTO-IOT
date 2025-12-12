//AUTOR: Ruben Sahuquillo Redondo
//ASIGNATURA: Internet de las Cosas
//CÓDIGO uC CONTROL


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
void configWifi() {
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
  String h = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>Control Motor</title>
<style>
body { 
    font-family: Arial; 
    background:#f2f2f2; 
    margin:0; 
    padding:20px; 
}
h2 { 
    text-align:center; 
    margin-bottom:20px; 
}
.panel { 
    background:white; 
    padding:20px; 
    border-radius:10px;
    box-shadow:0 2px 6px rgba(0,0,0,0.15); 
    max-width:400px; 
    margin:auto; 
}
.estado { 
    font-size:20px; 
    margin-bottom:15px; 
    font-weight:bold; 
}
button { 
    width:120px; 
    height:45px; 
    margin:10px; 
    font-size:18px;
    border:none; 
    border-radius:6px; 
    cursor:pointer; 
}
.stop { background:#c0392b; color:white; }
.cw   { background:#27ae60; color:white; }
.ccw  { background:#2980b9; color:white; }
</style>
</head>
<body>
<h2>Panel de Control del Motor</h2>
<div class="panel">
    <div class="estado">Velocidad: <span id="vel">%VEL%</span> Hz</div>
    <div class="estado">Estado: <span id="estado">%ESTADO%</span></div>
    <div style="text-align:center;">
        <button class="stop" onclick="accion('stop')">STOP</button><br>
        <button class="cw" onclick="accion('cw')">CW</button>
        <button class="ccw" onclick="accion('ccw')">CCW</button>
    </div>
</div>
<script>
function accion(cmd) {
    fetch('/' + cmd);
}
setInterval(() => {
    fetch('/datos')
    .then(r => r.json())
    .then(d => {
        document.getElementById("vel").innerText = d.velocidad;
        document.getElementById("estado").innerText = d.estado;
    });
}, 1000);
</script>
</body>
</html>
)rawliteral";
  h.replace("%VEL%", String(velocidad, 2));
  if (estado == PARADA){
    h.replace("%ESTADO%", "PARADO");
  }
  else if (estado == GIRO_CW{
    h.replace("%ESTADO%", "GIRO CW");
  }
  else if (estado == GIRO_CCW){
    h.replace("%ESTADO%", "GIRO CCW");
  }
  return h;
}

// --- MANTENER HTML --- //
void handleRoot() {
  server.send(200, "text/html", generarPagina());
}

// --- FUNCION ENVIAR DATOS DE ESTADO Y VELOCIDAD A SERVIDOR WEB MEDIANTE FICHERO .json --- //
void enviarDatos() {
  String json = "{";
  json += "\"velocidad\":\"" + String(velocidad, 2) + "\",";
  json += "\"estado\":\"";
  if (estado == PARADA) json += "PARADO";
  else if (estado == GIRO_CW) json += "GIRO CW";
  else if (estado == GIRO_CCW) json += "GIRO CCW";
  json += "\"}";
  server.send(200, "application/json", json);
}

// --- FUNCION PARA DETENER MOTOR --- //
void stopMotor() {
  estado = PARADA;
  server.sendHeader("Location", "/");
  server.send(303);
}

// --- FUNCION PARA GIRAR EN SENTIDO ANTIHORARIO --- //
void cwMotor() {
  estado = GIRO_CW;
  server.sendHeader("Location", "/");
  server.send(303);
}

// --- FUNCION PARA GIRAR EN SENTIDO HORARIO --- //
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

// --- FUNCION PARA INICIAR ARRANQUE EN ESTRELLA --- //
void iniciarArranque() {
  etapa = ETAPA_ESTRELLA;
  Serial.println("Estrella");
  digitalWrite(rele3, LOW);  
  digitalWrite(rele4, HIGH);
  actualizarArranque();
}

// --- FUNCION PARA ACTUALIZAR ARRANQUE Y CONMUTAR A TRIÁNGULO --- //
void actualizarArranque() {
  if (etapa == ETAPA_IDLE) return;
  if (etapa == ETAPA_TRIANGULO) return;
  if ((etapa == ETAPA_ESTRELLA) && (velocidad >= 40.00)) {
    etapa = ETAPA_TRIANGULO;
    digitalWrite(rele3, HIGH);
    digitalWrite(rele4, LOW);
  }
}

// --- SETUP --- //
void setup() {
  Serial.begin(115200);
  pinMode(rele1, OUTPUT);
  pinMode(rele2, OUTPUT);
  pinMode(rele3, OUTPUT);
  pinMode(rele4, OUTPUT);
  parar();
  configWifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  server.on("/", handleRoot);
  server.on("/stop", stopMotor);
  server.on("/cw", cwMotor);
  server.on("/ccw", ccwMotor);
  server.on("/datos", enviarDatos);
  server.begin();
}

// --- LOOP --- //
void loop() {
  if (!client.connected()){
    reconnect();
  }
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