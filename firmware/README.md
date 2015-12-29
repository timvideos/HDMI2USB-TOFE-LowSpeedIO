This firmware is for the PIC18F14K50 on the TOFE Expansion board. It provides
the following features;

 * CDC-ACM serial UART interface to the FPGA
 * CDC-ACM serial UART interface to the PIC
 * Emulation of a 24 series EEPROM for TOFE ID Detection
 * Access to the ADC data.

# Building

Currently this requires the following tools to build;
 * [Microchip MPLAB.X](http://microchip.com/mplabx) and
 * [C18 Compiler](http://www.microchip.com/c18)

See the [NOTES.md](NOTES.md) file for information on setting up the C18 compiler.

## Bootloader

The PIC is loaded with the Microchip HID USB Bootloader from the "Microchip
Libraries for Application" (MLA) to allow in the field upgrades of the
firmware.

The [mphidflash](https://code.google.com/p/mphidflash/) tool can be used to
load the new firmware onto the board.

## License

This firmware is based on the echo demo from the
[Dangerous Prototype's USB Stack](https://github.com/DangerousPrototypes/USB_stack).

This firmware code (like the original code) is released under a 
[CC-BY 3.0 Unported](http://creativecommons.org/licenses/by/3.0/) 

A copy of this license should be found in the [LICENSE.md](LICENSE.md) file
which accompanies this code. Otherwise, to view a copy of this license, visit
http://creativecommons.org/licenses/by/3.0/ or send a letter to

        Creative Commons,
        171 Second Street,
        Suite 300,
        San Francisco,
        California,
        94105,
        USA.

