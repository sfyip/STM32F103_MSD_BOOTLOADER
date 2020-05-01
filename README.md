# STM32F103_MSD_BOOTLOADER
STM32F103 Mass Storage Device Bootloader

Device: STM32F103C8T6 (Flash size: 128KB )

My example:
| Name | Address |
| --- | --- |
| Appcode starts from: | 0x0800_8000 - 0x0801_9999  (96KB) |
| (Unused) starts from: | 0x0800_3100 - 0x0800_7FFF (20KB)|
| Bootloader starts from: | 0x0800_0000 - 0x0800_30FF (12KB)|

Simulate a USB removable disk (FAT32).
The content of firmware.bin is mapped to Appcode area. Hence, the size of firmware.bin is also 96KB.

Update Appcode Procedure:
1. Convert the iHex file to bin file
2. Drag and drop the firmware file to update the appcode.

Only tested on Windows 8.<br />
***MacOSX doesn't work properly***

During power up, the bootloader will check the content of 0x0800_8000 exists or not.
Hold PA0 (Connect to GND) during power up can force to enter bootloader mode.
