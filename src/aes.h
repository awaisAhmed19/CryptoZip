
#ifndef AES_H
#define AES_H
#include <stddef.h>
enum keySize { SIZE_16 = 16, SIZE_24 = 24, SIZE_32 = 32 };

void expandKey(unsigned char *expandedKey, unsigned char *key, enum keySize,
               size_t expandedKeySize);
char aes_encrypt(unsigned char *input, unsigned char *output, unsigned char *key,
                 enum keySize size);

char aes_decrypt(unsigned char *input, unsigned char *output, unsigned char *key,
                 enum keySize size);
#endif
