// ============================================================
// ESP32 IoT Dashboard - Configuration File
// ============================================================
// Chinh sua cac thong so ben duoi cho phu hop voi he thong cua ban

#ifndef CONFIG_H
#define CONFIG_H

// ===== WIFI CONFIGURATION =====
// Dien ten WiFi va mat khau nha ban vao day
#define WIFI_SSID       "Cuong VCS"
#define WIFI_PASSWORD   "minhtri2016"

// ===== MQTT CONFIGURATION =====
// Dien thong tin MQTT broker (HiveMQ Cloud hoac broker khac)
// Sau khi tao tai khoan, dien cac thong tin vao day
#define MQTT_BROKER     "19f8cf0470ae4506af9c8ccb827ba06b.s1.eu.hivemq.cloud"  // VD: "abc123.s1.eu.hivemq.cloud"
#define MQTT_PORT       8883                             // TLS port
#define MQTT_USER       "cuongvcs"
#define MQTT_PASS       "Minhphuc@01"
#define MQTT_CLIENT_ID  "ESP32_IoT_Dashboard"

// ===== MQTT TOPICS =====
#define TOPIC_SENSORS   "esp32/sensors"
#define TOPIC_METER     "esp32/meter"
#define TOPIC_STATUS    "esp32/status"
#define TOPIC_CONTROL   "esp32/control"
#define TOPIC_WIFI      "esp32/wifi_status"

// ===== GPIO PIN MAPPING =====
// DHT22 - Cam bien nhiet do & do am
#define DHT_PIN         4
#define DHT_TYPE        DHT22

// Relay Module
#define RELAY_PIN       26

// LED
#define LED_PIN         2    // LED_BUILTIN tren ESP32 DevKit

// LDR - Cam bien anh sang (Analog)
#define LDR_PIN         34   // GPIO34 = ADC1_CH6

// RS485 Modbus RTU - Dong ho dien
#define RS485_RX_PIN    16   // RO pin cua MAX485
#define RS485_TX_PIN    17   // DI pin cua MAX485
#define RS485_DE_RE_PIN 5    // DE + RE pin cua MAX485 (noi chung)
#define MODBUS_SLAVE_ID 1    // Dia chi Modbus cua dong ho dien
#define MODBUS_BAUD     9600 // Toc do truyen Modbus

// ===== SENSOR READ INTERVAL =====
#define SENSOR_INTERVAL   2000   // Doc cam bien moi 2 giay (ms)
#define METER_INTERVAL    5000   // Doc dong ho dien moi 5 giay (ms)
#define MQTT_PUB_INTERVAL 3000   // Gui data len MQTT moi 3 giay (ms)

// ===== MODBUS REGISTER MAP (Dong ho dien pho thong) =====
// Cac dia chi register nay phu thuoc vao loai dong ho dien cua ban
// Duoi day la gia tri mac dinh cho dong ho Eastron SDM120/SDM630
#define REG_VOLTAGE     0x0000   // Dien ap (V)    - Float 32bit
#define REG_CURRENT     0x0006   // Dong dien (A)  - Float 32bit
#define REG_POWER       0x000C   // Cong suat (W)  - Float 32bit
#define REG_ENERGY      0x0156   // Nang luong (kWh) - Float 32bit
#define REG_FREQUENCY   0x0046   // Tan so (Hz)    - Float 32bit
#define REG_PF          0x001E   // He so cong suat (PF) - Float 32bit

#endif // CONFIG_H
