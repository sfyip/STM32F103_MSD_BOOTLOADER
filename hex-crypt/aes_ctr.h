#ifndef _AES_CTR_H_
#define _AES_CTR_H_

#include <stdint.h>

#define AES256

#define AES_IV_SIZE        16
#define AES_BLOCK_SIZE     16

#ifdef AES256
    #define AES_KEY_SIZE 32
#elif defined(AES192)
    #define AES_KEY_SIZE 24
#else
    #define AES_KEY_SIZE 16   // Key length in bytes
#endif

void aes_ctr_crypt_buffer(uint8_t* output, const uint8_t* input, uint32_t length, const uint8_t* key, const uint8_t* iv);

#endif //_AES_CTR_H_
