# PIC18F USB Stacks

There are multiple options for a USB stack on the PIC18F series chips.

For this board, we specifically need something which works with the PIC
18F14K50 IC. 


 * [m-stack](http://www.signal11.us/oss/m-stack/)
   * Pros
     * Good documentation.
     * Clean code.
     * Lots of examples
   * Cons
     * Currently no support for 18F14K50.
     * Seems to use *a lot* of resources.
     * Microchip xc8 compiler
   * Notes
     * Includes bootloader?
     * Apache 2.0 or LGPLv3 license

 * [Dangerous Prototypes JTR-Honken](https://github.com/DangerousPrototypes/USB_stack)
   * Pros
     * Working examples
     * Some documentation
     * PIC 18F14K50 support
     * Smallish
   * Cons
     * Code isn't high quality
     * Not really in library form
     * Microchip c18 compiler (EOL, no Linux support)
   * Notes
     * CC-BY License
     * [How To 1](http://dangerousprototypes.com/2012/06/06/open-source-usb-echo-demo-wiki/)

 * [Microchip mla_usb](https://github.com/MicrochipTech/mla_usb)
   * Pros
     * Created by chip manufacturer.
     * PIC 18F14K50 support
   * Cons
     * Examples are not under FOSS license
   * Notes
     * Apache 2.0 License

 * [SpareTimeLabs Minimal USB CDC-ACM](http://www.sparetimelabs.com/usbcdcacm/usbcdcacm.php)
   * Pros
     * Uses SDCC!
     * Seems pretty small.
   * Cons
     * Only supports the PIC18F4550
     * Source code in tar.gz!?

As you should be able to see from the Git history, I ended up going with
heavily customizing the Dangerous Prototypes JTR-Honken stack. I'm not entirely
happy with the code quality of this library, or having to use the C18 compiler
but it does seem to work.

# Using Microchip C18 Compiler on Linux

 * Install the i386 version of the Java runtime with;
   `apt-get install openjdk-7-jre:i386`
 * Install [MPLAB.X from Microchip](http://www.microchip.com/pagehandler/en-us/family/mplabx/)
 * Download [MPLAB C18 Linux Full Installer](https://github.com/Manouchehri/Microchip-C18-Lite/releases/download/v3.47/mplabc18-v3.47-linux-lite-installer.run)
 * Install with `sudo ./mplabc18-v3.40-linux-full-installer.run` (You may need
   `--mode text` otherwise the installer will just segfault on new Ubuntu
   versions.)

