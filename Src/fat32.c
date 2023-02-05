/*************************************************************************************
# Released under MIT License

Copyright (c) 2020 SF Yip (yipxxx@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************/

#include <stdint.h>
#include <string.h>

#include "stm32f1xx_hal.h"
#include "btldr_config.h"
#include "fat32.h"
#include "ihex_parser.h"
#include "crypt.h"

//-------------------------------------------------------

#ifndef MIN
  #define MIN(a,b) (((a)<(b))?(a):(b))
#endif

//-------------------------------------------------------

#define FAT32_SECTOR_SIZE           512

#define FAT32_ATTR_READ_ONLY        0x01
#define FAT32_ATTR_HIDDEN           0x02
#define FAT32_ATTR_SYSTEM           0x04
#define FAT32_ATTR_VOLUME_ID        0x08
#define FAT32_ATTR_DIRECTORY        0x10
#define FAT32_ATTR_ARCHIVE          0x20
#define FAT32_ATTR_LONG_NAME        (FAT32_ATTR_READ_ONLY | FAT32_ATTR_HIDDEN | FAT32_ATTR_SYSTEM | FAT32_ATTR_VOLUME_ID)

#define FAT32_MAKE_DATE(dd, mm, yyyy)   (uint16_t)( ( (((yyyy)-1980) & 0x7F)  << 9) | (((mm) & 0x0F) << 5) | ((dd) & 0x1F) )
#define FAT32_MAKE_TIME(hh,mm)          (uint16_t)(( ((hh) & 0x1F)<< 11) | (((mm) & 0x3F) << 5))

//-------------------------------------------------------

typedef struct
{
    uint8_t BS_jmpBoot[3];
    uint8_t BS_OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    
    // FAT32 Structure
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BS_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    uint8_t BS_VolLab[11];
    uint8_t BS_FilSysType[8];
}__attribute__((packed)) fat32_bpb_t  ;

typedef struct
{
    uint32_t FSI_LeadSig;
    uint8_t FSI_Reserved1[480];
    uint32_t FSI_StrucSig;
    uint32_t FSI_Free_Count;
    uint32_t FSI_Nxt_Free;
    uint8_t FSI_Reserved2[12];
    uint32_t FSI_TrailSig;
}__attribute__((packed)) fat32_fsinfo_t  ;

typedef struct
{
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
}__attribute__((packed)) fat32_dir_entry_t  ;

//-------------------------------------------------------

typedef union
{
  uint8_t buf[4];
  uint32_t value32;
}uint32x_t;               // used for flash align write

//-------------------------------------------------------

#define FAT32_DIR_ENTRY_ADDR         0x00400000
#define FAT32_README_TXT_ADDR        0x00400600
#define FAT32_FIRMWARE_BIN_ADDR      0x00400800

//-------------------------------------------------------

static const char btldr_desc[] = "STM32 bootloader\nPlease drag and drop the intel hex file to this drive to update the appcode";

//-------------------------------------------------------

#define FAT32_MBR_HARDCODE  1u

#if (FAT32_MBR_HARDCODE > 0u)
static const uint8_t FAT32_MBR_DATA0[] = {
    0xEB, 0xFE, 0x90, 0x4D, 0x53, 0x44, 0x4F, 0x53, 0x35, 0x2E, 0x30, 0x00, 0x02, 0x01, 0x7C, 0x11,
    0x02, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x3F, 0x00, 0xFF, 0x00, 0x3F, 0x00, 0x00, 0x00,
    0xC1, 0xC0, 0x03, 0x00, 0x42, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x29, 0xB0, 0x49, 0x90, 0x02, 0x4E, 0x4F, 0x20, 0x4E, 0x41, 0x4D, 0x45, 0x20, 0x20,
    0x20, 0x20, 0x46, 0x41, 0x54, 0x33, 0x32, 0x20, 0x20, 0x20};
#endif
  
