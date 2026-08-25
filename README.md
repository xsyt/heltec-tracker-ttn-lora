# Heltec Wireless Tracker v1.2 - TTN LoRa Movement Sensor

Arduino code for a movement detection sensor using Heltec Wireless Tracker v1.2 that sends data to The Things Network (TTN) Europe via LoRa.

## Features

- **Movement Detection**: Spring-loaded contact sensor on GPIO 4
- **LoRa Communication**: Sends data to TTN Europe (868 MHz)
- **OTAA Join Method**: Secure over-the-air activation
- **OLED Display**: Real-time status and movement count display
- **Low Power**: Optimized for battery operation
- **Debouncing**: 100ms debounce to prevent false triggers

## Hardware Requirements

- Heltec Wireless Tracker v1.2
- Spring-loaded contact sensor (connected to GPIO 4)
- USB cable for programming and power
- TTN account with registered device

## Software Requirements

- Arduino IDE 1.8.13 or later
- Heltec ESP32 board support
- IBM LMIC library for Arduino
- Heltec library

## Installation

### 1. Add Board Support

In Arduino IDE:
- Go to **File → Preferences**
- Add this URL to "Additional Boards Manager URLs":
  ```
  https://raw.githubusercontent.com/Heltec-Aaron-Lee/WiFi_Kit_series/master/package_heltec_index.json
  ```
- Go to **Tools → Board → Board Manager**
- Search for "Heltec" and install "Heltec ESP32 Series"

### 2. Install Libraries

In Arduino IDE, go to **Sketch → Include Library → Manage Libraries**:

- Search for and install **IBM LMIC library**
- Search for and install **Heltec ESP32 Dev Boards**

### 3. Configure Board Settings

In Arduino IDE, go to **Tools** and set:
- Board: `Heltec Wireless Tracker`
- Upload Speed: `921600` or `115200`
- CPU Frequency: `80MHz`
- Flash Size: `4MB`
- Partition Scheme: `Default 4MB with spiffs`

## Configuration

### 1. TTN Device Registration

1. Go to [The Things Network Console](https://console.cloud.thethings.network)
2. Create an application in EU region
3. Register a new end device with OTAA join method
4. Copy your:
   - **Device EUI** (DevEUI)
   - **Application EUI** (AppEUI)
   - **Application Key** (AppKey)

### 2. Update Arduino Code

Replace the placeholder values in the sketch:

```cpp
static const u1_t PROGMEM DEVEUI[8] = { 0x00, 0x00, ... };  // Your DevEUI
static const u1_t PROGMEM APPEUI[8] = { 0x00, 0x00, ... };  // Your AppEUI
static const u1_t PROGMEM APPKEY[16] = { 0x00, 0x00, ... }; // Your AppKey
```

**Important**: DevEUI and AppEUI must be entered in **LSB (Least Significant Byte first)** format in the code.

### 3. Sensor Wiring

Connect the spring-loaded contact sensor:
- One terminal to **GPIO 4**
- Other terminal to **GND**
- The internal pull-up resistor will handle the logic (LOW = contact closed/movement, HIGH = open)

## Usage

1. Upload the sketch to your Heltec Tracker
2. Open Serial Monitor (115200 baud) to see debug messages
3. The device will attempt to join TTN (watch OLED display)
4. Once joined, the device sends movement count every 60 seconds
5. Movement events are detected and counted in real-time

## Data Format

The payload sent to TTN contains:
- **4 bytes**: Movement count (big-endian 32-bit unsigned integer)

Decode in TTN Console with this JavaScript payload decoder:

```javascript
function Decoder(bytes, port) {
  return {
    movements: (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]
  };
}
```

## Customization

### Change TX Interval

Modify this line (in seconds):
```cpp
const unsigned long TX_INTERVAL = 60; // Send every 60 seconds
```

### Change Debounce Time

Modify this line (in milliseconds):
```cpp
const unsigned long DEBOUNCE_TIME = 100; // 100ms debounce
```

### Adjust LoRa Spreading Factor

The default is SF7. To change it:
```cpp
LMIC_setDrTxpow(DR_SF9, 14); // Use DR_SF7 through DR_SF12
```

## Troubleshooting

### Device not joining TTN
- Verify DevEUI, AppEUI, and AppKey are correct and in LSB format
- Check antenna connection
- Ensure you're in range of a TTN gateway
- Check Serial Monitor for error messages

### Movement not detected
- Test GPIO 4 with a multimeter (should be HIGH when open, LOW when closed)
- Verify debounce time isn't too long
- Check the spring mechanism is working properly

### No data received in TTN Console
- Confirm device has joined (OLED shows "Joined")
- Check TTN Console application settings
- Verify payload decoder is configured

## References

- [Heltec Wireless Tracker Documentation](https://heltec.org/project/wireless-tracker/)
- [The Things Network Documentation](https://www.thethingsnetwork.org/docs/)
- [IBM LMIC Library](https://github.com/mcci-catena/arduino-lmic)

## License

This project is provided as-is for personal and educational use.
