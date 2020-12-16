/*************************************************************************************
# Released under MIT License
Copyright (c) 2020 SF Yip (yipxxx@gmail.com)
Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <vector>


extern "C" {

#include "ihex_parser.h"
#include "crypt.h"

}


#define CONFIG_DEBUG_OUTPUT        0u

using namespace std;

typedef vector<uint8_t> byte_array_t;
static map<uint32_t, byte_array_t> mem_map;

bool save_flash_data(uint32_t addr, const uint8_t *buf, uint8_t bufsize)
{
    byte_array_t byte_array(buf, buf + bufsize);
    mem_map.insert(pair < uint32_t, byte_array_t>(addr, byte_array));

#if (CONFIG_DEBUG_OUTPUT > 0u)
    uint8_t i;
    printf("[Read from file] %08X:", addr);
    for (i = 0; i<bufsize; i++)
    {
        printf("%02X", buf[i]);
    }
    printf("\n");
#endif

    return true;
}

bool encrypt_file(const char *dest_filename, const char *src_filename)
{
    /* Open HEX file, read per 512 byte block size */
    bool return_status = false;
    
    map<uint32_t, byte_array_t>::iterator iter;
    uint8_t *phy_mem = 0;
    uint32_t start_addr;
    uint32_t size;
    
    uint8_t fbuf[512];
    size_t readcount = 0;
    
    uint32_t i, j;
    uint8_t cs;
    
    FILE *fp = fopen(src_filename, "r");
    if (fp == NULL)
    {
        printf("Cannot open hex file for reading\n");
        goto EXIT;
    }

    ihex_set_callback_func(save_flash_data);

    while ((readcount = fread(fbuf, 1, sizeof(fbuf), fp)) > 0)
    {
        if (readcount < sizeof(fbuf))
        {
            fbuf[readcount] = '\0';     // Add null teminated char
        }
        if (!ihex_parser(fbuf, sizeof(fbuf)))
        {
            printf("Parse failed\n");
            goto EXIT;
        }
    }
    fclose(fp);
    fp = NULL;
    
    start_addr = mem_map.begin()->first;
    size = mem_map.rbegin()->first + mem_map.rbegin()->second.size() - start_addr;

    printf("Start address: %08X\n", start_addr);
    printf("Size: %08X\n", size);

    if (size & AES_BLOCKLEN)
    {
        size = (size & ~(AES_BLOCKLEN-1)) + AES_BLOCKLEN;
        printf("Size after align: %08X\n", size);
    }

    phy_mem = (uint8_t*)malloc(size);
    if (!phy_mem)
    {
        printf("Failed to allocate memory\n");
        goto EXIT;
    }
    memset(phy_mem, 0xFF, size);

    for (iter = mem_map.begin(); iter != mem_map.end(); iter++)
    {
        uint32_t offs = iter->first - start_addr;
        memcpy(phy_mem + offs, iter->second.data(), iter->second.size());
    }

#if (CONFIG_DEBUG_OUTPUT > 0u)
    printf("Dump mem before encrypt\n");
    for (i = 0; i < size; i++)
    {
        printf("%02X", phy_mem[i]);
        if (((i+1) % 16) == 0)
        {
            printf("\n");
        }
    }
#endif
    
    crypt_init();
    
    for(i=0; i<size; i+=AES_BLOCKLEN)
    {
        crypt_encrypt(&phy_mem[i], AES_BLOCKLEN, start_addr + i);
    }

#if (CONFIG_DEBUG_OUTPUT > 0u)
    printf("Dump mem after encrypt\n");
    for (i = 0; i < size; i++)
    {
        printf("%02X", phy_mem[i]);
        if (((i + 1) % 16) == 0)
        {
            printf("\n");
        }
    }
