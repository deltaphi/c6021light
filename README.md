connect6021 light für Keyboard 6040
===================================

![PlatformIO Bluepill Environment](https://github.com/deltaphi/c6021light/workflows/PlatformIO%20Bluepill%20Environment/badge.svg?branch=master&event=push)

Project to connect one or several Keyboard 6040s to CAN. This project replaces the 6021 in your system.

## Safety Notes

When a button on the Keyboard is released, the Keyboard generates a "Power Off" command.
Unfortunately, the command does not directly translate to CAN. On CAN, commands are always sent
to the address of a specific accessory. The Keyboard 6040 however addresses 8 accessories at once.
There is a heuristic in the software to guess the address of the accessory to turn off. However,
this heuristic can be tricked and a power off command may not be sent to the correct address.

Please make sure to independently limit the Power On time of all accessories.

## Notes on Supply Power

The Keyboard operates on 8V. Note that its power draw is quite significant at about 120mA for a
single device.

The I2C bus operates at 5V.

# Hardware

Belegung der Buchse (Keyboard 6040 links)
```
SDA  SCL                      8V   8V
 2b   4b   6b   8b  10b  12b  14b  16b
 2a   4a   6a   8a  10a  12a  14a  16a
GND  GND  GND  GND  GND  GND  GND  GND
```

Federleiste DIN 41612 32-pol B/2
z.B. ept 102-90065


http://dl1zax.selfhost.de/ATMega/PDF/6022.pdf

# I2C Bus Communication

Zitat von [Dr. König's Märklin-Digital-Page](http://www.drkoenig.de/digital/i2c.htm):
```
Für die Übertragung von Weichenstellbefehlen genügen drei Byte als Schaltanforderung.
Es sind dies die Bytes für Empfänger (Zentrale), Keyboard-Nr. (0 bis 15) und das Datenbyte.
Das Datenbyte enthält die Angabe der Decoder-Ausgangsnummer (Bits 0 bis 3) und der Decoder- Teiladresse (Bits 4 und 5).
Mit Bit 0 wird die Schaltrichtung bestimmt (0=Rot, 1=Grün), mit Bit 3 wird der Schaltstrom eingeschaltet.
Da mit einem Keyboard (6040) vier Decoder angesprochen werden können, bestimmen die Bits 4 und 5, um welche Vierergruppe von Tastenpaaren und somit für welchen der vier für dieses Keyboard zuständigen Decoder sich der Schaltbefehl handelt.
Bei der Bildung der realen Decoder-Adresse (im binären Zahlenformat) stellen diese zwei Bits die Bits 0 und 1 dar. Die vier Bits der Keyboard-Adresse bilden dann die Bits 2 bis 5 der Decoder-Adresse. Diese Decoder-Adresse wird noch um 1 erhöht, da die Decoder-Adressierung im Märklin Digital-System immer mit der Adresse "01" beginnt. Somit können maximal 64 Decoder im Bereich von "01" bis "64" angesprochen werden.
Aufbau des Datenbytes:

    Bit 0 = Schaltrichtung, 0=Rot, 1=Grün
    Bit 1 = Decoderausgang LSB (Least Significant Bit)
    Bit 2 = Decoderausgang MSB (Most Significant Bit)
    Bit 3 = Schaltstrom, 0=aus, 1=ein
    Bit 4 = Decoder-Teiladresse LSB
    Bit 5 = Decoder-Teiladresse MSB 

Aufbau der Schaltanforderung:

Empfänger		Sender			Datenbyte		Stoppbits
1111 1110	0	001X XXX0	0	00XX XXXX	0	10
Zentrale	Q	Geräte-Adr.	Q			Q

Die Bestätigung von der Zentrale lautet:

Empfänger		Sender			Datenbyte		Stoppbit
001X XXX0	0	1111 1110	0	00XX XXXX	0	0
Geräte-Adr.	Q	Zentrale	Q			Q

```

Sequenz

Taste 0 rot

0xfe 0x20 0x08

0x20 0xfe 0x08 (Osci wA 0x10 -> Data 0x20)

## I2C Messages

Devices communicate over I2C. The system is multi-master, i.e., any device that has data to send
as as a master. Read from slave devices is not used.

The message format for Keyboards (Accessory commands) is as follows:

receiver sender data

* receiver is the I2C slave address the message is sent to. Thisi s not sent as a separate data
  byte but as the address byte of the I2C communication.
* sender is the I2C slave address under which the originator of the message can be reached.
* data The data byte, containing the action to be performed.

### I2C Device Addresses

Note that I2C addresses are 7 Bit. However, the above mentioned fields are all 8 bit. In practice,
this results in the fact that the receiver address and sender address are shifted by 1 bit. In the
following, all addresses are given as their I2C slave address repesentation (i.e., the value
observed in the receiver field).

* The Central Unit uses the slave address 0x7F.
* Keyboard 0 uses a slave address of 0x10. The Keyboard address is derived from the DIP switches on
  the back.

### I2C Data Byte

The data byte is constructed as follows:

`[0 0 Decoder_MSB Decoder_LSB Power Output_MSB Output_LSB Direction]`

Addressing on decoders on I2C is centered around the original approach, where a decoder had one
decoder address (set with DIP switches) and eight outputs. The KEyboard Address together with
the Decoder_* bits identify a decoder. The Output_* bits together with the Direction bit identify
one output of the decoder.

Modern Systems use individually addressable decoders (Keyboard Address + Decoder_* + Output_*) and
treat the direction separately.

## Keyboard Message flow

The Keyboard does not need any initialization. As soon as 8V are applied to one of the power pins
and the I2C lines are pulled up to 5V (a 10k resistor on each line does the job) it starts up and
the turnout indicators set at the last shutdown light up.
 
When a button is pressed, a message is sent to the Central Unit. When no Device with the address
of the Central Unit is present on the bus, the `receiver` byte will not be Acknowledged. In this
case, the Keyboard will abort transmission of the remaining bytes. It will then try resend the
message immediately and indefinitely, possibly blocking the bus.

If the message is received (i.e., a device with the address of the Central Unit is present on the
bus and ACKs the bytes), the Keyboard expects a response. If the response is not received, it
resends its original request indefinitely about once per second.

If no response is received, the indicators on the Keyboard don't change. Also, pressing additional
buttons has no effect. The Keyboard is stuck processing the original request. If a response is
received even when no corresponding request was sent, the indicators always change and no further
processing takes place on the Keyboard.

### Power On/Off

When a button is pressed, a "Power ON" message is sent. When a button is released, a "Power OFF"
message is sent. However, due to the Decoder-Based addressing scheme, the "Power Off" message is
not addressed towards a specific turnout but towards the decoder Address, i.e., all 8 outputs are
considered to be unpowered.