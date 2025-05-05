#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// WiFi
const char* ssid     = "ESP";
const char* password = "BUZZ1%99";

// MQTT Broker
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;

// Pines
const int led1 = 2;   // Controlado con "1"
const int led2 = 15;  // Controlado con "2"
const int led3 = 4;   // Controlado con "3"
const int buzzerPin = 5;
const int button18 = 18;
const int button19 = 19;

// Estados de LEDs
bool estadoLed1 = false;
bool estadoLed2 = false;
bool estadoLed3 = false;

// Estado anterior de botones
bool lastButton18 = HIGH;
bool lastButton19 = HIGH;

char mensajeRespuesta[50];
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

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String mensaje = String((char*)payload);

  Serial.print("📥 Mensaje recibido [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(mensaje);

  if (mensaje == "1") {
    estadoLed1 = !estadoLed1;
    digitalWrite(led1, estadoLed1);
    sprintf(mensajeRespuesta, "LED 1 en pin %d %s", led1, estadoLed1 ? "encendido" : "apagado");
  } else if (mensaje == "2") {
    estadoLed2 = !estadoLed2;
    digitalWrite(led2, estadoLed2);
    sprintf(mensajeRespuesta, "LED 2 en pin %d %s", led2, estadoLed2 ? "encendido" : "apagado");
  } else if (mensaje == "3") {
    estadoLed3 = !estadoLed3;
    digitalWrite(led3, estadoLed3);
    sprintf(mensajeRespuesta, "LED 3 en pin %d %s", led3, estadoLed3 ? "encendido" : "apagado");
  } else if (mensaje == "buzz") {
    tone(buzzerPin, 1000, 500);  // 1000 Hz, 500 ms
    strcpy(mensajeRespuesta, "🔊 Buzzer activado");
  } else {
    strcpy(mensajeRespuesta, "Comando no reconocido");
  }

  mqttClient.publish("ESP/Crypto", mensajeRespuesta);
  Serial.print("📤 Respuesta publicada: ");
  Serial.println(mensajeRespuesta);
}

void conectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Intentando conectar a MQTT...");
    if (mqttClient.connect("ESP32Client_Crypto")) {
      Serial.println("✅ Conectado");
      mqttClient.subscribe("ESPS/Crypto");
    } else {
      Serial.print("❌ Error: ");
      Serial.print(mqttClient.state());
      Serial.println(" - Reintentando en 3s");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Configurar pines
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(button18, INPUT_PULLUP);
  pinMode(button19, INPUT_PULLUP);

  // Inicializar LEDs apagados
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);

  wifiInit();
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);
}

void loop() {
  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();

  // Leer botones
  bool currentButton18 = digitalRead(button18);
  bool currentButton19 = digitalRead(button19);

  if (lastButton18 == HIGH && currentButton18 == LOW) {
    mqttClient.publish("ESP/Crypto", "Botón 18 presionado");
    Serial.println("🔘 Botón 18 presionado");
  }

  if (lastButton19 == HIGH && currentButton19 == LOW) {
    mqttClient.publish("ESP/Crypto", "Botón 19 presionado");
    Serial.println("🔘 Botón 19 presionado");
  }

  lastButton18 = currentButton18;
  lastButton19 = currentButton19;

  // Mensaje periódico opcional
  if (millis() - lastMsg > 10000) {
    lastMsg = millis();
    Serial.println("📡 Esperando comandos en ESPS/Crypto...");
  }

  delay(10);
}
