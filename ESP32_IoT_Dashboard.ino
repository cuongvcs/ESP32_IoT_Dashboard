// ============================================================
// ESP32 IoT Dashboard - Main Firmware
// ============================================================
// Ket noi WiFi nha -> MQTT broker -> Doc cam bien + Dong ho dien
// Dieu khien Relay, LED tu dien thoai qua MQTT
// ============================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <ModbusMaster.h>
#include <HTTPClient.h>
#include "config.h"

// ===== OBJECTS =====
WiFiClientSecure espClient;
PubSubClient mqtt(espClient);
DHT dht(DHT_PIN, DHT_TYPE);
ModbusMaster modbus;

// ===== DYNAMIC OUTPUT MANAGER =====
#define NUM_OUTPUTS 6
struct OutputPin {
  String id;
  int pin;
  bool state;
  bool enabled;
};
// Default mapping based on Web Dashboard defaults
OutputPin outputs[NUM_OUTPUTS] = {
  {"relay", RELAY_PIN, false, true},
  {"led", LED_PIN, false, true},
  {"out3", 14, false, false},
  {"out4", 12, false, false},
  {"out5", 13, false, false},
  {"out6", 27, false, false}
};

// Function declarations
void updateOutputPins();
void sendTelegram(String message);

// ===== TELEGRAM CONFIG & FAULT FLAGS =====
bool teleEnabled = false;
String teleToken = "";
String teleChatId = "";
bool sensorFault = false;
bool lightFault = false;
bool meterFault = false;
unsigned long lastTeleAlert = 0;
String lastTeleMessage = "";

// Sensor data
float temperature = 0;
float humidity = 0;
int lightLevel = 0;

// ===== MODBUS DYNAMIC REGISTERS =====
#define NUM_MODBUS_PARAMS 6
struct ModbusParam {
  String id;
  uint16_t reg;
  String type;
};
ModbusParam modbusMap[NUM_MODBUS_PARAMS] = {
  {"voltage", 0, "float32"},
  {"current", 6, "float32"},
  {"power", 12, "float32"},
  {"energy", 342, "float32"},
  {"freq", 70, "float32"},
  {"pf", 30, "float32"}
};

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

// ===== MULTI-WIFI PROFILER (PREFERENCES) =====
Preferences preferences;
#define MAX_WIFI_PROFILES 5

struct WiFiProfile {
  String ssid;
  String pass;
};
WiFiProfile wifiProfiles[MAX_WIFI_PROFILES];

void loadWiFiProfiles() {
  preferences.begin("wifi_config", false);
  for (int i = 0; i < MAX_WIFI_PROFILES; i++) {
    wifiProfiles[i].ssid = preferences.getString(("ssid_" + String(i)).c_str(), "");
    wifiProfiles[i].pass = preferences.getString(("pass_" + String(i)).c_str(), "");
  }
  preferences.end();
}

void saveWiFiProfile(String new_ssid, String new_pass) {
  if (new_ssid == "") return;
  loadWiFiProfiles();
  
  // Kiem tra xem da co chua
  for (int i = 0; i < MAX_WIFI_PROFILES; i++) {
    if (wifiProfiles[i].ssid == new_ssid) {
      if (wifiProfiles[i].pass != new_pass) {
        preferences.begin("wifi_config", false);
        preferences.putString(("pass_" + String(i)).c_str(), new_pass);
        preferences.end();
      }
      return; // Đã lưu
    }
  }
  
  // Chua co -> Dich array va the vao vi tri [0]
  preferences.begin("wifi_config", false);
  for (int i = MAX_WIFI_PROFILES - 1; i > 0; i--) {
    wifiProfiles[i] = wifiProfiles[i-1];
    preferences.putString(("ssid_" + String(i)).c_str(), wifiProfiles[i].ssid);
    preferences.putString(("pass_" + String(i)).c_str(), wifiProfiles[i].pass);
  }
  wifiProfiles[0].ssid = new_ssid;
  wifiProfiles[0].pass = new_pass;
  preferences.putString("ssid_0", new_ssid);
  preferences.putString("pass_0", new_pass);
  preferences.end();
  Serial.println("Saved new WiFi profile: " + new_ssid);
}

void postTransmission() {
  digitalWrite(RS485_DE_RE_PIN, LOW);   // Enable receive
}

