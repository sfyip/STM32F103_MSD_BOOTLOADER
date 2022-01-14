srec_cat.exe ..\..\example-hex\STM32F103_FlashPC13LED_FAST.hex -Intel ^
-fill 0xFF 0x08004000 0x0801FFFC ^
-crop 0x08004000 0x0801FFFC ^
-CRC32_Big_Endian 0x801FFFC ^
-o ..\..\example-hex\STM32F103_FlashPC13LED_FAST_CRC32.hex -Intel