// Addr: 0x0000 and 0x0C00
static void _fat32_read_bpb(uint8_t *b)
{
#if (FAT32_MBR_HARDCODE > 0u)
    memcpy(b, FAT32_MBR_DATA0, sizeof(FAT32_MBR_DATA0));
    memset(b + sizeof(FAT32_MBR_DATA0), 0x00, FAT32_SECTOR_SIZE - sizeof(FAT32_MBR_DATA0) - 2);
    b[510] = 0x55;
    b[511] = 0xAA;
#else
    fat32_bpb_t *bpb = (fat32_bpb_t*)b;
    memset(b, 0, FAT32_SECTOR_SIZE);
    
    bpb->BS_jmpBoot[0] = 0xEB; bpb->BS_jmpBoot[1] = 0xFE; bpb->BS_jmpBoot[2] = 0x90;
    memcpy(bpb->BS_OEMName, "MSDOS5.0", 8);
    bpb->BPB_BytsPerSec = 512;  // 00 02
    bpb->BPB_SecPerClus = 1;
    bpb->BPB_RsvdSecCnt = 0x117C;   //2238KB
    bpb->BPB_NumFATs = 2;
    //bpb->BPB_RootEntCnt = 0x0000;
    //bpb->BPB_TotSec16 = 0x0000;
    bpb->BPB_Media = 0xF8;
    //bpb->BPB_FATSz16 = 0x0000;
    bpb->BPB_SecPerTrk = 0x003F;
    bpb->BPB_NumHeads = 0x00FF;
    bpb->BPB_HiddSec = 0x0000003F;
    bpb->BPB_TotSec32 = 0x0003C0C1;     //120MB
    
    // FAT32 Structure
    bpb->BPB_FATSz32 = 0x00000742;
    bpb->BPB_ExtFlags = 0x0000;
    bpb->BPB_FSVer = 0x0000;
    bpb->BPB_RootClus = 0x00000002;
    bpb->BPB_FSInfo = 0x0001;
    bpb->BPB_BkBootSec = 0x0006;
    //bpb->BS_Reserved[12];
    bpb->BS_DrvNum = 0x80;
    //bpb->BS_Reserved1;
    bpb->BS_BootSig = 0x29;
    bpb->BS_VolID = 0x94B11E23;
    memcpy(bpb->BS_VolLab, "BOOTLOADER ", 11);
    memcpy(bpb->BS_FilSysType, "FAT32   ", 8);
    
    b[510] = 0x55;
    b[511] = 0xAA;
#endif
}

// Addr: 0x0000_0200 and 0x0000_0E00
static void _fat32_read_fsinfo(uint8_t *b)
{
    fat32_fsinfo_t *fsinfo = (fat32_fsinfo_t*)b;
    memset(b, 0, FAT32_SECTOR_SIZE);
    fsinfo->FSI_LeadSig = 0x41615252;
    fsinfo->FSI_StrucSig = 0x61417272;
    fsinfo->FSI_Free_Count = 0x000398BE; //0xFFFFFFFF;
    fsinfo->FSI_Nxt_Free = 0x00000805;
    b[510] = 0x55;
    b[511] = 0xAA;
}

// Addr: 0x0000_0400 and 0x0000_1000
static void _fat32_read_fsinfo2(uint8_t *b)
{
    memset(b, 0, FAT32_SECTOR_SIZE);
    b[510] = 0x55;
    b[511] = 0xAA;
}

// Addr: 0x0000_1800
// An operating system is missing.... BS_jmpBoot[1] = 0xFE (jmp $)
// No need to gen bootcode

// FAT32 table
// Addr: 0x0022_F800 - 0x0023_1814
// Addr: 0x0031_7C00 - 0x0031_9C14
static void _fat32_read_fat_table(uint8_t *b, uint32_t addr)
{
    uint32_t s_offset = (addr - 0x22F800) >> 2;
    uint32_t *b32 = (uint32_t*)b;
    uint32_t i;
    
    if(addr == 0x22F800)    // FAT table start
    {
        // 1MB
        b32[0] = 0x0FFFFFF8;
        b32[1] = 0xFFFFFFFF;
        b32[2] = 0x0FFFFFFF;
        b32[3] = 0x0FFFFFFF;
        b32[4] = 0x0FFFFFFF;
        
        for(i=5; i<128; i++)
        {
            b32[i] = s_offset + i +1;
        }
    }
    else if(addr == 0x231800)    // FAT table end
    {
        for(i=0; i<4; i++)
        {
            b32[i] = s_offset + i +1;
        }
        b32[4] = 0x0FFFFFFF;
        for(i=5; i<128; i++)
        {
            b32[i] = 0x00000000;
        }
    }
    else
    {
        for(i=0; i<128; i++)
        {
            b32[i] = s_offset + i +1;
        }
    }
}