// ===== READ DYNAMIC MODBUS REGISTER =====
float readDynamicModbus(uint16_t reg, String type) {
  if (type == "int16") {
    if (modbus.readInputRegisters(reg, 1) == modbus.ku8MBSuccess) {
      int16_t val = (int16_t)modbus.getResponseBuffer(0);
      return (float)val;
    }
  } 
  else if (type == "int32") {
    if (modbus.readInputRegisters(reg, 2) == modbus.ku8MBSuccess) {
      // Big-endian default (AB CD)
      int32_t val = ((int32_t)modbus.getResponseBuffer(0) << 16) | modbus.getResponseBuffer(1);
      return (float)val;
    }
  }
  else { // "float32"
    if (modbus.readInputRegisters(reg, 2) == modbus.ku8MBSuccess) {
      uint32_t raw = ((uint32_t)modbus.getResponseBuffer(0) << 16) | modbus.getResponseBuffer(1);
      float value;
      memcpy(&value, &raw, sizeof(value));
      return value;
    }
  }
  return -1.0;  // Error indicator
}

// ===== SETUP WIFI =====
void setupWiFi() {
  Serial.println("\nConnecting to WiFi / Scanning known networks...");

  // 1. Load danh sach WiFi da tung vao
  loadWiFiProfiles();
  WiFi.mode(WIFI_STA);
  bool connected = false;

  // 2. Scan xem xung quanh co mang nao quen khong
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    String scanned_ssid = WiFi.SSID(i);
    for (int j = 0; j < MAX_WIFI_PROFILES; j++) {
      if (wifiProfiles[j].ssid != "" && wifiProfiles[j].ssid == scanned_ssid) {
        Serial.println("Found known network: " + scanned_ssid + ". Connecting...");
        WiFi.begin(wifiProfiles[j].ssid.c_str(), wifiProfiles[j].pass.c_str());
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
          delay(500); Serial.print("."); attempts++;
        }
        if (WiFi.status() == WL_CONNECTED) {
          connected = true;
          break; // Thoat vong switch profiles
        }
      }
    }
    if (connected) break; // Thoat vong scan networks
  }

  // 3. Neu khong co mang wifi nao quen, khoi dong Captive Portal
  if (!connected) {
    Serial.println("\nNo known networks nearby. Starting Captive Portal (ESP32_Setup)...");
    WiFiManager wm;
    wm.setClass("invert");

    // CODE DE TEST: Bo comment dong duoi de xoá sách nhớ, BẮT BUỘC phát WiFi_Setup
    wm.resetSettings(); 
    
    bool res = wm.autoConnect("ESP32_Setup", "12345678"); 

    if (!res) {
      Serial.println("Failed to connect. Restarting...");
      delay(1000);
      ESP.restart();
    }
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // 4. Luu profile nay vao bo nho Multi-WiFi
  saveWiFiProfile(WiFi.SSID(), WiFi.psk());
}// ===== NATIVE TELEGRAM BOT ALERTS =====
void sendTelegram(String message) {
  if (!teleEnabled || teleToken == "" || teleChatId == "") return;

  // Prevent spamming (only 1 message every 3 minutes unless it's an OK message, 
  // Nhung cho phep gui cac tin nhan khac nhau)
  unsigned long now = millis();
  if (message.indexOf("ALARM") != -1) {
     if (message == lastTeleMessage && (now - lastTeleAlert < 180000)) {
        if (lastTeleAlert != 0) return; // Spam prevention for the SAME issue
     }
     lastTeleMessage = message;
     lastTeleAlert = now;
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + teleToken + "/sendMessage?chat_id=" + teleChatId + "&text=" + message;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("Telegram sent successfully: " + message);
    } else {
      Serial.println("Telegram send error: " + String(httpCode));
    }
    http.end();
  }
}

