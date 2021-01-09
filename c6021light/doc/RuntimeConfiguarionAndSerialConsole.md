# Runtime Configuration

Various settings of the c6021light can be adjusted at runtime using the serial interface. To connect to the serial interface of the c6021light, follow the instructions on the project Wiki for [Connectiong to the serial console](https://github.com/deltaphi/c6021light/wiki/ConnectToSerialConsole). Make sure that you can see the command prompt `c6021light > ` before continuing.

## Interacting with the Serial Console

The c6021light has a built-in help that tells you about possible options available in your version of the software. To display the help, simply type `help` into the serial console and press Enter. On the current version of the software, the help looks as follows:

```
c6021light > help
Available commands:
  config ... - Change runtime configuration.
  flash ... - Operations on persistent storage.
  lnSlotServer ... - Control the LocoNet Refresh Slot server.
  canEngineDB ... - Control the Can Engine DB.
  help - Display this help message.
c6021light >
```

Each line consists of several elements. The first element is always a *command*, e.g., `config`. The command tells the c6021light what you want to do. The second element are three dots `...`. These dots indicate that a command cannot be executed on its own but has further subcommands. Note that the dots are optional. For example, `help` does not have subcommands. If there is a help text configured for a command, the dots are followed by a dash (`-`) and the actual help text.

To find out more about available subcommands, simply type the name of the command and press Enter. If the command is incomplete, you will automatically get another help text for the next level of subcommands. Consider the following example:

```
c6021light > config get
Available commands:
  config get ... - Display configuration value.
  config set ... - Modify configuraiton value. 
c6021light >
```

You can use this to explore the tree of commands offered by the c6021light. Let's take the example a bit further and look at `config set turnoutProtocol`:

```
Available commands:
  config set turnoutProtocol MM2
  config set turnoutProtocol DCC
  config set turnoutProtocol SX1
```

Neither of these commands have dots. That means that these are complete commands. If you were to type them in, they will immediately be executed.

Note that when you use the serial console to change a configuration value, the change takes effect immediately. However, the change will be forgotten by the next reset (e.g., power cycle of your layout) unless you [persist the configuration](#persistent-storage-of-configuration-options).

# Serial Configuration Options

This is an incomplete description of runtime configuation options. Refer to the command line help of your c6021light to find out what options your firmware version of the c6021light offers.

| Setting | Possible Values | Default Value | Description |
|---------|-----------------|---------------|-------------|
| `turnoutProtocol` | `MM2`, `DCC`, `SX1` | `MM2` | Protocol to address turnouts on the Rail. Only affects forwarding of messages on CAN. |
| `lnSlotServer` | `disabled`, `passive`, `active` | `disabled` | Forwarding of Locomotive control commands between CAN and LocoNet. Disabled completely disables forwarding of Locomotive control messages. Passive requires a LocoNet Central on LocoNet. Active (not implemented) simulates a LocoNet Central for other control terminals on LocoNet. |



# Convenience Features of the Serial Console

The serial console of the c6021light offers two convenience features: Tab-completion and command history.

## Tab Completion

When you enter a prefix command of a command known to the c6021light, press the `Tab` key to trigger the tab completion. If the prefix you entered matches exactly one command, the c6021light will auto-complete the remainder of the current command word for you. For example: ```c6021light > h<Tab>``` will complete to ```c6021light > help```. In contrast, ```c6021light > c<Tab>``` will not auto-complete, as the 'c' could match either `config` or `canEngineDB`. If you extend the command line to ```c6021light > co<Tab>```, the command will complete to ```c6021light > config```.

## Command History

If you want to execute a previous command again or want to edit a previous command, you can use arrow up and arrow down to navigate through the command history.

Note that this feature does not work with all serial terminal applicaitons. Specifically, it is known not to work with the PlatformIO IDE serial terminal.

# Persistent storage of configuration options

When you change a value using the serial console, the change takes effect immediately but is not persistet across resets of the c6021light such as caused by a power cycle. To store configuration data across resets of the boards, the c6021light utilizes the internal flash of the STM32 controller on the bluepill board.

The serial interface offers several commands to interact with the flash memory. Use `flash save` to save the current configuration persistently. The configuration will automatically be loaded every time the board is reset.

To revert the c6021light to the default settigns of the software version currently installed, use the command `flash format`. It is not usually necessary to use this command.

If you feel really curious, `flash dump` gives you a raw view into the contents of the flash pages used to store configuration data. This command is used during software development.