#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Configura tu red WiFi
const char* ssid     = "ESP";
const char* password = "BUZZ1%99";

// Configura el broker MQTT
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;

const int ledPin = 2;
int var = 0;
char mensajeRespuesta[40];
unsigned long lastMsg = 0;

void wifiInit() {
  Serial.print("Conectándose a WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi conectado");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

// Callback al recibir mensaje MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String mensaje = String((char*)payload);

  Serial.print("📥 Mensaje recibido [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(mensaje);

  if (mensaje == "1") {
    digitalWrite(ledPin, HIGH);
    strcpy(mensajeRespuesta, "El LED se encendió");
  } else if (mensaje == "0") {
    digitalWrite(ledPin, LOW);
    strcpy(mensajeRespuesta, "El LED se apagó");
  } else {
    strcpy(mensajeRespuesta, "Comando inválido");
  }

  mqttClient.publish("ESP/Crypto", mensajeRespuesta);
  Serial.print("📤 Respuesta publicada: ");
  Serial.println(mensajeRespuesta);
}

// Conexión al broker MQTT
void conectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Intentando conectar a MQTT...");
    if (mqttClient.connect("ESP32Client_Crypto")) {
      Serial.println("✅ Conectado");
      mqttClient.subscribe("ESPS/Crypto");  // Escuchar comandos
    } else {
      Serial.print("❌ Error: ");
      Serial.print(mqttClient.state());
      Serial.println(" - Reintentando en 3s");
      delay(3000);
    }
  }
}

void setup() {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  wifiInit();

  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);
}

void loop() {
  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();

  // Información periódica (opcional)
  unsigned long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
    Serial.println("📡 Esperando comandos en ESPS/Crypto...");
  }
}
