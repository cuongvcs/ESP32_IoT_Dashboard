# ESP32 IoT Dashboard - Huong dan su dung

## Tong quan
He thong IoT dung ESP32S ket noi WiFi nha, gui du lieu cam bien va dong ho dien len MQTT broker.
Dien thoai truy cap web dashboard de giam sat va dieu khien tu bat ky dau. Dashboard ho tro theo doi cam bien so, dong ho dien nang Modbus RTU, va cac cam bien Analog chuyen dung (0-10V, 4-20mA).

Hien tai, he thong cho phep **tuy bien dong cac chan dieu khien Output (GPIO)** truc tiep tren giao dien Web!

## Phan cung can thiet

| Linh kien | So luong | Ghi chu |
|---|---|---|
| ESP32S DevKit | 1 | Board chinh (Tap trung I/O) |
| DHT22 / AM2302 | 1 | Cam bien nhiet do + do am |
| MAX485 Module | 1 | Chuyen doi RS485-TTL |
| Dong ho dien Modbus RTU | 1 | VD: Eastron SDM120 |
| Relays / LEDs | Tuy chon | 1 den 6 thiet bi dieu khien |
| Cam bien Analog cong nghiep | Tuy chon | Cam bien 0-10V, 4-20mA |

## So do noi day & Master ESP32 Pinout

ESP32 so huu nhieu chan I/O, nhung mot so chan duoc uu tien cho cac khoi giao tiep khac nhau. Duoi day la bang phan bo chan (Pinout) tong hop.

### 1. Giao tiep I2C & Modbus ghep cau (Co Dinh)
```text
ESP32 GPIO16   --->   MAX485 RO (Receive)
ESP32 GPIO17   --->   MAX485 DI (Transmit)
ESP32 GPIO5    --->   MAX485 DE + RE (Noi chung)

MAX485 A/B     --->   Dong ho dien nang Modbus RTU
```

### 2. Cam bien so & Analog (Nhap vao thong bao)
```text
ESP32 GPIO4    --->   DHT22 DATA (Them tro keo len 4.7k)
ESP32 GPIO34   --->   Trang thai LDR (Voltage Divider voi tro 10k)
ESP32 GPIO32   --->   ADC cho Cam bien Analog 0-10V (Can cau phan ap tu 10V xuong 3.3V)
ESP32 GPIO33   --->   ADC cho Cam bien Analog 4-20mA (Can tro shunt 165 ohm)
```

### 3. Output - Cac chan dieu khien (Tuy chinh tren trinh duyet)
Ban co the dang ky toi da 6 Output moi thong qua giao dien Settings tren Web (`Output Pin Configuration`). Day la cac chan duoc de xuat:
```text
ESP32 GPIO26   --->   Mac dinh dung cho Relay 1 (Dieu khien tai 220V)
ESP32 GPIO2    --->   Mac dinh dung cho Status LED (Can tro 220 Ohm)
ESP32 GPIO14   --->   Output tuy chon (Bơm)
ESP32 GPIO12   --->   Output tuy chon (Quạt)
ESP32 GPIO13   --->   Output tuy chon (Còi báo động)
ESP32 GPIO27   --->   Output tuy chon (Lò sưởi)
```

## Cai dat phan mem

### 1. Arduino IDE
1. Tai Arduino IDE tu: https://www.arduino.cc/en/software
2. Vao File > Preferences > Additional boards manager URLs, them:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. Vao Tools > Board > Boards Manager, tim "esp32" va cai dat
4. Chon Board: "ESP32 Dev Module"

### 2. Thu vien can cai (Arduino Library Manager)
- **PubSubClient** (Nick O'Leary) - MQTT client
- **DHT sensor library** (Adafruit) - Doc DHT22
- **ArduinoJson** (Benoit Blanchon) - Xu ly JSON
- **ModbusMaster** (Doc Walker) - Giao tiep Modbus RTU

### 3. Cau hinh
Mo file `config.h` va dien:
- WiFi SSID va password nha ban
- MQTT broker URL, port, username, password
- Code se lang nghe lenh MQTT tai `esp32/control` de thiet lap I/O dong.

## Su dung Dashboard

1. Mo file `dashboard.html` tren trinh duyet.
2. Dien thong tin MQTT broker (URL, port, username, password) tai Tab **Settings**.
3. Nhan "Connect".
4. Dashboard se hien thi du lieu cam bien va dong ho dien real-time.
5. De them/chinh sua Chan Dieu Khien (Outputs), mo **Settings** -> **Output Pin Configuration**, danh dau tick va dien ma chan GPIO da dau noi tren phan cung. Sau do luu thiet lap, code ESP32 se cap nhat Mode de xuat tin hieu.
6. Tuong tu, ban co the sua dai do cua Cam bien Analog tai **Analog Sensor Config**.

## MQTT Broker
- Dang ky tai khoan mien phi tai: https://console.hivemq.cloud/
- Tao Cluster mien phi (Free Serverless)
- Tao MQTT credentials (username/password)
- Lay broker URL va port (8883 cho TLS, 8884 cho WebSocket TLS)