// ===== MQTT CALLBACK (nhan lenh tu dien thoai) =====
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  if (strcmp(topic, TOPIC_CONTROL) == 0) {
    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
      Serial.println("Failed to parse JSON control");
      return;
    }

    const char* device = doc["device"];
    const char* action = doc["action"];

    if (device && action) {
      // 1. Output setup array config (from dashboard output config modal)
      if (strcmp(device, "outputs_setup") == 0 && strcmp(action, "configure") == 0) {
        JsonArray params = doc["params"].as<JsonArray>();
        preferences.begin("outputs", false);
        for (int i=0; i < params.size() && i < NUM_OUTPUTS; i++) {
          outputs[i].id = params[i]["id"].as<String>();
          outputs[i].pin = params[i]["pin"].as<int>();
          outputs[i].enabled = params[i]["enabled"].as<bool>();
          
          String base = "out_" + String(i);
          preferences.putString((base + "_id").c_str(), outputs[i].id);
          preferences.putInt((base + "_pin").c_str(), outputs[i].pin);
          preferences.putBool((base + "_en").c_str(), outputs[i].enabled);
        }
        preferences.end();
        
        updateOutputPins(); // run pinMode
        publishStatus();
        Serial.println("Outputs Setup configured dynamically and saved to NVS.");
        return;
      }
      
      // 2. Telegram webhook config
      if (strcmp(device, "telegram") == 0 && strcmp(action, "configure") == 0) {
        teleEnabled = doc["params"]["enabled"] | false;
        teleToken = doc["params"]["token"].as<String>();
        teleChatId = doc["params"]["chatId"].as<String>();
        
        preferences.begin("telegram", false);
        preferences.putBool("enabled", teleEnabled);
        preferences.putString("token", teleToken);
        preferences.putString("chatId", teleChatId);
        preferences.end();
        Serial.println("Telegram Config saved.");
        if(teleEnabled) sendTelegram("Bíp bíp! ESP32 System Telegram Configured Successfully!");
        return;
      }

      // 3. Modbus Config
      if (strcmp(device, "modbus") == 0 && strcmp(action, "configure") == 0) {
        // Doc cac thong so co ban nhu port, baudrate (da support tu truoc) thi nam trong doc["params"]["node"]
        int node = doc["params"]["node"] | 1;
        // Luu vao Preferences (Co the phai khoi dong lai de nhan baudrate/serial, o day tam bo qua baud rate)
        
        // Map configs
        if (doc["params"]["map"].is<JsonArray>()) {
          JsonArray mapArray = doc["params"]["map"].as<JsonArray>();
          preferences.begin("modbus", false);
          for (int i=0; i < mapArray.size() && i < NUM_MODBUS_PARAMS; i++) {
             modbusMap[i].id = mapArray[i]["id"].as<String>();
             modbusMap[i].reg = mapArray[i]["reg"].as<int>();
             modbusMap[i].type = mapArray[i]["type"].as<String>();
             
             String base = "mb_" + String(i);
             preferences.putString((base + "_id").c_str(), modbusMap[i].id);
             preferences.putInt((base + "_reg").c_str(), modbusMap[i].reg);
             preferences.putString((base + "_ty").c_str(), modbusMap[i].type);
          }
          preferences.end();
          Serial.println("Modbus Register Map configured dynamically and saved to NVS.");
          sendTelegram("✅ Cấu hình Bản đồ Thanh ghi Modbus (Dynamic Register Map) đã lưu thành công!");
        }
        return;
      }

      // 4. Command to trigger toggles (relay, led, out3, out4, etc)
      bool found = false;
      for (int i = 0; i < NUM_OUTPUTS; i++) {
        if (outputs[i].enabled && outputs[i].id == String(device)) {
          if (strcmp(action, "toggle") == 0) {
            outputs[i].state = !outputs[i].state;
          } else if (strcmp(action, "on") == 0) {
            outputs[i].state = true;
          } else if (strcmp(action, "off") == 0) {
            outputs[i].state = false;
          }
          digitalWrite(outputs[i].pin, outputs[i].state ? HIGH : LOW);
          Serial.printf("Output %s (Pin %d) is now: %s\n", outputs[i].id.c_str(), outputs[i].pin, outputs[i].state ? "ON" : "OFF");
          found = true;
          break;
        }
      }
      if (found) publishStatus();
    }
  }
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
  StaticJsonDocument<256> doc;
  
  for(int i=0; i<NUM_OUTPUTS; i++){
    if(outputs[i].enabled) {
      doc[outputs[i].id] = outputs[i].state;
    }
  }

  char buffer[256];
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
  float currentTemp = dht.readTemperature();
  float currentHum = dht.readHumidity();
  lightLevel = analogRead(LDR_PIN);

  // Fault Detection - DHT (Temp & Hum)
  if (isnan(currentTemp) || isnan(currentHum)) {
    if (!sensorFault) {
      Serial.println("DHT Sensor failed!");
      sendTelegram("⚠️ ALARM: Cảm biến DHT22 (Temp & Hum) đã mất kết nối hoặc bị hỏng!");
      sensorFault = true;
    }
    temperature = 0; humidity = 0; // fallback ui
  } else {
    temperature = currentTemp;
    humidity = currentHum;
    if (sensorFault) {
      sendTelegram("✅ OK: Cảm biến DHT22 đã hoạt động trở lại bình thường.");
      sensorFault = false;
      lastTeleMessage = ""; // Reset de co the gui alarm moi neu can
    }
  }

  // Fault Detection - LDR (Light)
  // Neu mat ket noi hoan toan hoac chap pin, thuong analogRead cho 0 hoac = 4095 giat lien tuc
  if (lightLevel <= 0 || lightLevel >= 4095) {
    if (!lightFault) {
      sendTelegram("⚠️ ALARM: Cảm biến Ánh Sáng (LDR) bị đứt dây hoặc chập mạch!");
      lightFault = true;
    }
  } else {
    if (lightFault) {
      sendTelegram("✅ OK: Cảm biến Ánh Sáng (LDR) đã khôi phục tín hiệu.");
      lightFault = false;
      lastTeleMessage = "";
    }
  }

  Serial.printf("Temp: %.1f°C | Hum: %.1f%% | Light: %d\n", temperature, humidity, lightLevel);
}

