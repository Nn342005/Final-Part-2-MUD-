#include <WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==== CONFIGURATION ====
const char* ssid = "Linksys00771_5GHz_2.4GEXT";
const char* password = "589dqb8vgr";

const char* mqtt_server = "35.192.68.69";       
const char* udp_host = "35.192.68.69";         
const int udp_port = 12345;

#define SDA 14
#define SCL 13

WiFiUDP udp;
WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool gameOver = false;

// ==== FUNCTIONS ====
void setup_wifi() {
  delay(100);
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println(" connected!");
}

void callback(char* topic, byte* message, unsigned int length) {
  char msg[length + 1];
  for (unsigned int i = 0; i < length; i++) msg[i] = (char)message[i];
  msg[length] = '\0';

  Serial.print("MQTT: ");
  Serial.println(msg);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Room:");
  lcd.setCursor(0, 1);
  lcd.print(msg);

  if (String(msg).indexOf("Game over") != -1) {
    Serial.println("🎮 GAME OVER!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("🎉 GAME OVER!");
    lcd.setCursor(0, 1);
    lcd.print("Congrats!");
    gameOver = true;
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println(" connected");
      client.subscribe("mud/room");
    } else {
      Serial.print(" failed, rc=");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void sendMove(char move) {
  udp.beginPacket(udp_host, udp_port);
  udp.write(move);
  udp.endPacket();
  Serial.print("Sent move: ");
  Serial.println(move);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA, SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  udp.begin(4210); // Local port
  lcd.clear();
  lcd.print("Ready!");
  Serial.println("ESP32 Ready!");
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  if (gameOver) return; // Freeze input after win

  if (Serial.available() > 0) {
    char input = Serial.read();
    if (input == 'N' || input == 'S' || input == 'E' || input == 'W' ||
        input == 'n' || input == 's' || input == 'e' || input == 'w') {
      sendMove(toupper(input));
    }
  }
}
