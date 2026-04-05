// ============================================================
// ESP32 IoT Dashboard - Main Firmware
// ============================================================
// Ket noi WiFi nha -> MQTT broker -> Doc cam bien + Dong ho dien
// Dieu khien Relay, LED tu dien thoai qua MQTT
// ============================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <ModbusMaster.h>
#include "config.h"

// ===== OBJECTS =====
WiFiClientSecure espClient;
PubSubClient mqtt(espClient);
DHT dht(DHT_PIN, DHT_TYPE);
ModbusMaster modbus;

// ===== STATE VARIABLES =====
bool relayState = false;
bool ledState = false;

// Sensor data
float temperature = 0;
float humidity = 0;
int lightLevel = 0;

// Meter data (Modbus)
float meterVoltage = 0;
float meterCurrent = 0;
float meterPower = 0;
float meterEnergy = 0;
float meterFrequency = 0;
float meterPF = 0;

// Timers
unsigned long lastSensorRead = 0;
unsigned long lastMeterRead = 0;
unsigned long lastMqttPub = 0;
unsigned long lastReconnect = 0;

// ===== RS485 DIRECTION CONTROL =====
void preTransmission() {
  digitalWrite(RS485_DE_RE_PIN, HIGH);  // Enable transmit
}

void postTransmission() {
  digitalWrite(RS485_DE_RE_PIN, LOW);   // Enable receive
}

// ===== READ FLOAT FROM MODBUS (2 registers = 32-bit float) =====
float readModbusFloat(uint16_t reg) {
  uint8_t result = modbus.readInputRegisters(reg, 2);
  if (result == modbus.ku8MBSuccess) {
    // Combine 2 x 16-bit registers into 1 x 32-bit float (big-endian)
    uint32_t raw = ((uint32_t)modbus.getResponseBuffer(0) << 16) | modbus.getResponseBuffer(1);
    float value;
    memcpy(&value, &raw, sizeof(value));
    return value;
  }
  return -1;  // Error
}

// ===== SETUP WIFI =====
void setupWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi Connection Failed! Restarting...");
    ESP.restart();
  }
}

// ===== MQTT CALLBACK (nhan lenh tu dien thoai) =====
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Parse JSON command
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  
  if (error) {
    Serial.println("Failed to parse MQTT command");
    return;
  }

  const char* device = doc["device"];
  const char* action = doc["action"];

  Serial.print("Command received - Device: ");
  Serial.print(device);
  Serial.print(", Action: ");
  Serial.println(action);

  // Xu ly lenh dieu khien
  if (strcmp(device, "relay") == 0) {
    if (strcmp(action, "toggle") == 0) {
      relayState = !relayState;
    } else if (strcmp(action, "on") == 0) {
      relayState = true;
    } else if (strcmp(action, "off") == 0) {
      relayState = false;
    }
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
    Serial.print("Relay is now: ");
    Serial.println(relayState ? "ON" : "OFF");
  }
  else if (strcmp(device, "led") == 0) {
    if (strcmp(action, "toggle") == 0) {
      ledState = !ledState;
    } else if (strcmp(action, "on") == 0) {
      ledState = true;
    } else if (strcmp(action, "off") == 0) {
      ledState = false;
    }
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    Serial.print("LED is now: ");
    Serial.println(ledState ? "ON" : "OFF");
  }

  // Gui trang thai moi ngay lap tuc sau khi dieu khien
  publishStatus();
}

// ===== CONNECT / RECONNECT MQTT =====
void connectMQTT() {
  if (mqtt.connected()) return;
  
  // Khong reconnect qua nhanh
  if (millis() - lastReconnect < 5000) return;
  lastReconnect = millis();

  Serial.print("Connecting to MQTT broker...");

  if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
    Serial.println(" Connected!");
    
    // Subscribe topic dieu khien
    mqtt.subscribe(TOPIC_CONTROL);
    Serial.println("Subscribed to: " TOPIC_CONTROL);
    
    // Gui trang thai hien tai
    publishStatus();
  } else {
    Serial.print(" Failed, rc=");
    Serial.println(mqtt.state());
  }
}

