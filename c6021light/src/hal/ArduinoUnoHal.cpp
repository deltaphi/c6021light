#include <CAN.h>
#include <Wire.h>
#include <hal/ArduinoUnoHal.h>
#include "RR32Can/RR32Can.h"

namespace hal {

ArduinoUnoHal::I2CBuf ArduinoUnoHal::i2cRxBuf;

/**
 * \brief Callback when a message is received.
 */
void ArduinoUnoHal::receiveEvent(int howMany) {
  if (howMany != 2) {
    Serial.println(F("I2C message with incorrect number of bytes."));
    Wire.flush();
  }

  if (!ArduinoUnoHal::i2cRxBuf.msgValid) {
    for (uint8_t i = 0; i < howMany && i < kMsgBytesLength; ++i) {
      // Bytewise-copy because Wire cannot read to volatile memory.
      i2cRxBuf.msgBytes[i] = Wire.read();
    }
    ArduinoUnoHal::i2cRxBuf.msgValid = true;
  } else {
    Serial.println(F("I2C RX Buffer full, lost message."));
  }
}

void ArduinoUnoHal::beginI2c() {
  i2cRxBuf.msgValid = false;
  Wire.begin(i2cLocalAddr);
  Wire.onReceive(ArduinoUnoHal::receiveEvent);
}

void ArduinoUnoHal::beginCan() {
  Serial.print(F("Init Can: "));
  if (!CAN.begin(250E3)) {
    Serial.println(F(" failed."));
    while (1)
      ;
  } else {
    Serial.println(F(" success."));
  }
}

void ArduinoUnoHal::loopCan() {
  int packetSize = CAN.parsePacket();

  if (packetSize > 0) {
    // received a packet
    Serial.print(F("Received "));

    if (CAN.packetExtended()) {
      Serial.print(F("extended "));
    }

    if (CAN.packetRtr()) {
      // Remote transmission request, packet contains no data
      Serial.print(F("RTR "));
    }

    Serial.print(F("packet with id 0x"));
    long packetId = CAN.packetId();
    Serial.print(packetId, HEX);
    RR32Can::Identifier rr32id = RR32Can::Identifier::GetIdentifier(packetId);
    RR32Can::Data data;
    data.dlc = packetSize;

    if (CAN.packetRtr()) {
      Serial.print(F(" and requested length "));
      Serial.println(CAN.packetDlc());
    } else {
      Serial.print(F(" and length "));
      Serial.println(packetSize);

      // only print packet data for non-RTR packets
      uint8_t i = 0;
      while (CAN.available()) {
        data.data[i] = CAN.read();
        Serial.print(' ');
        Serial.print(data.data[i], HEX);
        ++i;
      }
      Serial.println();
      RR32Can::RR32Can.HandlePacket(rr32id, data);
    }

    Serial.println();
  }
}

void ArduinoUnoHal::SendPacket(const RR32Can::Identifier& id, const RR32Can::Data& data) {
  CAN.beginExtendedPacket(id.makeIdentifier(), data.dlc, false);
  for (uint8_t i = 0; i < data.dlc; ++i) {
    CAN.write(data.data[i]);
  }
  CAN.endPacket();
}

void ArduinoUnoHal::SendI2CMessage(const MarklinI2C::Messages::AccessoryMsg& msg) {
  Wire.beginTransmission(msg.destination);
  Wire.write(msg.source);
  Wire.write(msg.data);
  Wire.endTransmission();
  Serial.println(F("I2C message sent."));
}

MarklinI2C::Messages::AccessoryMsg ArduinoUnoHal::getI2cMessage() const {
  MarklinI2C::Messages::AccessoryMsg msg;
  msg.destination = i2cLocalAddr;
  msg.source = i2cRxBuf.msgBytes[0];
  msg.data = i2cRxBuf.msgBytes[1];
  i2cRxBuf.msgValid = false;
  return msg;
}

void ArduinoUnoHal::led(bool on) {
  if (on) {
    digitalWrite(13, LOW);
  } else {
    digitalWrite(13, HIGH);
  }
}

void ArduinoUnoHal::toggleLed() { digitalWrite(13, !digitalRead(13)); }

}  // namespace hal