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


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ihex_parser.h"

//-------------------------------------------------------

//IHEX file parser state machine
#define START_CODE_STATE        0
#define BYTE_COUNT_0_STATE      1
#define BYTE_COUNT_1_STATE      2
#define ADDR_0_STATE            3
#define ADDR_1_STATE            4
#define ADDR_2_STATE            5
#define ADDR_3_STATE            6
#define RECORD_TYPE_0_STATE     7
#define RECORD_TYPE_1_STATE     8
#define DATA_STATE              9
#define CHECKSUM_0_STATE        10
#define CHECKSUM_1_STATE        11

//-------------------------------------------------------

#define RECORD_TYPE_DATA            0x00
#define RECORD_TYPE_EOF             0x01
#define RECORD_TYPE_EX_SEG_ADDR     0x02
#define RECORD_TYPE_START_SEG_ADDR  0x03
#define RECORD_TYPE_EX_LIN_ADDR     0x04
#define RECORD_TYPE_START_LIN_ADDR  0x05
#define RECORD_TYPE_CRYPT_MODE      0x0E

//-------------------------------------------------------

#define INVALID_HEX_CHAR        'x'
#define IHEX_DATA_SIZE          255

//-------------------------------------------------------

static uint8_t HexToDec(uint8_t h)
{
    if (h >= '0' && h <= '9')
        return h - '0';
    else if (h >= 'A' && h <= 'F')
        return h - 'A' + 0xA;
    else if (h >= 'a' && h <= 'z')
        return h - 'a' + 0xA;
    else
        return INVALID_HEX_CHAR;
}

static uint8_t state;
static uint8_t byte_count;
static uint16_t address_lo;
static uint16_t address_hi;
static bool ex_segment_addr_mode = false;
static uint8_t record_type;
static uint8_t data[IHEX_DATA_SIZE];
static uint16_t data_size_in_nibble;

static uint8_t temp_cs;         // save checksum high byte
static uint8_t calc_cs;         // calculate checksum
static bool calc_cs_toogle = false;

static ihex_callback_fp callback_fp = 0;
static bool crypt_mode = false;     // extend the intex hex file format to support encryption

#define TRANSFORM_ADDR(addr_hi, addr_lo)       (ex_segment_addr_mode) ?                                  \
                                                ( (((uint32_t)(addr_hi)) << 4) + ((uint32_t)(addr_lo)) ): \
                                                ( (((uint32_t)(addr_hi)) << 16) | ((uint32_t)(addr_lo)) )


void ihex_reset_state()
{
    state = 0;
    address_lo = 0;
    address_hi = 0;
    ex_segment_addr_mode = false;
    crypt_mode = false;
}

void ihex_set_callback_func(ihex_callback_fp fp)
{
    callback_fp = fp;
}

bool ihex_is_crypt_mode()
{
    return crypt_mode;
}

bool ihex_parser(const uint8_t *steambuf, uint32_t size)
{
    uint32_t i;
    uint8_t c, hc;
    
    for (i = 0; i<size; i++)
    {
        c = steambuf[i];

        if (c == '\0')
        {
            return true;
        }

        if (state == START_CODE_STATE)
        {
            calc_cs = 0x00;
            calc_cs_toogle = false;
        }
        else if (state >= BYTE_COUNT_0_STATE && state <= CHECKSUM_1_STATE)
        {
            if ((hc = HexToDec(c)) == INVALID_HEX_CHAR)
            {
                return false;
            }

            if (!calc_cs_toogle)
            {
                temp_cs = hc;
            }
            else
            {
                calc_cs += (temp_cs << 4) | hc;
            }
            calc_cs_toogle = !calc_cs_toogle;
        }

        switch (state)
        {
        case START_CODE_STATE:
            if (c == '\r' || c == '\n')
            {
                continue;
            }
            else if (c == ':')
            {
                byte_count = 0;
                record_type = RECORD_TYPE_DATA;
                address_lo = 0x0000;
                memset(data, 0, sizeof(data));
                data_size_in_nibble = 0;
                ++state;
            }
            else
            {
                return false;
            }
            break;

        case BYTE_COUNT_0_STATE:
        case BYTE_COUNT_1_STATE:
            byte_count = (byte_count << 4) | hc;
            ++state;
            break;

        case ADDR_0_STATE:
        case ADDR_1_STATE:
        case ADDR_2_STATE:
        case ADDR_3_STATE:
        {
            address_lo = ((address_lo << 4) | hc);   // only alter lower 16-bit address
            ++state;
            break;
        }
        
        case RECORD_TYPE_0_STATE:
            if (hc != 0)
            {
                return false;
            }
            ++state;
            break;

        case RECORD_TYPE_1_STATE:
            if ( !(hc <= RECORD_TYPE_START_LIN_ADDR || hc == RECORD_TYPE_CRYPT_MODE) )
            {
                return false;
            }
            
            record_type = hc;

            if (byte_count == 0)
            {
                state = CHECKSUM_0_STATE;
            }
            else if (byte_count > sizeof(data))
            {
                return false;
            }
            else
            {
                ++state;
            }

            break;

        case DATA_STATE:
        {
            uint8_t b_index = data_size_in_nibble >> 1;
            data[b_index] = (data[b_index] << 4) | hc;

            ++data_size_in_nibble;
            if ((data_size_in_nibble >> 1) >= byte_count)
            {
                ++state;
            }
            break;
        }
        
        case CHECKSUM_0_STATE:
            ++state;
            break;

        case CHECKSUM_1_STATE:
            if((byte_count<<1) != data_size_in_nibble)  // Check whether byte count field match the data size 
            {
                return false;
            }
            
            if (calc_cs != 0x00)
            {
                return false;
            }

            if (record_type == RECORD_TYPE_EX_SEG_ADDR)           // Set extended segment addresss
            {
                address_hi = ((uint16_t)data[0] << 8) | (data[1]);
                ex_segment_addr_mode = true;
            }
            else if (record_type == RECORD_TYPE_EX_LIN_ADDR)      // Set linear addresss
            {
                address_hi = ((uint16_t)data[0] << 8) | (data[1]);
                ex_segment_addr_mode = false;
            }

            if (record_type == RECORD_TYPE_DATA && callback_fp != 0)
            {
                uint32_t address = TRANSFORM_ADDR(address_hi, address_lo);
                if(!callback_fp(address, data, data_size_in_nibble>>1))
                {
                    return false;
                }
            }
            else if(record_type == RECORD_TYPE_CRYPT_MODE)
            {
                crypt_mode = true;
            }
#if (CONFIG_SOFT_RESET_AFTER_IHEX_EOF > 0u)
            else if(record_type == RECORD_TYPE_EOF)
            {
                NVIC_SystemReset();
            }
#endif

            state = START_CODE_STATE;
            break;

        default:
            return false;
        }
    }
    return true;
}


