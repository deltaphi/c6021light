# Runtime Configuration

Various settings of the c6021light can be adjusted at runtime using the serial interface. To connect to the serial interface of the c6021light, simply hook up a USB-to-Serial adapter *capable of operating at 3.3V* to J52 on the c6021light. A USB-to-Serial adapter operating at a higher voltage may cause permanent damage to the c6021light or to other devices of your railroad layout!

*Note [Issue #7](https://github.com/deltaphi/c6021light/issues/7): On some versions of the c6021light PCB, the RX and TX lines on J52 are swapped. If your PCB is affected by this bug, use jumper cables to cross over RX and TX.*

Populating the on-board USB-to-Serial converter will also let you access the serial console, but is a somewhat difficult soldering task.

## Connecting to the Serial Console

Once you have connected a USB-to-Serial converter to the c6021light and to your computer, you can use a terminal program to open the serial port and talk to the c6021light. The connection runs at `115200 8N1`, i.e., 115200 Baud, 8 Bits, no Parity, 1 Stop bit. If you have the PlatformIO IDE installed, e.g., to be able to update the software of the c6021light, the PlatformIO IDE has a serial terminal built in. With the PlatformIO IDE open on the c6021light software project, simply press the "upwards-facing plug" icon in the bottom left of the window. This will start a serial terminal to the c6021light. While this terminal works perfectly well to edit the configuration of the c6021light, note that it does not offer acces to all convenience features of the command line interface, such as the command history.

If you do not have the PlatformIO IDE installed or want a more feature-complete serial terminal experience, [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/) has been tested to work well.

If the serial terminal opens successfully (regardless of the program), you will likely see a screen that is empty, aside from status messages of your serial program. Right after connecting from the PlatformIO IDE, the screen appears as follows:

```
> Executing task in folder c6021light: C:\Users\Damian\.platformio\penv\Scripts\pio.exe device monitor <

--- Available filters and text transformations: colorize, debug, default, direct, hexlify, log2file, nocontrol, printable, send_on_enter, time
--- More details at http://bit.ly/pio-monitor-filters
--- Miniterm on COM4  115200,8,N,1 ---
--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---


```

To bring up the command prompt, press `Enter` on your computers' keyboard. You will be greeted as follows:

```
> Executing task in folder c6021light: C:\Users\Damian\.platformio\penv\Scripts\pio.exe device monitor <

--- Available filters and text transformations: colorize, debug, default, direct, hexlify, log2file, nocontrol, printable, send_on_enter, time
--- More details at http://bit.ly/pio-monitor-filters
--- Miniterm on COM4  115200,8,N,1 ---
--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
c6021light > 
```

`c6021light > ` is the command prompt that indicates that the c6021light now accepts your serial commands.

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