# c6021light User Manual

c6021light is a project that connects devices on the following Model Railroad control networks:

* Marklin devices on CAN bus
* Marklin devices on I2C bus
* LocoNet devices

In the current version, c6021light transfers turnout commands between all three busses.
It also transfers sensor commands (e.g., from an S88 Bus or LocoNet track sensor), synchronizes the Stop/Go state (Track Power On/Off) and translated Locomotive control commands between Marklin System CAN and LocoNet.

Unfortunately, transfering locomotive control commands is not as straight-forward as transfering other commands. c6021light employs heuristics in an attempt to provide a seamless railroading experience. Nevertheless, it is in the nature of these heuristics that they may fail and that you may have to manually configure c6021light to correctly transfer locomotive control commands. See the [General Operation](GeneralOperation.md) section for more details.

*Please be aware of the following safety note: When controlling accessories such as turnouts from the Marklin Digital devices on an I2C bus, there is no reliable way to translate a "Turnout Power Off" event to other bus systems. c6021light employs a heuristic to guess which accessory output should be powered off. However, a power off command still may not be sent to the correct accessory, possibly causing permanent damage to your model railroad, e.g., by burning out a turnout actuator coil, possibly even setting the accessory on fire. Make sure to use additional, independent protective measures to limit the on-time of sensitive devices attached as an accessory to avoid any potential for damage.*

## Table of Contents

The user manual for the c6021light is split into several chapters.

**As a new user, you should begin setting up your c6021light with the [Getting Started Guide](GettingStarted.md).**

* [Getting Started Guide](GettingStarted.md) - This is the entry point for all new users. Even if you already have your c6021light running, it is recommended to at least skim this chapter as it will also introduce you to some gneral concepts of the c6021light.
* [General Operation](GeneralOperation.md) - Details the features of the c6021light as well as its operation principles.
* [Runtime Configuration & use of the Serial Console](RuntimeConfiguarionAndSerialConsole.md) - Details how to access the serial console and how to edit and persist the configuration.


