# FatFs example program for the Cyclone V SoCFPGA on the DE10-Nano development board

## Overview

A standalone C program demonstrating use of the FatFs module library to write and read a plain text file with a SD card containing a FAT/exFAT filesystem.

## Run requirements

- You will need an SD card containing a FAT/exFAT partition
- If the SD card is bootable, don't insert it just yet, apply power to the DE10-Nano, start the debugging in Eclipse (which runs U-Boot SPL with SD/MMC controller disabled), then insert the card

## Build requirements

Minimum to build the C sources:
- GNU C/C++ cross toolchain for ARM processors supporting Cortex-A9
- newlib library.  This is usually included with the above toolchain

### Using my script to build

To build the C sources using the script under Windows, you will need a Windows port of GNU make with some shell facilities, a good solution is xPack's Build Tools.

### Using "Eclipse IDE for Embedded C/C++ Developers" to build

To build the C sources using Eclipse, you can open with the included project file for Eclipse IDE, but you may need to setup other things first as described in my guide.

### Building the SD card image and U-Boot sources

To build these under Windows you will need to use WSL2 or Linux under a VM.  See the makefile or my guide for more information.

You can find the guide on my website:
[https://truhy.co.uk](https://truhy.co.uk)

## Limitations

- I did not override the portable get_fattime() function because the Cyclone V SoCFPGA does not have an RTC, instead we can use the default function with a constant date.  To do we edit the FatFs ffconf.h file with the parameter: FF_FS_NORTC = 1 and a constant date
- Only the char type string is supported;	it is very difficult to support wchar_t string, UNICODE string, etc.

## Libraries used in the source code

- [FatFs module library](http://elm-chan.org/fsw/ff)
- [Intel SoC FPGA HWLIB](https://github.com/altera-opensource/intel-socfpga-hwlib)