// Addr: 0x0040_0000
static void _fat32_read_dir_entry(uint8_t *b)
{
    fat32_dir_entry_t *dir = (fat32_dir_entry_t*)b;
    memset(b, 0, FAT32_SECTOR_SIZE);
    
    memcpy(dir->DIR_Name, "BOOTLOADER ", 11);
    dir->DIR_Attr = FAT32_ATTR_VOLUME_ID;
    dir->DIR_NTRes = 0x00;
    dir->DIR_CrtTimeTenth = 0x00;
    dir->DIR_CrtTime = FAT32_MAKE_TIME(0,0);
    dir->DIR_CrtDate = FAT32_MAKE_DATE(28,04,2020);
    dir->DIR_LstAccDate = FAT32_MAKE_DATE(28,04,2020);
    dir->DIR_FstClusHI = 0x0000;
    dir->DIR_WrtTime = FAT32_MAKE_TIME(0,0);
    dir->DIR_WrtDate = FAT32_MAKE_DATE(28,04,2020);
    dir->DIR_FstClusLO = 0x0000;
    dir->DIR_FileSize = 0x00000000;

    ++dir;

    memcpy(dir->DIR_Name, "README  TXT", 11);
    dir->DIR_Attr = FAT32_ATTR_ARCHIVE | FAT32_ATTR_READ_ONLY;
    dir->DIR_NTRes = 0x18;
    dir->DIR_CrtTimeTenth = 0x00;
    dir->DIR_CrtTime = FAT32_MAKE_TIME(0,0);
    dir->DIR_CrtDate = FAT32_MAKE_DATE(28,04,2020);
    dir->DIR_LstAccDate = FAT32_MAKE_DATE(28,04,2020);
    dir->DIR_FstClusHI = 0x0000;
    dir->DIR_WrtTime = FAT32_MAKE_TIME(0,0);
    dir->DIR_WrtDate = FAT32_MAKE_DATE(28,04,2020);
    dir->DIR_FstClusLO = 0x0005;
    dir->DIR_FileSize = strlen(btldr_desc);

#if (CONFIG_READ_FLASH > 0u)
    ++dir;
    
    memcpy(dir->DIR_Name, "FIRMWAREBIN", 11);
    dir->DIR_Attr = FAT32_ATTR_ARCHIVE;
    dir->DIR_NTRes = 0x18;
    dir->DIR_CrtTimeTenth = 0x00;
    dir->DIR_CrtTime = FAT32_MAKE_TIME(0,0);
    dir->DIR_CrtDate = FAT32_MAKE_DATE(28,04,2020);
    dir->DIR_LstAccDate = FAT32_MAKE_DATE(28,04,2020);
    dir->DIR_FstClusHI = 0x0000;
    dir->DIR_WrtTime = FAT32_MAKE_TIME(0,0);
    dir->DIR_WrtDate = FAT32_MAKE_DATE(28,04,2020);
    dir->DIR_FstClusLO = 0x0006;
    dir->DIR_FileSize = APP_SIZE;
#endif
}

// Addr : 0x0040_0600
static void _fat32_read_btldr_desc(uint8_t *b, uint32_t addr)
{
    memcpy(b, btldr_desc, sizeof(btldr_desc));
}

// Addr : 0x0040_0800
static void _fat32_read_firmware(uint8_t *b, uint32_t addr)
{
#if (CONFIG_READ_FLASH > 0u)
    uint32_t offset = addr - FAT32_FIRMWARE_BIN_ADDR;
    uint32_t addr_end = MIN(offset + FAT32_SECTOR_SIZE, APP_SIZE);
    int32_t total_size = addr_end - offset;
    
    memcpy(b, (void*)(APP_ADDR + offset), total_size);
#else
    memset(b, 0x00, FAT32_SECTOR_SIZE);
#endif
}

