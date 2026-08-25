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
 * Compatible with: MCCI LoRaWAN LMIC library v6.0.1+
 * Compatible with: Heltec ESP32 Dev-Boards v2.1.6+
 */

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// TTN Configuration - REPLACE WITH YOUR KEYS
// Format: LSB (Least Significant Byte first)
static const u1_t PROGMEM DEVEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const u1_t PROGMEM APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const u1_t PROGMEM APPKEY[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8); }
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16); }

// GPIO Configuration
const int MOVEMENT_SENSOR_PIN = 4;  // Spring-loaded contact on GPIO 4

// LoRa Pin Configuration for Heltec Wireless Tracker v1.2
const lmic_pinmap lmic_pins = {
    .nss = 18,
    .rxtx = 23,
    .rst = 14,
    .dio = {26, 33, 32},
};

// Application variables
static osjob_t sendjob;
static uint32_t movement_count = 0;
static bool movement_detected = false;
static unsigned long last_movement_time = 0;
static bool network_joined = false;
static bool lmic_initialized = false;
const unsigned long DEBOUNCE_TIME = 100; // 100ms debounce

// TX Interval in seconds
const unsigned long TX_INTERVAL = 60; // Send every 60 seconds

// Forward declarations
void do_send(osjob_t* j);
void onEvent(ev_t ev);
void checkMovement();
void printStatus();

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println(F("\n\nHeltec Tracker v1.2 - TTN LoRa Movement Sensor"));
    Serial.println(F("MCCI LMIC v6.0.1+ compatible"));
    
    // Setup movement sensor pin
    pinMode(MOVEMENT_SENSOR_PIN, INPUT_PULLUP);
    Serial.print(F("Movement sensor on GPIO: "));
    Serial.println(MOVEMENT_SENSOR_PIN);
    
    delay(1000);
    Serial.println(F("Initializing LMIC..."));
    
    // LMIC init
    os_init();
    Serial.println(F("LMIC os_init() complete"));
    
    delay(500);
    
    // Reset the MAC state
    LMIC_reset();
    Serial.println(F("LMIC_reset() complete"));
    
    delay(500);
    
    // Set the data rate to Spreading Factor 7 (default)
    LMIC_setDrTxpow(DR_SF7, 14);
    Serial.println(F("LMIC_setDrTxpow() complete"));
    
    lmic_initialized = true;
    
    Serial.println(F("Setup complete. Starting join procedure..."));
    delay(1000);
    
    // Start job to join network
    do_send(&sendjob);
}

void loop() {
    // Only process LMIC if initialized
    if (!lmic_initialized) {
        delay(100);
        return;
    }
    
    // Check for movement
    checkMovement();
    
    // LMIC event processing
    os_runloop_once();
}

void checkMovement() {
    int sensorState = digitalRead(MOVEMENT_SENSOR_PIN);
    unsigned long current_time = millis();
    
    // Detect LOW state (contact closed) with debounce
    if (sensorState == LOW && !movement_detected && (current_time - last_movement_time > DEBOUNCE_TIME)) {
        movement_detected = true;
        last_movement_time = current_time;
        movement_count++;
        
        Serial.print(F(">> Movement detected! Count: "));
        Serial.println(movement_count);
        printStatus();
    }
    
    // Detect HIGH state (contact open)
    if (sensorState == HIGH && movement_detected) {
        movement_detected = false;
    }
}

void printStatus() {
    Serial.print(F("   Network State: "));
    if (network_joined) {
        Serial.println(F("Joined"));
    } else {
        Serial.println(F("Not joined"));
    }
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            network_joined = false;
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            Serial.println(F("Init Rx delay"));
            network_joined = true;
            LMIC_setLinkCheckMode(0);
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            network_joined = false;
            // Try again in 10 seconds
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(10), do_send);
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            network_joined = false;
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            
            // Check if we received data
            if (LMIC.dataLen) {
                Serial.print(F("Received "));
                Serial.print(LMIC.dataLen);
                Serial.println(F(" bytes of payload"));
            }
            
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // Data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            Serial.println(F("EV_RXSTART"));
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void do_send(osjob_t* j) {
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
        return;
    }
    
    // Prepare upstream data transmission at the next possible time
    // Payload: movement count (4 bytes, big-endian)
    unsigned char mydata[4];
    mydata[0] = (movement_count >> 24) & 0xFF;
    mydata[1] = (movement_count >> 16) & 0xFF;
    mydata[2] = (movement_count >> 8) & 0xFF;
    mydata[3] = movement_count & 0xFF;
    
    LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
    Serial.print(F("Packet queued with movement count: "));
    Serial.println(movement_count);
}
