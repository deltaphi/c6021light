# connect6021 light

Project to equip an Arduino Uno + CANdiy shield to serve as a drop-in-replacement for a 6021
Central Unit for connecting Keyboards (only!) to CAN.

## Safety Notes

Power Off commands cannot be fully translated from I2C to CAN. There is a heuristic in place to
find the decoder address to use for a Power Off command. However, this heuristic can be tricked
and a power off command may not be sent. Please make sure to independently limit the Power On time
of accessories.

## Notes on Supply Power

The Keyboard operates on 8V. Note that its power draw is quite significant at about 120mA for a
single device.

The I2C bus operates at 5V.

## Notes on libopencm3 and PlatformIO

The current release of libopencm3 used by PlatformIO is quite outdated (from 2015, at the time of writing). Google turns up several issue reports.

The only way that has worked for me is the one outlined in [https://github.com/platformio/platform-ststm32/issues/218]. My approach on Ubuntu 20.04 was:
* Attempt a build with PlatformIO. This build wil fail but it creates all the folders needed for PlatformIO to acces libopencm3.
* Get the prerequisites for building libopencm3
* Get https://github.com/libopencm3/libopencm3-miniblink [This commit](https://github.com/libopencm3/libopencm3-miniblink/commit/2893202586a3841311c25e75329c4b549e15fba1)
 at the time of writing.
* Completely build the miniblink example.
* Copy the built libopencm3 over the one that PlatformIO uses: cp libopencm3-miniblink/libopencm3/* ~/.platformio/packages/framework-libopencm3
* Build again using PlatformIO. This build should work.

Suggestions for getting a GNU Make-based build with libopencm3 working *without* breaking the PlatformIO-based Arduino-AVR build are welcome.

### Windows-Approach

* Install a MinGW Base Toolchain. [Get it here](http://www.mingw.org/wiki/Getting_Started).
    * The installer installs a package manager.
    * The Package "mingw32-base-bin" contains everything you need (and a lot more).
* Get a git checkout of libopencm3.
* build libopencm3 from the PlatfomIO shell using the installed mingw32-make.exe.
    * The build will first generate a bunch of headers.
    * The build will fail when compiling the source code, as it cannot find the compiler. This is fine. The generated headers is all that we needed.
* Replace the PlatformIO-delivered libopencm3 with the newer sources
    * cp -r libopencm3/* ~/.platformio/packages/framework-libopencm3
* Now the build from PlatformIO works.

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

[0 0 Decoder_MSB Decoder_LSB Power Output_MSB Output_LSB Direction]

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