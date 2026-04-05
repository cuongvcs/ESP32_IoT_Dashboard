# Bill of Materials (BOM) - ESP32 IoT Dashboard

## Project: ESP32S Monitoring & Control System
**Version:** 1.1  
**Date:** 2026-03-24

---

## Main Controller

| # | Component | Specification | Qty | Notes |
|---|-----------|--------------|-----|-------|
| 1 | ESP32S DevKit V1 | ESP32-WROOM-32, WiFi + BLE, 38 pins | 1 | Main controller board |

## Sensors

| # | Component | Specification | Qty | GPIO | Notes |
|---|-----------|--------------|-----|------|-------|
| 2 | DHT22 (AM2302) | Temp: -40~80°C, Hum: 0~100% | 1 | GPIO4 | Include 4.7kΩ pull-up |
| 3 | LDR (Photoresistor) | 5mm, 5~500kΩ | 1 | GPIO34 (ADC) | Voltage divider with R5 |
| 4 | Analog 0-10V Sensor | Industrial 0-10V Standard | 1 | GPIO32 (ADC) | Needs voltage divider (e.g. 10k/4.7k) to drop to 3.3V |
| 5 | Analog 4-20mA Sensor | Industrial Current Loop | 1 | GPIO33 (ADC) | Needs 165Ω precision shunt resistor to convert to 3.3V |

## Actuators (Configurable Outputs)

*Note: The Dashboard now supports dynamic output pin allocation. You can wire LEDs or Relays to any available GPIO and configure them via the dashboard.*

| # | Component | Specification | Qty | GPIO (Default) | Notes |
|---|-----------|--------------|-----|------|-------|
| 6 | Relay Module 5V | 1 Channel, 10A/250VAC | 1 | GPIO26 | Active HIGH |
| 7 | LED 5mm | Green/Red or any color | 1 | GPIO2 | Status indicator |
| 8 | Relay/LED Expansion | Additional Modules | Max 4 | 14, 12, 13, 27 | Expandable via `Output Pin Configuration` tab |

## RS485 / Modbus RTU (Power Meter)

| # | Component | Specification | Qty | GPIO | Notes |
|---|-----------|--------------|-----|------|-------|
| 9 | MAX485 Module | TTL to RS485, 3.3V/5V | 1 | RX:16, TX:17, DE/RE:5 | Half-duplex |
| 10| Power Meter | Eastron SDM120 / SDM630 | 1 | RS485 A/B | Slave ID: 1, Baud: 9600 |

## Passive Components

| # | Component | Specification | Qty | Notes |
|---|-----------|--------------|-----|-------|
| 11 | Resistor | 220Ω, 1/4W | 1 | LED current limiting |
| 12 | Resistor | 10kΩ, 1/4W | 1 | LDR voltage divider |
| 13 | Resistor | 4.7kΩ, 1/4W | 1 | DHT22 pull-up |
| 14 | Precision Resistor| 165Ω, 1% | 1 | For 4-20mA current-to-voltage sensing |

## Connectors & Supplies

| # | Component | Specification | Qty | Notes |
|---|-----------|--------------|-----|-------|
| 15 | Breadboard | 830 points, full size | 1 | Prototyping |
| 16 | Jumper Wires | Male-Male, Male-Female | 30 | Breadboard & Sensor connections |
| 17 | Micro USB Cable | USB-A to Micro-B | 1 | ESP32 programming & power |
| 18 | USB Charger 5V | 5V DC, min 2A | 1 | ESP32 standalone power |

---

## Wiring Summary (Master IO Map)

```text
# Fixed Interface Pins:
ESP32 GPIO4  --> DHT22 DATA
ESP32 GPIO16 --> MAX485 RO
ESP32 GPIO17 --> MAX485 DI
ESP32 GPIO5  --> MAX485 DE+RE

# Analog Inputs (Sensors):
ESP32 GPIO34 --> LDR (voltage divider)
ESP32 GPIO32 --> 0-10V Interface (with divider)
ESP32 GPIO33 --> 4-20mA Interface (with shunt)

# Digital Outputs (Configurable in Dashboard):
ESP32 GPIO26 --> Relay 1 IN
ESP32 GPIO2  --> LED 1 Anode
ESP32 GPIO14 --> Expansion Out 3
ESP32 GPIO12 --> Expansion Out 4
ESP32 GPIO13 --> Expansion Out 5
ESP32 GPIO27 --> Expansion Out 6
```
