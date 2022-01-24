# STM32F103_MSD_BOOTLOADER
STM32F103 Mass Storage Device Bootloader

Device: STM32F103C8T6 (Flash size: 64KB) / STM32F103CBT6 (Flash size: 128KB)

My example:
| Name | STM32F103C8T6 Address | STM32F103CBT6 Address |
| --- | --- | --- |
| Appcode starts from: | 0x0800_4000 - 0x0800_FFFF  (48KB) | 0x0800_4000 - 0x0801_FFFF  (112KB) |
| (Unused) starts from: | 0x0800_36E0 - 0x0800_3FFF (2.28KB) | 0x0800_36E0 - 0x0800_3FFF (2.28KB)|
| Bootloader starts from: | 0x0800_0000 - 0x0800_36DF (13.72KB) | 0x0800_0000 - 0x0800_36DF (13.72KB)|

Simulate a USB removable disk (FAT32).

Just drag and drop the intel hex file to update the appcode. The bootloader will automatically restart if EOF RecordType is found in the hex file.
<b>Drag and drop the BIN file is removed.</b> Since intel hex file has the following advantages over the bin file:
1. Most of the compiler tools support direct export to intel hex file. No need to covert to bin file anymore
2. provides better integrity check
3. macOS works properly because of unique format in ihex file

During power up, the bootloader will check the content of 0x0800_4000 exists or not.
Hold PA0 (Connect to GND) during power up can force to enter bootloader mode.

#### USB Drive tested with following operating systems: 
- [x] Windows 8
- [x] Windows 7
- [x] MacOS Sierra
- [x] Ubuntu 18.04
- [x] Windows 10

Several examples are provided in "example-hex" folder to validate the bootloader feature.

### Test Procedure:
1. Hold PA0 during power up to enter bootloader mode.
2. A removable disk drive named "BOOTLOADER" is recognized.
3. Ensure PA0 is released.
4. Drag and drop STM32F103_FlashPC13LED_FAST.hex to the removable disk.
5. Since the intel hex file contains EOF record type, the bootloader will be self-reset.
6. The bootloader jumps to Appcode. LED (PC13) blinks. 

### Features currently supported:
The following features can be activated by preprocessor constant in btldr_config.h

#### AES256-CTR Encryption:
For more detail, please see the tools/hex-crypt folder.

#### CRC32 Checksum verification:
Before bootloader jumps to main application, it calculates the app's CRC32 checksum and compares it to the CRC32 calculated at build time (which is stored at end of flash). Jump to app is only performed in case of valid checksum.
1. In btldr_config.h, set BTLDR_ACT_CksNotVld to 1.
2. Use Keil to build the project, the bootloader size should be under 16KB (STM32_MSD_BTLDR_CRC32.hex).
3. Execute tools/crc-calc/add_crc32.bat to generate new hex file (CRC checksum is placed at last 32bit block of Flash).

#### Enable Bootloader from Application
It is possible to activate the bootloader from a running main application. 

```c
*((volatile uint32_t*)0x20004ffc) = BOOTKEY;	//same BOOTKEY as defined in Bootloader

/* Pull USB D+ PIN to GND so USB Host detects device disconnect */
LL_GPIO_SetPinMode(GPIOA,LL_GPIO_PIN_12, LL_GPIO_MODE_OUTPUT);
LL_GPIO_SetPinSpeed(GPIOA, LL_GPIO_PIN_12, LL_GPIO_SPEED_FREQ_LOW);
LL_GPIO_SetPinOutputType(GPIOA,LL_GPIO_PIN_12, LL_GPIO_OUTPUT_PUSHPULL);
LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_12);

LL_mDelay(1000);

NVIC_SystemReset();
```

#### Read the appcode content in firmware.bin
In btldr_config.h, set CONFIG_READ_FLASH to 1u to read the appcode content in firmware.bin. The content in firmware.bin is mapped to appcode area. Hence, the bin file size (flash) is also 48KB / 112KB.
