## c8 emulator and debugger

### What is c8?
c8 is an emulator and debugger for the [chip8 system](https://en.wikipedia.org/wiki/CHIP-8).

The emulator also comes with a debugger and disassembler to more easily debug chip8 programs.

### Installation
Build the cmake project or download the windows binaries.

### Usage
```c8 <rom file>```

Some roms require a different clock speed. c8 refreshes the display at 60 fps. To specify the number of instructions per clock use the ```--clocks=<clocks>``` switch.

```c8 <rom file> --clocks=6```

This sets the emulator to run 6 instructions per frame.


To open the debugger with a rom the ```--debug``` switch can be used. To break the program on launch use the ```--break``` in conjunction with debug mode.

![alt text](https://github.com/08jne01/chip8/blob/master/example.png)

From the debug mode there are several commands:
| Key | Command |
|-----|----------|
| F10 | step over |
| F11 | step into |
| PAUSE | break/continue |
| PAGEUP | scroll up disassembly |
| PAGEDOWN | scroll down disassembly |
| ARROW UP | scroll up memory |
| ARROW down | scroll down memory |
| END | toggle command input |

Toggling command input allows the user to enter commands. Currently there are only two commands

```addbreak <code address in hex>```

```delbreak <code address in hex>```

which add and remove break points respectively.
