# STM32F103_MSD_BOOTLOADER
STM32F103 Mass Storage Device Bootloader

Device: STM32F103C8T6 (Flash size: 128KB )

My example:
| Name | Address |
| --- | --- |
| Appcode starts from: | 0x0800_4000 - 0x0801_FFFF  (112KB) |
| (Unused) starts from: | 0x0800_3170 - 0x0800_3FFF (3.64KB)|
| Bootloader starts from: | 0x0800_0000 - 0x0800_316F (12.36KB)|

Simulate a USB removable disk (FAT32).
The content of firmware.bin is mapped to Appcode area. Hence, the size of firmware.bin is also 96KB.

Update Appcode Procedure:
1. Drag and drop the iHEX file to update the appcode

Only tested on Windows 8 and MacOS Sierra<br />

During power up, the bootloader will check the content of 0x0800_4000 exists or not.
Hold PA0 (Connect to GND) during power up can force to enter bootloader mode.
