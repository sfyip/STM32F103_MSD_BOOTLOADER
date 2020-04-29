#ifndef _BTLDR_CONFIG_H_
#define _BTLDR_CONFIG_H_

// STM32F103C8T6 - 128KB Flash Size
#define DEV_CODE_ADDR           0x08000000
#define DEV_FLASH_SIZE          (128*1024)
#define DEV_ERASE_PAGE_SIZE     1024

#define APP_ADDR                (DEV_CODE_ADDR + 0x8000)
#define APP_SIZE                (DEV_FLASH_SIZE - 0x8000)

#define CONFIG_READ_FLASH       1u

#endif
