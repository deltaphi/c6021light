# General Operation

## Turnout Control

Turnout contol messages are automatically forwarded between all bus systems. 

Turnout control messages on CAN carry information about the rail protocol used to contact the turnout decoder, whereas messages on I2C and LocoNet do not carry this information. Therefore, the configuration value `turnoutProtocol` is used to add protocol information to turnout control messages when forwarding to CAN. Similarly, the protocol information is stripped from messages when forwarding from CAN to other bus systems.

Devices on the I2C bus send out turnout control request messages and expect turnout control response messages in return, before they update their state indication (Keyboard) or proceed with processing (Memory). In the current version of the software, turnout control response message are generated only by a central device on the CAN bus that is not in the "STOP" mode. Devices on LocoNet do not generate turnout control response messages.

## Sensor Messages

Sensor messages are forwarded as-is between CAN and LocoNet in both directions. No configuration is required or possible.

## System Power Status (Stop/Go)

Stop and Go messages are forwarded as-is between CAN and LocoNet in both directions. No configuration is required or possible.

## Locomotive Control

*Note that Locomotive Control is an experimental feature that is currently under development. It is not fully available between all bus systems and in all directions. Even if it is available, it may spuriously fail.*

Forwarding of Locomotive control message is only enabled, when the configuration setting `lnSlotServer` is not set to `disabled`. However, there is currently only a partial implementation of  `passive` mode and no implementation of `active` mode.

In the current implementation of `passive` mode, engine speed, direction and Functions F0-F8 are forwarded from LocoNet to CAN. If no additional measures are taken, c6021light produces messages to an engine using the address entered on the control device on LocoNet, using the MM2 protocol.

The c6021light can also obtain the list of known engines from a central device on the CAN bus using the command `canEngineDB downloadEngines`. Note that the download will take several seconds to complete. You can at any time view the list of engines known to the c6021light using the command `canEngineDB dump`.

If an Engine Database is available on the c6021light, it will try to find an engine with an address that matches the address entered o n the LocoNet-side. If a match is found, the c6021light will forward messages for that engine.

In the reverse direction, i.e., CAN to LocoNet, no information is transmitted unless an engine database is available. If an engine database is available, the c6021light will attempt to control engines on the LocoNet side when a control message is received on the CAN side. Control is currently limited to setting the engine speed. Engine direction or functions are not currently supported. 
