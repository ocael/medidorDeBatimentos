#include <Wire.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define PULSE_PIN 35
#define LED_PIN 2
#define BUZZER_PIN 25

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "test.mosquitto.org";

WiFiClient espClient;
PubSubClient client(espClient);

int minHeartRate = 50;
int maxHeartRate = 120;

void setupWiFi() {
  delay(1000);
  Serial.print("Conectando-se à ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nConectado ao Wi-Fi");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conectar MQTT...");
    if (client.connect("ESP32_HeartRate")) {
      Serial.println("Conectado ao MQTT");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha na inicialização do SSD1306"));
    while(1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,30);
  display.println("Monitor de Batimentos");
  display.display();
  delay(2000);

  setupWiFi();
  client.setServer(mqtt_server, 1883);
}

void emitirSom() {
  // 3 bipes curtos audíveis
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 1000); // 1000 Hz = som agudo audível
    delay(200);
    noTone(BUZZER_PIN);
    delay(200);
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Simulação de leitura do sensor
  int16_t pulseValue = analogRead(PULSE_PIN);
  int heartRate = map(pulseValue, 300, 600, 40, 180);
  heartRate = constrain(heartRate, 0, 200);

  String status;

  if (heartRate < minHeartRate) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Batimentos Baixos");
    display.setCursor(0,20);
    display.print(String(heartRate) + " bpm");
    display.display();

    status = "abaixo";
    digitalWrite(LED_PIN, HIGH);
    emitirSom();
  }
  else if (heartRate > maxHeartRate) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Batimentos Altos");
    display.setCursor(0,20);
    display.print(String(heartRate) + " bpm");
    display.display();

    status = "acima";
    digitalWrite(LED_PIN, HIGH);
    emitirSom();
  }
  else {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Batimentos Normais");
    display.setCursor(0,20);
    display.print(String(heartRate) + " bpm");
    display.display();

    status = "normal";
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN); // garante que pare o som
  }

  Serial.println("Batimento: " + String(heartRate) + " bpm");
  Serial.println("Situação: " + status);

  // Publicar no MQTT
  char bpmMsg[10];
  char statusMsg[20];
  sprintf(bpmMsg, "%d", heartRate);
  status.toCharArray(statusMsg, 20);
  client.publish("coracao/batimento", bpmMsg);
  client.publish("coracao/situacao", statusMsg);

  delay(2000);
}