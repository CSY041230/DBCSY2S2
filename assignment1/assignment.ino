#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ================= WIFI =================
const char* ssid = "AJ@Student";
const char* password = "Student2026";

// ================= MQTT =================
const char* mqtt_server = "w02dd116.ala.asia-southeast1.emqxsl.com";
const int mqtt_port = 8883;

const char* mqtt_user = "test";
const char* mqtt_password = "123";

// ================= GPIO =================
#define TRIG_PIN 18
#define ECHO_PIN 19
#define IR_PIN 5
#define LED_PIN 23

WiFiClientSecure espClient;
PubSubClient client(espClient);

// =======================================
// WiFi Connection
// =======================================
void setup_wifi() {
  Serial.print("Connecting to WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// =======================================
// MQTT Callback
// =======================================
void callback(char* topic, byte* payload, unsigned int length) {

  String message = "";

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message received: ");
  Serial.println(message);

  if (String(topic) == "home/command") {

    if (message == "ON") {
      digitalWrite(LED_PIN, HIGH);
    }

    if (message == "OFF") {
      digitalWrite(LED_PIN, LOW);
    }
  }
}

// =======================================
// MQTT Reconnect
// =======================================
void reconnect() {

  while (!client.connected()) {

    Serial.print("Connecting to MQTT...");

    if (client.connect(
          "ESP32_001",
          mqtt_user,
          mqtt_password
        )) {

      Serial.println("Connected");

      client.subscribe("home/command");

    } else {

      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" retry in 5 seconds");

      delay(5000);
    }
  }
}

// =======================================
// Read Ultrasonic Distance
// =======================================
float readDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return -1;
  }

  return duration * 0.0343 / 2.0;
}

// =======================================
// Setup
// =======================================
void setup() {

  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(IR_PIN, INPUT);

  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);

  setup_wifi();

  espClient.setInsecure();

  client.setServer(mqtt_server, mqtt_port);

  client.setCallback(callback);
}

// =======================================
// Loop
// =======================================
void loop() {

  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  float distance = readDistance();

  if (distance == -1) {
    Serial.println("Ultrasonic timeout");
    delay(1000);
    return;
  }

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Only activate monitoring zone below 300 cm
  if (distance < 30) {

    bool irDetected = (digitalRead(IR_PIN) == LOW);
    bool ledState = false;

    if (irDetected) {
      digitalWrite(LED_PIN, HIGH);
      ledState = true;
      Serial.println("IR DETECTED OBJECT");
    }
    else {
      digitalWrite(LED_PIN, LOW);
      Serial.println("IR NO OBJECT");
    }

    String payload = "{";
    payload += "\"deviceId\":\"ESP32_001\",";
    payload += "\"distance\":" + String(distance, 2) + ",";
    payload += "\"irDetected\":" + String(irDetected ? "true" : "false") + ",";
    payload += "\"ledState\":" + String(ledState ? "true" : "false");
    payload += "}";

    Serial.println("Publishing:");
    Serial.println(payload);

    client.publish("home/telemetry", payload.c_str());
  }
  else {
    digitalWrite(LED_PIN, LOW);
    Serial.println("Outside monitoring zone");
  }

  delay(2000);
}