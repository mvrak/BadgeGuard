// Test Arduino sketch for RC522 RFID module with UART interface on Seeed XIAO ESP32-C6
// Note: Module requires 5V power. Connect module VCC to XIAO's 5V pin (USB powered), GND to GND.
// Module RX to XIAO D6 (TX), module TX to XIAO D7 (RX).
// Use level shifter or voltage divider on module TX to XIAO RX line (5V to 3.3V).
// Baud rate: 115200.
// This sketch resets the module, turns on antenna, then loops to detect card and print UID.

#include <HardwareSerial.h>

HardwareSerial rfidSerial(1);  // Use UART1

#define RFID_RX_PIN 7  // D7
#define RFID_TX_PIN 6  // D6

const byte HEADER = 0x7F;
const byte TAIL_SUCCESS = 0xF7;
const byte TAIL_ERROR = 0x55;

// Commands
const byte CMD_RESET = 0x00;
const byte CMD_ANTENNA_ON = 0x01;
const byte CMD_REQUEST = 0x03;
const byte CMD_ANTICOLL = 0x04;

// Request modes
const byte REQ_ALL = 0x52;  // All cards

void setup() {
  Serial.begin(115200);
  while (!Serial);

  rfidSerial.begin(115200, SERIAL_8N1, RFID_RX_PIN, RFID_TX_PIN);

  Serial.println("Initializing RFID module...");

  // Reset
  if (sendCommand(CMD_RESET, NULL, 0)) {
    Serial.println("Reset OK");
  } else {
    Serial.println("Reset failed");
  }

  // Antenna on
  if (sendCommand(CMD_ANTENNA_ON, NULL, 0)) {
    Serial.println("Antenna ON OK");
  } else {
    Serial.println("Antenna ON failed");
  }

  Serial.println("Ready. Present a card...");
}

void loop() {
  byte tagType[2];
  int respLen;

  // Send Request (PICC_REQALL)
  byte reqData = REQ_ALL;
  if (sendCommand(CMD_REQUEST, &reqData, 1, tagType, &respLen)) {
    if (respLen == 2) {
      // Got tag type, now anticoll for UID
      byte uid[4];
      if (sendCommand(CMD_ANTICOLL, NULL, 0, uid, &respLen)) {
        if (respLen == 4) {
          Serial.print("UID: ");
          for (int i = 0; i < 4; i++) {
            if (uid[i] < 0x10) Serial.print("0");
            Serial.print(uid[i], HEX);
            Serial.print(" ");
          }
          Serial.println();
        } else {
          Serial.println("Anticoll invalid response");
        }
      } else {
        Serial.println("No card (anticoll failed)");
      }
    } else {
      Serial.println("Request invalid response");
    }
  } else {
    Serial.println("No card detected");
  }

  delay(1000);
}

// Send command and get response. Returns true on success.
// response buffer should be large enough, respLen updated.
bool sendCommand(byte cmd, byte* data, int dataLen, byte* response = NULL, int* respLen = NULL) {
  // Send: 7F cmd [data] F7
  rfidSerial.write(HEADER);
  rfidSerial.write(cmd);
  if (data) rfidSerial.write(data, dataLen);
  rfidSerial.write(TAIL_SUCCESS);  // Tail is F7 for send

  // Read response: 7F [value] F7/55
  if (rfidSerial.available() < 2) return false;  // At least header + tail

  if (rfidSerial.read() != HEADER) return false;

  byte tail = TAIL_ERROR;
  int idx = 0;
  while (rfidSerial.available()) {
    byte b = rfidSerial.read();
    if (b == TAIL_SUCCESS || b == TAIL_ERROR) {
      tail = b;
      break;
    }
    if (response) response[idx++] = b;
  }

  if (respLen) *respLen = idx;

  // Clear any extra
  while (rfidSerial.available()) rfidSerial.read();

  return (tail == TAIL_SUCCESS);
}
