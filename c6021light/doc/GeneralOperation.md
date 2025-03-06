# General Operation

## LED Status Indicators

The c6021light features two status indicator LEDs: The "On-board LED" and the "Extra LED".

### On-board LED

The On-board LED can convey two types of information: Whether the system is in a regular operation state (LED on), whether a [CAN Engine Database Download](#can-engine-database) is ongoing (LED blinks slowly), or whether the system is in an error state (LED blinks fast).
There is also a mode where the LED is off, which is currently unused.

### Extra LED

Functionality of the "Extra LED" is described in the section on [System Power Status Indication](#system-power-status-indication).

## Turnout Control

Turnout contol messages are automatically forwarded between all bus systems. 

Turnout control messages on CAN carry information about the rail protocol used to contact the turnout decoder, whereas messages on I2C and LocoNet do not carry this information. Therefore, the configuration value `turnoutProtocol` is used to add protocol information to turnout control messages when forwarding to CAN. Similarly, the protocol information is stripped from messages when forwarding from CAN to other bus systems.

Devices on the I2C bus send out turnout control request messages and expect turnout control response messages in return, before they update their state indication (Keyboard) or proceed with processing (Memory). In the current version of the software, turnout control response message are generated only by a central device on the CAN bus that is not in the "STOP" mode. Devices on LocoNet do not generate turnout control response messages.

## Sensor Messages

Sensor messages are forwarded as-is between CAN and LocoNet in both directions. No configuration is required or possible.

## System Power Status (Stop/Go)

Stop and Go messages are forwarded as-is between CAN and LocoNet in both directions. No configuration is required or possible.

In addition, c6021light provides an indication of the last observed system state using the "Extra LED".

### System Power Status Indication

The "Extra LED" indicates whether the c6021light considers the system to be in a "Stop" state (LED off) or a "Go" state (LED on). In addition, the c6021light will flash the "Extra LED" when it has no information about the current Stop/Go state of the system.

The c6021light considers Stop/Go commands from all bus systems equally. Whenever a STOP- or GO-Command is received, it updates its internal state accordingly and sets the state of the LED. Not that the Stop/Go state of the system may be out of sync if any component of the system is reset or disconnected.

### System Power Status Download

After power on or reset, the c6021light has no information about the Stop/Go-State of the system and will start to flash the Extra LED. In parallel, it will send request messages to determine the Stop/Go state on the CAN bus. Unfortunately, not all CAN-based railroad control systems respond to this message. In this case, the c6021light cannot determine the Stop/Go state of the system until the first STOP or GO command is observed on any bus.

Lack of the current system state does not influence the forwarding of STOP/GO commands between bus systems.


## Locomotive Control

*Note that Locomotive Control is an experimental feature that is currently under development. It is not fully available between all bus systems and in all directions. Even if it is available, it may spuriously fail.*

### Overview over Locomotive Control

In contrast to all other types of information forwarded by c6021light, Locomotive Control is a more complex task. Turnout control, track sensors and Stop/Go messages work similarly on all bus systems. Therefore, messages can be forwarded by simply translating message formats between the bus systems. 

Locomotive Control, however, useses widely different message types and even message interactions and addressing schemes across the different bus networks. c6021light is capable of translating the message interactions for Locomotive Control between LocoNet and Marklin CAN, provided that it has state information about the engine control happening on both bus systems.

On LocoNet, Locomotives are assigned a control slot by the LocoNet Master. The Slot is associated with a Locomotive Address (e.g., the DCC decoder address sent on the track), but all control messages contain only the slot number.

On CAN, Locomotives are assigned a unique ID (UID). The UID is associated with the Locomotive Address and the track protocol for the Locomotive Address, but all control messages contain only the UID.

To successfully forward Locomotive Control Commands between CAN and LocoNet, c6021light therefore must know the CAN Engine Database as well as the LocoNet Slot Database.

Note that CAN considers both the Locomotive Address as well as the track protocol used for the respective address, whereas LocoNet uses only the Locomotive Address. It is (at least in theory) possible to control several Locomotives that use the same address on different Protocols (e.g., one Locomotive using MM2-50 and another using DCC-50) from CAN. LocoNet would not be able to distinguish control commands for these engines. It is therefore recommended that you give unique Locomotive Addresses to all of your Locomotives, independent of the rail protocol they use.


### Locomotive Control using c6021light

Forwarding of Locomotive control message is only enabled, when the configuration setting `lnSlotServer` is not set to `disabled`.
Furthermore, a CAN Engine DB must be available for the c6021light to be able to forward incoming Locomotive Control Messages.

#### LocoNet Slot Server

The LocoNet Slot Server offers two operating modes: `passive` mode and `active` mode.

In `passive` mode, the c6021light expects a LocoNet Master to be present on the bus, e.g., an Uhlenbrock IntelliBox. The LocoNet Master controls the assignment of Engines to Slots. In `passive` mode, the c6021light monitors the LocoNet bus for ongoing slot assignments and Locomotive Control Commands and caches all received information in its internal Slot Server.

In `active` mode, the c6021light acts as a LocoNet Master itself. Thus, it controls the assignment of Locomotives to slots both as other devices on LocoNet request Locomotive control slots and as control messages from CAN are forwarded for Locomotives currently not assigned a slot.

In the current implementation, engine speed, direction and Functions F0-F31 are forwarded from LocoNet to CAN. If no additional measures are taken, c6021light produces messages to an engine using the address entered on the control device on LocoNet, using the MM2 protocol.

#### CAN Engine Database

The c6021light can also obtain the list of known engines from a central device on the CAN bus using the command `canEngineDB downloadEngines`.
An ongoing download is indicated by flashing the "On-board LED".

Note that the download time is proportional to the number of engines in your database.
It will typically take several seconds to complete.
You can at any time view the list of engines known to the c6021light using the command `canEngineDB dump` on the serial command line.

To avoid having to manually download the CAN Engine Database every time the c6021light is reset or powered down, the c6021light will try to download the CAN Engine Database after power-up. Over the first ten seconds of its operation, it will request a download of the CAN Engine Database once every second.

#### Forwarding between CAN and LocoNet

If an Engine Database is available on the c6021light, it will try to find an engine with an address that matches the address entered on the LocoNet-side. If a match is found, the c6021light will forward messages for that engine.

In the reverse direction, i.e., CAN to LocoNet, no information is transmitted unless an engine database is available. If an engine database is available, the c6021light will attempt to control engines on the LocoNet side when a control message is received on the CAN side. Control is currently limited to setting the engine speed. Engine direction or functions are not currently supported. 
