#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_JSON.h>

// WiFi y MQTT
const char* ssid = "ESP";
const char* password = "BUZZ1%99";
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// OLED
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// Pines
const int ledRed = 4;
const int ledGreen = 15;
const int ledWhite = 2;
const int buttonRight = 19;
const int buttonLeft = 18;
const int buzzer = 5;

// Estados de botones
bool lastButtonRight = HIGH;
bool lastButtonLeft = HIGH;

// Variables de precio
float lastBTC = -1;
float lastSOL = -1;
float currentBTC = -1;
float currentSOL = -1;

// Configuración dinámica
String criptoBoton1 = "BTCUSDT";
String criptoBoton2 = "SOLUSDT";
float alertaBoton1 = -1;
float alertaBoton2 = -1;
int duracionBuzzer = 300;

String selectedAsset = "";

// Mostrar OLED + Serial
void mostrarEnPantallaYSerial(const String& msg1, const String& msg2 = "") {
  Serial.println(msg1);
  if (msg2 != "") Serial.println(msg2);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.println(msg1);
  if (msg2 != "") display.println(msg2);
  display.display();
}

// Inicializar WiFi
void wifiInit() {
  mostrarEnPantallaYSerial("Conectando WiFi", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  String ip = WiFi.localIP().toString();
  mostrarEnPantallaYSerial("WiFi OK", "IP: " + ip);
}

// LEDs según cambio de precio
void manejarCambioDePrecio(float anterior, float actual) {
  if (anterior < 0 || actual < 0) return;

  if (actual > anterior) {
    digitalWrite(ledRed, LOW);
    digitalWrite(ledGreen, LOW);
    digitalWrite(ledWhite, HIGH);
  } else if (actual < anterior) {
    digitalWrite(ledRed, LOW);
    digitalWrite(ledGreen, HIGH);
    digitalWrite(ledWhite, LOW);
  } else {
    digitalWrite(ledRed, HIGH);
    digitalWrite(ledGreen, LOW);
    digitalWrite(ledWhite, LOW);
  }
}

// MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String msg = String((char*)payload);
  String topicStr = String(topic);

  if (topicStr == "config/monitor01") {
    JSONVar config = JSON.parse(msg);
    if (JSON.typeof(config) == "undefined") {
      Serial.println("❌ Error al parsear JSON");
      return;
    }

    criptoBoton1 = (const char*)config["boton1"];
    criptoBoton2 = (const char*)config["boton2"];
    alertaBoton1 = (double)config["alerta1"];
    alertaBoton2 = (double)config["alerta2"];
    duracionBuzzer = (int)config["duracion"];

    Serial.println("✅ Configuración actualizada:");
    Serial.println("Botón 1: " + criptoBoton1 + " Alerta: " + String(alertaBoton1));
    Serial.println("Botón 2: " + criptoBoton2 + " Alerta: " + String(alertaBoton2));
    Serial.println("Duración buzzer: " + String(duracionBuzzer));
    return;
  }

  float valor = msg.toFloat();

  if (topicStr == "crypto/bitcoin") {
    if (currentBTC >= 0) lastBTC = currentBTC;
    currentBTC = valor;
    if (selectedAsset == "BTC" || selectedAsset == "BTCUSDT") {
      manejarCambioDePrecio(lastBTC, currentBTC);
      mostrarEnPantallaYSerial("BTC:", String(currentBTC, 2));
      if (alertaBoton1 > 0 && currentBTC >= alertaBoton1) tone(buzzer, 2000, duracionBuzzer);
    }
  } else if (topicStr == "crypto/solana") {
    if (currentSOL >= 0) lastSOL = currentSOL;
    currentSOL = valor;
    if (selectedAsset == "SOL" || selectedAsset == "SOLUSDT") {
      manejarCambioDePrecio(lastSOL, currentSOL);
      mostrarEnPantallaYSerial("SOL:", String(currentSOL, 2));
      if (alertaBoton2 > 0 && currentSOL >= alertaBoton2) tone(buzzer, 2000, duracionBuzzer);
    }
  }
}

// Conectar MQTT
void conectarMQTT() {
  while (!mqttClient.connected()) {
    mostrarEnPantallaYSerial("MQTT", "Conectando...");
    if (mqttClient.connect("ESP32_Crypto")) {
      mostrarEnPantallaYSerial("MQTT", "Conectado");
      mqttClient.subscribe("crypto/bitcoin");
      mqttClient.subscribe("crypto/solana");
      mqttClient.subscribe("config/monitor01");
    } else {
      mostrarEnPantallaYSerial("MQTT FAIL", "Estado: " + String(mqttClient.state()));
      delay(3000);
    }
  }
}

// Setup
void setup() {
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    mostrarEnPantallaYSerial("Error OLED");
    while (true);
  }

  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledWhite, OUTPUT);
  pinMode(buttonRight, INPUT_PULLUP);
  pinMode(buttonLeft, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);

  digitalWrite(ledRed, LOW);
  digitalWrite(ledGreen, LOW);
  digitalWrite(ledWhite, LOW);

  wifiInit();
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(callback);
}

// Loop principal
void loop() {
  if (!mqttClient.connected()) conectarMQTT();
  mqttClient.loop();

  bool btnRight = digitalRead(buttonRight);
  bool btnLeft = digitalRead(buttonLeft);

  if (lastButtonRight == HIGH && btnRight == LOW) {
    selectedAsset = criptoBoton1.startsWith("BTC") ? "BTC" : criptoBoton1;
    mostrarEnPantallaYSerial(selectedAsset + ":", currentBTC >= 0 ? String(currentBTC, 2) : "Cargando");
    tone(buzzer, 1000, 150);
    if (lastBTC >= 0 && currentBTC >= 0) manejarCambioDePrecio(lastBTC, currentBTC);
  }

  if (lastButtonLeft == HIGH && btnLeft == LOW) {
    selectedAsset = criptoBoton2.startsWith("SOL") ? "SOL" : criptoBoton2;
    mostrarEnPantallaYSerial(selectedAsset + ":", currentSOL >= 0 ? String(currentSOL, 2) : "Cargando");
    tone(buzzer, 1500, 150);
    if (lastSOL >= 0 && currentSOL >= 0) manejarCambioDePrecio(lastSOL, currentSOL);
  }

  lastButtonRight = btnRight;
  lastButtonLeft = btnLeft;
  delay(10);
}
