# STM32F103_MSD_BOOTLOADER
STM32F103 Mass Storage Device Bootloader

Device: STM32F103C8T6 (Flash size: 128KB )

| Name | Address |
| --- | --- |
| Appcode start from: | 0x0800_8000 - 0x0802_0000  (96KB) |
| Bootloader start from: | 0x0800_0000 - 0x8000_7FFF (32KB)|

Simulate a USB removable disk (FAT32).
The size of firmware.bin is also 96KB which is mapped to Appcode area.

To update the file, user must convert the iHex file to BIN file first.
Drag and drop the BIN file, named "firmware.bin", to update the appcode.
