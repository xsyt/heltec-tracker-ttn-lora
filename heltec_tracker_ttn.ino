/*
 * Heltec Wireless Tracker v1.2 - TTN Europe LoRa Movement Sensor
 * 
 * Features:
 * - Movement detection via spring-loaded contact on GPIO 4
 * - LoRa communication with TTN Europe
 * - OTAA Join Method
 * - Low power optimization
 * - Serial debug output
 * 
 * Compatible with: Heltec ESP32 Dev-Boards v2.1.6+ (using LoRaWAN library)
 * Based on: Heltec LoRaWAN example
 */

#include "LoRaWan_APP.h"

// ===== TTN OTAA Configuration =====
// Format: LSB (Least Significant Byte first)
// Get these values from TTN Console
uint8_t devEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appEui[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// ===== LoRaWAN Configuration =====
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;  // EU868
DeviceClass_t loraWanClass = CLASS_A;
uint32_t appTxDutyCycle = 60000;  // 60 seconds
bool overTheAirActivation = true;
bool loraWanAdr = true;
bool isTxConfirmed = false;  // Unconfirmed uplinks
uint8_t appPort = 10;
uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };
uint8_t confirmedNbTrials = 4;

// ===== Application Variables =====
const int MOVEMENT_SENSOR_PIN = 4;  // Spring-loaded contact on GPIO 4
uint32_t movement_count = 0;
bool movement_detected = false;
unsigned long last_movement_time = 0;
const unsigned long DEBOUNCE_TIME = 100;  // 100ms debounce

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println(F("\n\nHeltec Tracker v1.2 - TTN LoRa Movement Sensor"));
    Serial.println(F("Using Heltec LoRaWAN Library"));
    
    // Initialize Heltec MCU
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
    Serial.println(F("Heltec MCU initialized"));
    
    // Setup movement sensor pin
    pinMode(MOVEMENT_SENSOR_PIN, INPUT_PULLUP);
    Serial.print(F("Movement sensor on GPIO: "));
    Serial.println(MOVEMENT_SENSOR_PIN);
    
    Serial.println(F("Setup complete!"));
}

void loop() {
    // Check for movement
    checkMovement();
    
    // Handle LoRaWAN state machine
    switch(deviceState) {
        case DEVICE_STATE_INIT:
        {
#if(LORAWAN_DEVEUI_AUTO)
            LoRaWAN.generateDeveuiByChipID();
#endif
            LoRaWAN.init(loraWanClass, loraWanRegion);
            LoRaWAN.setDefaultDR(3);
            Serial.println(F("LoRaWAN initialized"));
            break;
        }
        case DEVICE_STATE_JOIN:
        {
            Serial.println(F("Joining TTN..."));
            LoRaWAN.join();
            break;
        }
        case DEVICE_STATE_SEND:
        {
            prepareTxFrame(appPort);
            LoRaWAN.send();
            deviceState = DEVICE_STATE_CYCLE;
            break;
        }
        case DEVICE_STATE_CYCLE:
        {
            // Schedule next packet transmission
            txDutyCycleTime = appTxDutyCycle + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
            LoRaWAN.cycle(txDutyCycleTime);
            deviceState = DEVICE_STATE_SLEEP;
            break;
        }
        case DEVICE_STATE_SLEEP:
        {
            LoRaWAN.sleep(loraWanClass);
            break;
        }
        default:
        {
            deviceState = DEVICE_STATE_INIT;
            break;
        }
    }
}

void checkMovement() {
    int sensorState = digitalRead(MOVEMENT_SENSOR_PIN);
    unsigned long current_time = millis();
    
    // Detect LOW state (contact closed) with debounce
    if (sensorState == LOW && !movement_detected && (current_time - last_movement_time > DEBOUNCE_TIME)) {
        movement_detected = true;
        last_movement_time = current_time;
        movement_count++;
        
        Serial.print(F(">> Movement detected! Total count: "));
        Serial.println(movement_count);
    }
    
    // Detect HIGH state (contact open)
    if (sensorState == HIGH && movement_detected) {
        movement_detected = false;
    }
}

/* Prepare payload with movement count */
static void prepareTxFrame(uint8_t port) {
    appDataSize = 4;
    // Send movement count as big-endian 4-byte value
    appData[0] = (movement_count >> 24) & 0xFF;
    appData[1] = (movement_count >> 16) & 0xFF;
    appData[2] = (movement_count >> 8) & 0xFF;
    appData[3] = movement_count & 0xFF;
    
    Serial.print(F("Sending movement count: "));
    Serial.println(movement_count);
}
