# STM32F103_MSD_BOOTLOADER HEX Crypt Utility

Usage: hex_crypt -o dest.hex -i src.hex

#### Description:
Generate an encrypted HEX file by AES256-CTR.

The AES Key and IV is placed in crypt.c, which is same as STM32F103_MSD_BOOTLOADER one.

According to the AES-CTR standard, the IV should change every time. In order to make it difficult to guest, LFSR128(INIT_IV, prog_addr) is used to calculate the new IV value.

#### Detail Operation:
1. Parse src.hex, load the content into std::map<uint32_t, byte_array_t> mem_map
2. Get the start address and calculate the aligned size (AES block size = 16 byte)
3. Create dest.hex, write a special record type :0000000EF2 at 1st line to indicate it is an encrypted HEX file
4. Encrypt the file content per AES block, append to dest.hex
5. Append EOF record type after encryption is finished