static bool _fat32_write_firmware(uint32_t phy_addr, const uint8_t *buf, uint8_t size)
{
    bool return_status = true;
  
    HAL_StatusTypeDef status;
    
    HAL_FLASH_Unlock();
    
#if (CONFIG_SUPPORT_CRYPT_MODE > 0u)
    
    if(ihex_is_crypt_mode())
    {
      if(size != AES_BLOCKLEN)
      {
          return false;
      }
      
      crypt_decrypt((uint8_t*)buf, size, phy_addr);
    }
#endif
    
    if(phy_addr == APP_ADDR)
    {
        // Erase the APPCODE area
        uint32_t PageError = 0;
        FLASH_EraseInitTypeDef eraseinitstruct;
        
        eraseinitstruct.TypeErase = FLASH_TYPEERASE_PAGES;
        eraseinitstruct.PageAddress = APP_ADDR;
        eraseinitstruct.NbPages = APP_SIZE / FLASH_PAGE_SIZE;
        status = HAL_FLASHEx_Erase(&eraseinitstruct, &PageError);
        
        if(status != HAL_OK)
        {
            return_status = false;
            goto EXIT;
        }
    }
      
    if((phy_addr >= APP_ADDR) && ((phy_addr+size) <= (APP_ADDR + APP_SIZE)) )
    {
        uint8_t unalign = phy_addr & 0x03;    // support unalign write
        uint32x_t content;
      
        if(unalign)
        {
            uint32_t prog_addr = phy_addr & ~(0x03);
            uint8_t prog_size = MIN(size, 4-unalign);
            
            content.buf[0] = 0xFF;
            content.buf[1] = 0xFF;
            content.buf[2] = 0xFF;
            content.buf[3] = 0xFF;
        
            uint8_t i;
            for(i=0; i<prog_size; i++)
            {
                content.buf[i+unalign] = *buf++;
                ++phy_addr;
                --size;
            }
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, prog_addr, content.value32);
        }
      
        while(size >= 4)
        {
            content.buf[0] = buf[0];
            content.buf[1] = buf[1];
            content.buf[2] = buf[2];
            content.buf[3] = buf[3];
            
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, phy_addr, content.value32);
            phy_addr += 4;
            buf += 4;
            size -= 4;
        }
        
        if(size)  // write remaining byte
        {
            content.buf[0] = 0xFF;
            content.buf[1] = 0xFF;
            content.buf[2] = 0xFF;
            content.buf[3] = 0xFF;
            
            uint8_t i;
            for(i=0; i<size; i++)
            {
                content.buf[i] = *buf++;
            }
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, phy_addr, content.value32);
        }
    }
    
EXIT:
    HAL_FLASH_Lock();
    return return_status;
}

//-------------------------------------------------------

// sector size should be 512 byte

bool fat32_read(uint8_t *b, uint32_t addr)
{
    if(addr & (FAT32_SECTOR_SIZE - 1))      // if not align ?
    {
        return false;
    }
    
    if(addr == 0x0000 || addr == 0x0C00)
    {
        _fat32_read_bpb(b);
    }
    else if(addr == 0x0200 || addr == 0x0E00)
    {
        _fat32_read_fsinfo(b);
    }
    else if(addr == 0x0400 || addr == 0x1000)
    {
        _fat32_read_fsinfo2(b);
    }
    else if((addr >= 0x22F800 && addr < 0x231A00) || (addr >= 0x317C00&& addr < 0x319E00))
    {
        _fat32_read_fat_table(b, addr);
    }
    else if(addr == FAT32_DIR_ENTRY_ADDR)
    {
        _fat32_read_dir_entry(b);
    }
    else if(addr >= FAT32_README_TXT_ADDR && addr < (FAT32_README_TXT_ADDR+FAT32_SECTOR_SIZE))
    {
        _fat32_read_btldr_desc(b, addr);
    }
    else if(addr >= FAT32_FIRMWARE_BIN_ADDR && addr < (FAT32_FIRMWARE_BIN_ADDR+APP_SIZE))
    {
        _fat32_read_firmware(b, addr);
    }
    else
    {
        memset(b, 0x00, FAT32_SECTOR_SIZE);
    }
    
    return true;
}

bool fat32_write(const uint8_t *b, uint32_t addr)
{
    if(addr & (FAT32_SECTOR_SIZE - 1))      // if not align ?
    {
        return false;
    }
    
    if(addr < FAT32_DIR_ENTRY_ADDR)
    {
      // No operation
    }
    else if(addr == FAT32_DIR_ENTRY_ADDR)
    {
        uint32_t i;
        for(i=0; i<FAT32_SECTOR_SIZE; i+= sizeof(fat32_dir_entry_t))
        {
            const uint8_t *b_offset = (const uint8_t *)(b + i);
            fat32_dir_entry_t *entry = (fat32_dir_entry_t*)b_offset;
            
            uint8_t *filename = entry->DIR_Name;

            if(filename[8] == 'H' && filename[9] == 'E' && filename[10] == 'X')
            {
                ihex_reset_state();
                
                // uint32_t clus = (((uint32_t)(entry->DIR_FstClusHI)) << 16) | entry->DIR_FstClusLO;
              
                // fw_addr_range.begin = ((clus-2) + 0x2000 ) * FAT32_SECTOR_SIZE;
                // fw_addr_range.end = fw_addr_range.begin + entry->DIR_FileSize;       // don't know the size of HEX file
            }
        }
    }
    else
    {
        ihex_set_callback_func(_fat32_write_firmware);
        ihex_parser(b, FAT32_SECTOR_SIZE);
    }
    
    return true;
}
