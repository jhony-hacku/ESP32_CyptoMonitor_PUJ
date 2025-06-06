// === CÓDIGO ESP32 (Arduino) ===
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_JSON.h>

const char* ssid = "ESP";
const char* password = "BUZZ1%99";
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

const int ledRed = 32;
const int ledGreen = 25;
const int ledWhite = 33;
const int buttonRight = 19;
const int buttonLeft = 18;
const int buzzer = 26;

bool lastButtonRight = HIGH;
bool lastButtonLeft = HIGH;

float lastBTC = -1;
float lastSOL = -1;
float lastETH = -1;
float currentBTC = -1;
float currentSOL = -1;
float currentETH = -1;

String criptoBoton1 = "BTCUSDT";
String criptoBoton2 = "SOLUSDT";
float alertaBoton1 = -1;
float alertaBoton2 = -1;
int duracionBuzzer = 300;

String selectedAsset = "";

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

void manejarCambioDePrecio(float anterior, float actual) {
  if (anterior < 0 || actual < 0) return;

  if (actual > anterior) {
    digitalWrite(ledRed, LOW);
    digitalWrite(ledGreen, HIGH);
    digitalWrite(ledWhite, LOW);
  } else if (actual < anterior) {
    digitalWrite(ledRed, HIGH);
    digitalWrite(ledGreen, LOW);
    digitalWrite(ledWhite, LOW);
  } else {
    digitalWrite(ledRed, LOW);
    digitalWrite(ledGreen, LOW);
    digitalWrite(ledWhite, HIGH);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String msg = String((char*)payload);
  String topicStr = String(topic);

  if (topicStr == "config/monitor01") {
    JSONVar config = JSON.parse(msg);
    if (JSON.typeof(config) == "undefined") {
      Serial.println("Error al parsear JSON");
      return;
    }
    criptoBoton1 = (const char*)config["boton1"];
    criptoBoton2 = (const char*)config["boton2"];
    alertaBoton1 = (double)config["alerta1"];
    alertaBoton2 = (double)config["alerta2"];
    duracionBuzzer = (int)config["duracion"];
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
  } else if (topicStr == "crypto/ethereum") {
    if (currentETH >= 0) lastETH = currentETH;
    currentETH = valor;
    if (selectedAsset == "ETH" || selectedAsset == "ETHUSDT") {
      manejarCambioDePrecio(lastETH, currentETH);
      mostrarEnPantallaYSerial("ETH:", String(currentETH, 2));
      if (alertaBoton1 > 0 && currentETH >= alertaBoton1) tone(buzzer, 2000, duracionBuzzer);
    }
  }
}

void conectarMQTT() {
  while (!mqttClient.connected()) {
    mostrarEnPantallaYSerial("MQTT", "Conectando...");
    if (mqttClient.connect("ESP32_Crypto")) {
      mostrarEnPantallaYSerial("MQTT", "Conectado");
      mqttClient.subscribe("crypto/bitcoin");
      mqttClient.subscribe("crypto/solana");
      mqttClient.subscribe("crypto/ethereum");
      mqttClient.subscribe("config/monitor01");
    } else {
      mostrarEnPantallaYSerial("MQTT FAIL", "Estado: " + String(mqttClient.state()));
      delay(3000);
    }
  }
}

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

void loop() {
  if (!mqttClient.connected()) conectarMQTT();
  mqttClient.loop();

  bool btnRight = digitalRead(buttonRight);
  bool btnLeft = digitalRead(buttonLeft);

  if (lastButtonRight == HIGH && btnRight == LOW) {
    selectedAsset = criptoBoton1;
    float precio = -1;
    if (selectedAsset.startsWith("BTC")) precio = currentBTC;
    else if (selectedAsset.startsWith("SOL")) precio = currentSOL;
    else if (selectedAsset.startsWith("ETH")) precio = currentETH;

    mostrarEnPantallaYSerial(selectedAsset + ":", precio >= 0 ? String(precio, 2) : "Cargando");
    tone(buzzer, 1000, 150);
    if (precio >= 0) manejarCambioDePrecio(precio, precio);
  }

  if (lastButtonLeft == HIGH && btnLeft == LOW) {
    selectedAsset = criptoBoton2;
    float precio = -1;
    if (selectedAsset.startsWith("BTC")) precio = currentBTC;
    else if (selectedAsset.startsWith("SOL")) precio = currentSOL;
    else if (selectedAsset.startsWith("ETH")) precio = currentETH;

    mostrarEnPantallaYSerial(selectedAsset + ":", precio >= 0 ? String(precio, 2) : "Cargando");
    tone(buzzer, 1500, 150);
    if (precio >= 0) manejarCambioDePrecio(precio, precio);
  }

  lastButtonRight = btnRight;
  lastButtonLeft = btnLeft;
  delay(10);
}