// ===== READ POWER METER VIA MODBUS RTU =====
void readMeter() {
  float vals[NUM_MODBUS_PARAMS] = {0};
  bool timeout = false;

  for (int i=0; i<NUM_MODBUS_PARAMS; i++) {
    float v = readDynamicModbus(modbusMap[i].reg, modbusMap[i].type);
    if (v < 0) {
       timeout = true; 
       break; 
    }
    vals[i] = v;
    delay(50); // Delay giua cac lenh doc khong dong ho bi ngheng
  }

  if (timeout) {
    if (!meterFault) {
      Serial.println("Modbus Meter reading timeout!");
      sendTelegram("⚠️ ALARM: Mất kết nối Modbus RTU tới Đồng hồ điện!");
      meterFault = true;
    }
    meterVoltage = 0; meterCurrent = 0; meterPower = 0;
    meterEnergy = 0; meterFrequency = 0; meterPF = 0;
  } else {
    // Map theo dungs ID luu
    for (int i=0; i<NUM_MODBUS_PARAMS; i++) {
       if (modbusMap[i].id == "voltage") meterVoltage = vals[i];
       else if (modbusMap[i].id == "current") meterCurrent = vals[i];
       else if (modbusMap[i].id == "power") meterPower = vals[i];
       else if (modbusMap[i].id == "energy") meterEnergy = vals[i];
       else if (modbusMap[i].id == "freq") meterFrequency = vals[i];
       else if (modbusMap[i].id == "pf") meterPF = vals[i];
    }
    
    if (meterFault) {
      sendTelegram("✅ OK: Quá trình đọc dữ liệu Modbus Đồng hồ điện đã khôi phục.");
      meterFault = false;
      lastTeleMessage = "";
    }
  }

  Serial.printf("Meter: %.1fV | %.2fA | %.1fW | %.2fkWh | %.1fHz | PF:%.2f\n",
    meterVoltage, meterCurrent, meterPower, meterEnergy, meterFrequency, meterPF);
}

// ===== SETUP =====
void updateOutputPins() {
  for (int i = 0; i < NUM_OUTPUTS; i++) {
    if (outputs[i].enabled) {
      pinMode(outputs[i].pin, OUTPUT);
      digitalWrite(outputs[i].pin, outputs[i].state ? HIGH : LOW);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 IoT Dashboard ===");

  // Load Telegram preferences
  preferences.begin("telegram", true);
  teleEnabled = preferences.getBool("enabled", false);
  teleToken = preferences.getString("token", "");
  teleChatId = preferences.getString("chatId", "");
  preferences.end();
  
  // Load Output configs from NVS
  preferences.begin("outputs", true);
  for (int i=0; i < NUM_OUTPUTS; i++) {
    String base = "out_" + String(i);
    outputs[i].id = preferences.getString((base + "_id").c_str(), outputs[i].id);
    outputs[i].pin = preferences.getInt((base + "_pin").c_str(), outputs[i].pin);
    outputs[i].enabled = preferences.getBool((base + "_en").c_str(), outputs[i].enabled);
  }
  preferences.end();
  
  // Load Modbus configs from NVS
  preferences.begin("modbus", true);
  for (int i=0; i < NUM_MODBUS_PARAMS; i++) {
    String base = "mb_" + String(i);
    modbusMap[i].id = preferences.getString((base + "_id").c_str(), modbusMap[i].id);
    // getInt will return 0 if key not found, so we check if key exists or just fallback safely
    if (preferences.isKey((base + "_reg").c_str())) {
      modbusMap[i].reg = preferences.getInt((base + "_reg").c_str(), modbusMap[i].reg);
      modbusMap[i].type = preferences.getString((base + "_ty").c_str(), modbusMap[i].type);
    }
  }
  preferences.end();

  // Output Pins init
  updateOutputPins();

  pinMode(RS485_DE_RE_PIN, OUTPUT);
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
  mqtt.setBufferSize(2048); // MUST be > 512 to receive outputs_setup JSON array!

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
