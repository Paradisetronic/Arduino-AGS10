/*
  Paradisetronic - Beispielcode
  --------------------------------------------
  
  
  Verdrahtung (Arduino UNO / Nano):
    VCC -> 3.3V
    GND -> GND
    SDA -> A4
    SCL -> A5
  
  Hinweis:
    - Verwenden Sie Pull-up-Widerstände (2–10 kΩ) auf SDA und SCL, 
      falls Ihr Modul keine eingebauten hat.
    - Der Sensor benötigt mindestens 120 Sekunden Vorheizzeit.
    - Zwischen zwei Messungen mindestens 1,5 Sekunden warten.
*/

#include <Wire.h>  // Bibliothek für I²C-Kommunikation
//  I²C-Adresse und Register laut Datenblatt 
#define AGS10_ADDR 0x1A               // Standardadresse (7-bit)
#define REG_DATA_ACQ        0x00      // TVOC-Messwert lesen
#define REG_READ_VERSION    0x11      // Firmware-Version lesen
#define REG_READ_RESISTANCE 0x20      // Sensorwiderstand lesen
#define CMD_DELAY_MS        30        // Mindestwartezeit zwischen Befehlen
#define READ_DELAY_MS       1500      // Mindestwartezeit zwischen Messungen


// CRC8-Prüfsumme (Polynom 0x31, Initialwert 0xFF)
// Diese Funktion überprüft, ob die empfangenen Daten gültig sind.
uint8_t calcCRC8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0xFF;
  for (uint8_t b = 0; b < len; b++) {
    crc ^= data[b];
    for (uint8_t i = 0; i < 8; i++)
      crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
  }
  return crc;
}

// Liest 5 Bytes vom Sensor (4 Datenbytes + 1 CRC-Byte)
// Gibt "true" zurück, wenn die Daten korrekt sind.

bool i2cRead5(uint8_t reg, uint8_t *out) {
  Wire.beginTransmission(AGS10_ADDR);
  Wire.write(reg);                         // Gewünschtes Register senden
  if (Wire.endTransmission() != 0) return false;
  delay(CMD_DELAY_MS);

  if (Wire.requestFrom((int)AGS10_ADDR, 5) != 5) return false;
  for (uint8_t i = 0; i < 5; i++) out[i] = Wire.read();

  // CRC-Überprüfung
  return (calcCRC8(out, 4) == out[4]);
}
// Liest den aktuellen TVOC-Wert (in ppb).
// "status" enthält Statusinformationen (z.B. RDY-Bit).

bool readTVOC(uint32_t &tvoc, uint8_t &status) {
  uint8_t buf[5];
  if (!i2cRead5(REG_DATA_ACQ, buf)) return false;

  status = buf[0];  // Status-Byte speichern
  tvoc = ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
  delay(READ_DELAY_MS);
  return true;
}


// Liest die Firmware-Version des Sensors.
bool readVersion(uint8_t &ver) {
  uint8_t buf[5];
  if (!i2cRead5(REG_READ_VERSION, buf)) return false;
  ver = buf[3];
  return true;
}


// Liest den aktuellen Sensorwiderstand (in 0.1 kΩ-Einheiten).
bool readResistance(uint32_t &res) {
  uint8_t buf[5];
  if (!i2cRead5(REG_READ_RESISTANCE, buf)) return false;
  res = ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
  return true;
}
// Initialisierung: Serielle Schnittstelle & I²C starten,
// Sensorinformationen abrufen und erste Werte anzeigen.
void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial.println(F("Paradisetronic - AGS10 TVOC Sensor Test"));
  Serial.println(F("Bitte mindestens 2 Minuten Vorheizzeit beachten..."));
  delay(2000);

  // Firmware-Version lesen
  uint8_t fw;
  if (readVersion(fw)) {
    Serial.print(F("Firmware-Version: 0x"));
    Serial.println(fw, HEX);
  } else {
    Serial.println(F("Fehler: Firmware-Version konnte nicht gelesen werden."));
  }

  // Aktuellen Sensorwiderstand lesen
  uint32_t r;
  if (readResistance(r)) {
    Serial.print(F("Sensor-Widerstand: "));
    Serial.print(r / 10.0, 1);  // Umrechnung in kΩ
    Serial.println(F(" kΩ"));
  } else {
    Serial.println(F("Fehler: Widerstand konnte nicht gelesen werden."));
  }

  Serial.println(F("Starte TVOC-Messung..."));
}
// Hauptschleife: Liest regelmäßig den TVOC-Wert und zeigt ihn an.
void loop() {
  uint32_t tvoc;
  uint8_t status;

  if (readTVOC(tvoc, status)) {
    Serial.print(F("Status: 0x"));
    Serial.print(status, HEX);
    Serial.print(F(" | TVOC: "));
    Serial.print(tvoc);
    Serial.println(F(" ppb"));
  } else {
    Serial.println(F("Fehler beim Lesen der TVOC-Daten."));
  }

  // Messintervall (alle 2 Sekunden)
  delay(2000);
}