#endif

    // Export to intel hex
    fp = fopen(dest_filename, "wb");
    if (!fp)
    {
        printf("Cannot open file for writing\n");
        goto EXIT;
    }

    fprintf(fp, ":0000000EF2\n");       // 0E record type (self-defined record type), this file is encrypted

    cs = 0x02;                          // generate checksum
    cs += 0x04;
    cs += (start_addr >> 24) & 0xff;
    cs += (start_addr >> 16) & 0xff;
    cs = ~cs + 1;
    fprintf(fp, ":02000004%04X%02X\n", (start_addr >> 16) & 0xffff, cs);

    for (i = 0; i < size; i+=16)
    {
        cs = 0x10;                      // generate checksum
        cs += ((start_addr+i) >> 8) & 0xff;
        cs += (start_addr+i) & 0xff;
        for (j = 0; j < 16; j++)
        {
            cs += phy_mem[i+j];
        }
        cs = ~cs + 1;
        fprintf(fp, ":10%04X00%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", (start_addr+i) & 0xffff,     \
                    phy_mem[i], phy_mem[i+1], phy_mem[i+2], phy_mem[i+3], phy_mem[i+4], phy_mem[i+5], phy_mem[i+6], phy_mem[i+7],   \
                    phy_mem[i+8], phy_mem[i+9], phy_mem[i+10], phy_mem[i + 11], phy_mem[i + 12], phy_mem[i + 13], phy_mem[i + 14], phy_mem[i + 15], cs);
    }

    fprintf(fp, ":00000001FF\n");       // EOF record type

    fclose(fp);
    fp = NULL;

    return_status = true;

EXIT:
    mem_map.clear();
    
    if (fp)
        fclose(fp);
    
    if(phy_mem)
        free(phy_mem);
    
    return return_status;
}

bool test_crypt()
{
    // Test crypt function
    uint8_t plaintext[AES_BLOCKLEN] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
    uint8_t cipherbuf[AES_BLOCKLEN];
    uint8_t chk_plaintext[AES_BLOCKLEN];
    uint8_t i;
    
    crypt_init();
    
    printf("=== Test crypt function\n");
    memcpy(cipherbuf, plaintext, AES_BLOCKLEN);
    crypt_encrypt(cipherbuf, AES_BLOCKLEN, 0x08000000);
    printf("Ciphertext result:");
    for (i = 0; i < AES_BLOCKLEN; i++)
    {
        printf("%02X", cipherbuf[i]);
    }
    printf("\n");

    memcpy(chk_plaintext, cipherbuf, AES_BLOCKLEN);
    crypt_decrypt(chk_plaintext, AES_BLOCKLEN, 0x08000000);
    printf("Plaintext result:");
    for (i = 0; i < AES_BLOCKLEN; i++)
    {
        printf("%02X", chk_plaintext[i]);
    }
    printf("\n");

    if (memcmp(chk_plaintext, plaintext, AES_BLOCKLEN) != 0)
    {
        printf("Decrypt result does not match\n");
        return false;
    }

    return true;
}

void show_help()
{
    printf("Crypt utility for STM32 MSD bootloader @ 2020\n\n");
    printf("Orginial author: https://github.com/sfyip\n");
    printf("Released under MIT License. Anyone is free to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so\n\n");
    printf("Usage: hex_crypt -o dest.hex -i src.hex\n");
}

int main(int argc, char *argv[])
{
#if 0
    if (!test_crypt())
    {
        printf("Test crypt function failed\n");
    }
#endif

    if (argc == 5)
    {
        const char* dest_filename = 0;
        const char* src_filename = 0;

        uint8_t i;
        for (i = 1; i < 5; i++)
        {
            if (strcmp(argv[i], "-o") == 0)
            {
                if ((i + 1) < argc)
                {
                    dest_filename = argv[i + 1];

                }
            }
            else if (strcmp(argv[i], "-i") == 0)
            {
                if ((i + 1) < argc)
                {
                    src_filename = argv[i + 1];
                }
            }
        }

        if (src_filename == NULL)
        {
            printf("Please specific src filename\n");
            return EXIT_FAILURE;
        }

        if (dest_filename == NULL)
        {
            printf("Please specific dest filename\n");
            return EXIT_FAILURE;
        }

        if (!encrypt_file(dest_filename, src_filename))
        {
            printf("Encrypt file failed\n");
            return EXIT_FAILURE;
        }
        printf("Encrypt file done\n");
    }
    else
    {
        show_help();
    }

    return EXIT_SUCCESS;
}

