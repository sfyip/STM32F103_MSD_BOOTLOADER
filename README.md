# STM32F103_MSD_BOOTLOADER
STM32F103 Mass Storage Device Bootloader

Device: STM32F103C8T6 (Flash size: 128KB )

| Name | Address |
| --- | --- |
| Appcode start from: | 0x0800_8000 - 0x0801_9999  (96KB) |
| Bootloader start from: | 0x0800_0000 - 0x8000_7FFF (32KB)|

Simulate a USB removable disk (FAT32).
The size of firmware.bin is mapped to Appcode area, which is also 96KB.

To update the file, user must convert the iHex file to BIN file first.
Drag and drop the BIN file, named "firmware.bin", to update the appcode.

During power up, the bootloader will check the 0x0800_8000 content exists or not.
Hold PA0 (Connect to GND) during power up can force to enter bootloader mode.
