# STM32F103_MSD_BOOTLOADER
STM32F103 Mass Storage Device Bootloader

Device: STM32F103C8T6 (Flash size: 128KB )

My example:
| Name | Address |
| --- | --- |
| Appcode starts from: | 0x0800_4000 - 0x0801_FFFF  (112KB) |
| (Unused) starts from: | 0x0800_36E0 - 0x0800_3FFF (2.28KB)|
| Bootloader starts from: | 0x0800_0000 - 0x0800_36DF (13.72KB)|

Simulate a USB removable disk (FAT32).
The content of firmware.bin is mapped to Appcode area. Hence, the size of firmware.bin is also 112KB.

Just drag and drop the intel hex file to update the appcode. The bootloader will automatically restart if EOF RecordType is found in the hex file.
<b>Drag and drop the BIN file is removed.</b> Since intel hex file has the following advantages over the bin file:
1. Most of the compiler tools support direct export to intel hex file. No need to covert to bin file anymore
2. provides better integrity check
3. macOS works properly because of unique format in ihex file

During power up, the bootloader will check the content of 0x0800_4000 exists or not.
Hold PA0 (Connect to GND) during power up can force to enter bootloader mode.

#### Only tested on Windows 8, MacOS Sierra and Ubuntu 18.04<br />

Several examples are provided in "example-hex" folder to validate the bootloader feature.

#### Test Procedure:
1. Hold PA0 during power up to enter bootloader mode.
2. A removable disk drive named "BOOTLOADER" is recognized.
3. Ensure PA0 is released.
4. Drag and drop STM32F103_FlashPC13LED_FAST.hex to the removable disk.
5. Since the intel hex file contains EOF record type, the bootloader will be self-reset.
6. The bootloader jumps to Appcode. LED (PC13) blinks. 

#### Added AES256-CTR Encryption:
For more detail, please see the hex-crypt folder.
