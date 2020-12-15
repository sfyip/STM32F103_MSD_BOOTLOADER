#include "crypt.h"
#include "aes_ctr.h"


static const uint8_t AES_INIT_IV[AES_IV_SIZE] =    {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
static const uint8_t AES_KEY[AES_KEY_SIZE]    =    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, \
                                                    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF};

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