// ===== PUBLISH SENSOR DATA =====
void publishSensors() {
  StaticJsonDocument<256> doc;
  doc["temp"] = round(temperature * 10) / 10.0;
  doc["hum"] = round(humidity * 10) / 10.0;
  doc["light"] = lightLevel;

  char buffer[256];
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_SENSORS, buffer);
}

// ===== PUBLISH METER DATA =====
void publishMeter() {
  StaticJsonDocument<256> doc;
  doc["voltage"] = round(meterVoltage * 10) / 10.0;
  doc["current"] = round(meterCurrent * 100) / 100.0;
  doc["power"] = round(meterPower * 10) / 10.0;
  doc["energy"] = round(meterEnergy * 100) / 100.0;
  doc["freq"] = round(meterFrequency * 10) / 10.0;
  doc["pf"] = round(meterPF * 100) / 100.0;

  char buffer[256];
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_METER, buffer);
}

// ===== PUBLISH DEVICE STATUS =====
void publishStatus() {
  StaticJsonDocument<128> doc;
  doc["relay"] = relayState;
  doc["led"] = ledState;

  char buffer[128];
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_STATUS, buffer, true); // retained message
}

// ===== PUBLISH WIFI STATUS =====
void publishWifiStatus() {
  StaticJsonDocument<128> doc;
  doc["connected"] = (WiFi.status() == WL_CONNECTED);
  doc["ssid"] = WiFi.SSID();
  doc["rssi"] = WiFi.RSSI();

  char buffer[128];
  serializeJson(doc, buffer);
  mqtt.publish(TOPIC_WIFI, buffer);
}

// ===== READ SENSORS =====
void readSensors() {
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  lightLevel = analogRead(LDR_PIN);

  // Kiem tra nan
  if (isnan(temperature)) temperature = 0;
  if (isnan(humidity)) humidity = 0;

  Serial.printf("Temp: %.1f°C | Hum: %.1f%% | Light: %d\n", temperature, humidity, lightLevel);
}

// ===== READ POWER METER VIA MODBUS RTU =====
void readMeter() {
  meterVoltage = readModbusFloat(REG_VOLTAGE);
  delay(50);
  meterCurrent = readModbusFloat(REG_CURRENT);
  delay(50);
  meterPower = readModbusFloat(REG_POWER);
  delay(50);
  meterEnergy = readModbusFloat(REG_ENERGY);
  delay(50);
  meterFrequency = readModbusFloat(REG_FREQUENCY);
  delay(50);
  meterPF = readModbusFloat(REG_PF);

  Serial.printf("Meter: %.1fV | %.2fA | %.1fW | %.2fkWh | %.1fHz | PF:%.2f\n",
    meterVoltage, meterCurrent, meterPower, meterEnergy, meterFrequency, meterPF);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 IoT Dashboard ===");

  // GPIO Setup
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RS485_DE_RE_PIN, OUTPUT);
  
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(RS485_DE_RE_PIN, LOW);

  // DHT Sensor
  dht.begin();

  // Modbus RTU via RS485
  Serial2.begin(MODBUS_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  modbus.begin(MODBUS_SLAVE_ID, Serial2);
  modbus.preTransmission(preTransmission);
  modbus.postTransmission(postTransmission);

  // WiFi
  setupWiFi();

  // MQTT
  espClient.setInsecure(); // Skip cert verification for simplicity
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setBufferSize(512);

  Serial.println("Setup complete! Starting main loop...\n");
}

// ===== MAIN LOOP =====
void loop() {
  // Reconnect WiFi neu mat ket noi
  if (WiFi.status() != WL_CONNECTED) {
    setupWiFi();
  }

  // Reconnect MQTT
  if (!mqtt.connected()) {
    connectMQTT();
  }
  mqtt.loop();

  unsigned long now = millis();

  // Doc cam bien theo chu ky
  if (now - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = now;
    readSensors();
  }

  // Doc dong ho dien theo chu ky
  if (now - lastMeterRead >= METER_INTERVAL) {
    lastMeterRead = now;
    readMeter();
  }

  // Gui du lieu len MQTT theo chu ky
  if (now - lastMqttPub >= MQTT_PUB_INTERVAL) {
    lastMqttPub = now;
    if (mqtt.connected()) {
      publishSensors();
      publishMeter();
      publishWifiStatus();
    }
  }
}
