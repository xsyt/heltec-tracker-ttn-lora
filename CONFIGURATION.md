# Heltec Wireless Tracker v1.2 LoRa Configuration Guide

## Quick Start Checklist

- [ ] Install Arduino IDE and board support
- [ ] Install required libraries (LMIC, Heltec)
- [ ] Register device on TTN Console (EU region)
- [ ] Copy DevEUI, AppEUI, AppKey from TTN
- [ ] Update these values in the sketch (LSB format)
- [ ] Connect spring sensor to GPIO 4
- [ ] Upload sketch to Heltec Tracker
- [ ] Monitor Serial output and OLED display
- [ ] Verify device joins TTN
- [ ] Add payload decoder to TTN Application

## Pin Configuration Reference

### Heltec Wireless Tracker v1.2 LoRa Pins (Hardware SPI)

```
LoRa Module:
- NSS (Chip Select): GPIO 18
- MOSI: GPIO 27
- MISO: GPIO 19
- SCK: GPIO 5

LoRa Control:
- RST (Reset): GPIO 14
- DIO0: GPIO 26
- DIO1: GPIO 33
- DIO2: GPIO 32
- RxTx: GPIO 23

Other:
- OLED SDA: GPIO 4 (CONFLICT - use different sensor GPIO!)
- OLED SCL: GPIO 15
```

## IMPORTANT: GPIO 4 Conflict

**Note**: The standard Heltec Wireless Tracker uses GPIO 4 for OLED SDA. If you need to use GPIO 4 for the sensor:

### Option 1: Use Different GPIO
Edit the sketch:
```cpp
const int MOVEMENT_SENSOR_PIN = 36;  // Use GPIO 36 (ADC input) instead
```

### Option 2: Reconfigure OLED (Advanced)
If you must use GPIO 4, reconfigure the OLED pins before Heltec.begin():
```cpp
// In Heltec library config, change I2C pins before initialization
// This requires modifying the Heltec library configuration
```

**Recommended**: Use GPIO 36, 37, 38, or 39 (ADC pins) for the sensor.

## TTN Configuration Steps

### 1. Create TTN Application
```
Console → Applications → Create Application
- Application ID: my-movement-sensor
- Region: Europe 1 (EU868)
```

### 2. Register Device
```
Application → Devices → Register Device
- Device ID: tracker-movement-01
- DevEUI: Generate (or use yours)
- AppEUI: Select from application
- AppKey: Generate
- Activation method: OTAA
```

### 3. Get Credentials
In Device Overview, note:
- **Device EUI** (copy as-is, will convert to LSB)
- **Application EUI** (copy as-is, will convert to LSB)
- **Application Key** (copy as-is, will convert to LSB)

### 4. Convert to LSB Format

TTN shows credentials in **MSB (Most Significant Byte first)** format.
Arduino code needs **LSB (Least Significant Byte first)** format.

**Example conversion:**
```
TTN shows (MSB):  70 B3 D5 7E D0 00 00 00
Arduino needs (LSB): { 0x00, 0x00, 0x00, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 }
```

Simply **reverse the byte order**.

### 5. Update Arduino Sketch

```cpp
// Example with reversed byte order
static const u1_t PROGMEM DEVEUI[8] = { 0x00, 0x00, 0x00, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
static const u1_t PROGMEM APPEUI[8] = { 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x00, 0x00, 0x00 };
static const u1_t PROGMEM APPKEY[16] = { /* 16 bytes in LSB */ };
```

## Payload Decoder Setup

1. Go to **Application → Integrations → Payload Decoder**
2. Select Format: **Javascript**
3. Copy code from `ttn_payload_decoder.js`
4. Test with: `70 B3 D5 7E` (example payload)

## Physical Connection Diagram

```
Heltec Wireless Tracker v1.2
┌─────────────────────────┐
│                         │
│  GPIO 36 ┬─────────────┬┤◄─ Spring Contact Sensor
│          │             ││
│  GND ────┴─────────────┴┤
│                         │
│  USB Power              │
│                         │
└─────────────────────────┘
```

Spring Contact Sensor:
- One wire: GPIO 36 (with internal pull-up)
- Other wire: GND
- Contact closed (movement): GPIO reads LOW
- Contact open (no movement): GPIO reads HIGH

## Testing the Connection

### 1. Test Sensor
Upload this test sketch:
```cpp
void setup() {
  Serial.begin(115200);
  pinMode(36, INPUT_PULLUP);
}

void loop() {
  Serial.println(digitalRead(36));
  delay(100);
}
```
Serial output should toggle between 1 (open) and 0 (closed) when you actuate the sensor.

### 2. Test LoRa Module
Check if LMIC initializes correctly by watching Serial output in the main sketch:
```
Heltec Tracker v1.2 - TTN LoRa Movement Sensor
EV_JOINING
EV_JOINED
```

### 3. Monitor TTN Console
```
Application → Devices → Your Device → Live data
```
Should show incoming uplink messages every 60 seconds.

## Transmission Details

**Default Configuration:**
- TX Interval: 60 seconds
- Spreading Factor: SF7
- Bandwidth: 125 kHz
- Region: EU868 (869.525 MHz)
- Payload: 4 bytes (movement count)

**Airtime**: ~100 ms per transmission

## Power Consumption

- Active TX: ~100 mA
- Listening: ~10 mA
- Sleep: ~0.5 mA (if deep sleep implemented)

## Debugging

### Serial Monitor Output Examples

**Joining TTN:**
```
os_init
[11700] EV_JOINING
[12345] EV_JOINED
```

**Normal Operation:**
```
Movement detected! Count: 1
[60000] Packet queued with movement count: 1
[62100] EV_TXCOMPLETE (includes waiting for RX windows)
[120000] Packet queued with movement count: 1
```

**Join Failure:**
```
[10000] EV_JOINING
[20000] EV_JOIN_FAILED
[30000] EV_JOINING
```

## Common Issues and Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| Won't join TTN | Wrong DevEUI/AppEUI/AppKey | Verify bytes are in LSB format, check TTN Console |
| Won't join TTN | No gateway in range | Use TTN coverage map to verify coverage |
| Movement not detected | GPIO 4 OLED conflict | Switch to GPIO 36, 37, 38, or 39 |
| No data in TTN Console | Device not sending | Check "TX Complete" message in Serial Monitor |
| Frequent rejoin attempts | Signal too weak | Move antenna or move to location with better coverage |

## Additional Resources

- **Heltec Wiki**: https://heltec.org/project/wireless-tracker/
- **TTN Docs**: https://www.thethingsnetwork.org/docs/
- **LMIC Library Issues**: https://github.com/mcci-catena/arduino-lmic/issues
- **Arduino IDE**: https://www.arduino.cc/en/software

