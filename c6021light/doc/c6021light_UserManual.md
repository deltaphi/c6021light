# c6021light User Manual

c6021light is a project that connects devices on the following Model Railroad control networks:

* Marklin devices on CAN bus
* Marklin devices on I2C bus
* LocoNet devices

In the current version, c6021light transfers turnout commands between all three busses.
It also transfers sensor commands (e.g., from an S88 Bus or LocoNet track sensor) and syncrhonizes the Stop/Go state (Track Power On/Off) between Marklin System CAN and LocoNet.

*Please be aware of the following safety note: When controlling turnouts from the Marklin Digital devices on an I2C bus, there is no reliable way to translate a "Turnout Power Off" event to other bus systems. c6021light employs a heuristic to guess which turnout output should be powered off. However, a power off command still may not be sent to the correct tunrout, possibly causing permanent damage, e.g., by burning out a turnout actuator coil. Make sure to use additional measures to limit the on-time of sensitive devices attached as a turnout.*

## Table of Contents

The user manual for the c6021light is split into several chapters.

**As a new user, you should begin setting up your c6021light with the [Getting Started Guide](GettingStarted.md).**

* [Getting Started Guide](GettingStarted.md) - This is the entry point for all new users. Even if you already have your c6021light running, it is recommended to at least skim this chapter as it will also introduce you to some gneral concepts of the c6021light.
* [General Operation](GeneralOperation.md) - Details the features of the c6021light as well as its operation principles.
* [Runtime Configuration & use of the Serial Console](RuntimeConfiguarionAndSerialConsole.md) - Details how to access the serial console and how to edit and persist the configuration.


