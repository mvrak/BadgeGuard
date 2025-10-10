# BadgeGuard

BadgeGuard is a prototype for an RFID-based access control system using a Seeed ESP32-C6 microcontroller, designed to unlock a 12V electric drop bolt lock upon scanning authorized RFID tags. Authorization data is synced from a blockchain contract (Base chain) and cached locally for quick access. The system displays lock status ("Locked" or "Open") on a 2.13" b/w e-ink display. Phase 2 will add wallet signature verification as a second authentication factor.

## Features
- **RFID Authentication**: Scans 13.56MHz RFID tags (RC522) to unlock a relay-controlled 12V lock.
- **Blockchain Integration**: Fetches authorized UIDs from a Base chain contract via JSON-RPC (e.g., Infura/Alchemy).
- **E-Ink Display**: Shows lock status on a Waveshare 2.13" V4 b/w e-ink display (250x122).
- **Phase 2 (Planned)**: Verifies wallet signatures for matched RFID tags, enhancing security.
- **Hardware**: Compact prototype with Seeed ESP32-C6, 5V relay, and diode protection for the lock.

## Hardware Requirements
- **Seeed ESP32-C6** (or fallback: ESP32-S3, Raspberry Pi)
- **HiLetgo RC522 RFID Reader** (13.56MHz)
- **5V 2-Channel Relay Module**
- **12V Electric Drop Bolt Lock** (with small wooden frame for testing)
- **Waveshare 2.13" b/w E-Ink Display HAT (V4)**
- **1N4007 Rectifier Diode** (for flyback protection)
- **12V and 5V Power Supplies**, assorted wires, casing material

## Wiring
| Component | ESP32-C6 GPIO | Notes |
|-----------|--------------|-------|
| **RC522 RFID** | VCC: 3V3, GND: GND, SDA: 10, SCK: 12, MOSI: 11, MISO: 13, RST: 9 | SPI |
| **E-Ink** | VCC: 3V3/5V, GND: GND, BUSY: 4, RST: 16, DC: 17, CS: 10, CLK: 12, DIN: 11 | Shared SPI |
| **Relay (CH1)** | VCC: 5V, GND: GND, IN1: 2 | Active low |
| **Lock** | Relay NO: 12V+, COM: 12V- | 1N4007 diode across terminals |

**Note**: Shared SPI CS (GPIO10) requires software multiplexing. Use separate 12V PSU for lock, 5V for relay/e-ink.

## Software Requirements
- **Arduino IDE** (v3.0+ for ESP32-C6 support)
- **Libraries**:
  - `MFRC522` (Miguel Balboa) - RFID
  - `GxEPD2` (ZinggJM) - E-Ink
  - `ArduinoJson` - JSON parsing
  - `web3-arduino` ([GitHub](https://github.com/kopanitsa/web3-arduino)) - Blockchain queries
- **Base Chain RPC**: Infura/Alchemy API key (Base Mainnet or Sepolia testnet, chain ID 8453/84532)
- **WiFi**: Configure SSID/password for blockchain sync

## Installation
1. **Hardware Setup**:
   - Wire components per table above.
   - Add 1N4007 diode across lock (cathode to +12V).
   - Power ESP32-C6 via USB or 5V/3.3V; lock via 12V PSU.
2. **Software Setup**:
   - Install Arduino IDE and ESP32 board support (`espressif/arduino-esp32`).
   - Install libraries via Library Manager or ZIP.
   - Configure `rpcUrl`, `ssid`, `password` in code.
3. **Upload Code**:
   - Use provided Arduino sketch (see `src/main.ino`).
   - Hardcode 2-3 UIDs for testing; replace with blockchain sync.
4. **Test**:
   - Scan RFID tags; verify unlock (5s) and display update.
   - Monitor Serial (115200) for debug output.
   - Test blockchain sync on Base Sepolia (free ETH faucet).

## Usage
- **Phase 1**: Scan RFID tag. If UID matches cached list (from blockchain), relay unlocks for 5s, e-ink shows "Open", then reverts to "Locked".
- **Phase 2 (Planned)**: On RFID match, ESP32-C6 queries wallet signature via contract, verifies with `mbedtls`, unlocks if valid.
- **Cache**: UIDs stored in `allowedUIDs[]` (EEPROM planned). Sync via HTTP JSON-RPC to Base contract (`eth_call`).

## Code Example
```cpp
#include <SPI.h>
#include <MFRC522.h>
#include <GxEPD2_BW.h>
#include <ArduinoJson.h>

#define SS_PIN 10
#define RST_PIN 9
#define RELAY_PIN 2
MFRC522 mfrc522(SS_PIN, RST_PIN);
GxEPD2_BW<GxEPD2_213_B74, GxEPD2_213_B74::HEIGHT> display(GxEPD2_213_B74(10, 17, 16, 4));
String allowedUIDs[] = {"04A1B2C3", "04D4E5F6"};
bool locked = true;

void setup() {
  Serial.begin(115200);
  SPI.begin(12, 13, 11, 10);
  mfrc522.PCD_Init();
  display.init(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  updateDisplay();
}

void loop() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uid += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "") + String(mfrc522.uid.uidByte[i], HEX);
    }
    uid.toUpperCase();
    if (checkUID(uid)) {
      locked = false;
      digitalWrite(RELAY_PIN, LOW);
      updateDisplay();
      delay(5000);
      digitalWrite(RELAY_PIN, HIGH);
      locked = true;
      updateDisplay();
    }
    mfrc522.PICC_HaltA();
  }
}

void updateDisplay() {
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(10, 50);
    display.print(locked ? "Locked" : "Open");
  } while (display.nextPage());
}
```

## Blockchain Integration
- **Contract**: Deploy on Base (e.g., `getAccessUIDs()` returns string array).
- **Query**: Use `web3-arduino` or manual JSON-RPC (`eth_call`) to fetch UIDs.
- **Phase 2**: Query wallet public key by UID, verify ECDSA signature with `mbedtls`.

## Troubleshooting
- **RAM Issues**: If JSON parsing fails (ESP32-C6 has 512KB), switch to ESP32-S3 (~8MB PSRAM) or Raspberry Pi.
- **SPI Conflicts**: Ensure CS multiplexing (GPIO10). Test RFID, e-ink separately.
- **Blockchain**: Use Base Sepolia for testing. Check API key, WiFi status.

## Future Work
- Implement EEPROM for UID caching.
- Add Phase 2 wallet signature verification.
- Optimize power for battery use (ESP32-C6 deep sleep).
- Port to Raspberry Pi for easier prototyping if needed.

## Contributing
Contributions welcome! Fork, create a branch, and submit a PR with clear descriptions.

## License
MIT License. See `LICENSE` for details.