#include "crypt.h"
#include "aes_ctr.h"


static const uint8_t AES_INIT_IV[AES_IV_SIZE] =    {0x84, 0x5E, 0xF4, 0x23, 0x36, 0x83, 0x40, 0x8E, 0x83, 0x22, 0x74, 0xCF, 0xF1, 0xF0, 0x07, 0xCC};
static const uint8_t AES_KEY[AES_KEY_SIZE]    =    {0x29, 0x76, 0xDE, 0xF0, 0x2A, 0xF4, 0x4E, 0xD7, 0xBE, 0x87, 0x1E, 0xA9, 0xDA, 0xB2, 0x5B, 0x24, \
                                                    0x3A, 0xCA, 0x66, 0x38, 0xA7, 0xF6, 0x44, 0x45, 0xB1, 0x2C, 0x5E, 0x86, 0xCB, 0x73, 0xBA, 0x2F};

static void gen_iv_by_lfsr(uint8_t *iv, uint32_t addr)
{
    uint8_t i;
  
    for(i=0; i<AES_BLOCK_SIZE; i+=4)
    {
      iv[i]   = (addr         & 0xff) ^ AES_INIT_IV[i];
      iv[i+1] = ((addr >>  8) & 0xff) ^ AES_INIT_IV[i+1];
      iv[i+2] = ((addr >> 16) & 0xff) ^ AES_INIT_IV[i+2];
      iv[i+3] = ((addr >> 24) & 0xff) ^ AES_INIT_IV[i+3];
    }
    
    uint32_t bit;     // Must be 32-bit to allow bit<<31 later in the code
    uint8_t loop = 100;

    while (loop--)
    {
        bit = ((iv[0] >> 0) ^ (iv[1] >> 2) ^ (iv[2] >> 3) ^ (iv[2] >> 7) ^ (iv[3] >> 5)) & 1;
    
        iv[0] = (iv[0] >> 1) | (iv[1] << 31);
        iv[1] = (iv[1] >> 1) | (iv[2] << 31);
        iv[2] = (iv[2] >> 1) | (iv[3] << 31);
        iv[3] = (iv[3] >> 1) | (bit << 31);
    };
}

void decrypt(uint8_t *outbuf,  const uint8_t *inbuf, uint32_t size, uint32_t addr)
{
    uint8_t new_iv[AES_IV_SIZE];
    gen_iv_by_lfsr(new_iv, addr);
    aes_ctr_crypt_buffer(outbuf, inbuf, size, AES_KEY, new_iv);
}
