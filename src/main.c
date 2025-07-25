// main.c
/***************************************************************************
 *          Lempel, Ziv Encoding and Decoding
 *
 *   File    : main.c
 *                              DESCRIPTION
 *
 *   LZ77 is a lossless data compression algorithms published by Abraham
 *   Lempel and Jacob Ziv in 1977. It is a dictionary coder and maintains a
 *   sliding window during compression. The sliding window is divided in two
 *   parts: Search-Buffer (dictionary - encoded data) and Lookahead (uncomp-
 *   ressed data). LZ77 algorithms achieve compression by addressing byte
 *   sequences from former contents instead of the original data. All data
 *   will be coded in the same form (called token): -Address to already
 *   coded contents; -Sequence length; -First deviating symbol.
 *   The window is contained in a fixed size buffer.
 *   The match between SB and LA is made by a binary tree, implemented in an
 *   array.
 ***************************************************************************/

/***************************************************************************
 *                             INCLUDED FILES
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <unitstd.h>

#include <sys/wait.h>  // for wait()
#include <unistd.h>    // for fork(), execv()

#include "aes.h"
#include "bitio.h"
#include "debug.h"
#include "getopt.h"
#include "lz77.h"

/***************************************************************************
 *                                CONSTANTS
 ***************************************************************************/
#define MIN_LA_SIZE 2     /* min lookahead size */
#define MAX_LA_SIZE 255   /* max lookahead size */
#define MIN_SB_SIZE 0     /* min search buffer size */
#define MAX_SB_SIZE 65535 /* max search buffer size */

/***************************************************************************
 *                            TYPE DEFINITIONS
 ***************************************************************************/
typedef enum { ENCODE, DECODE } MODES;

/***************************************************************************
 *                            USER INTERFACE
 * Syntax: ./lz77 <options>
 * Options: -c: compression mode
 *          -d: decompression mode
 *          -i <filename>: input file
 *          -o <filename>: output file
 *          -l <value> : lookahead size (default 15)
 *          -s <value> : search-buffer size (default 4095)
 *          -h: help
 ***************************************************************************/
int main(int argc, char *argv[]) {
    int opt;
    FILE *file = NULL;
    FILE *intermediate = NULL;
    struct bitFILE *bitF = NULL;
    MODES mode = -1;
    char *filenameIn = NULL, *filenameOut = NULL;
    int la_size = 15, sb_size = 4095;
    int doEncrypt = 0, doDecrypt = 0;
    char *aesKeyStr = NULL;
    unsigned char aesKey[16] = {0};

    int runTests = 0;
    fprintf(stderr, "[INIT] Parsing command-line args...\n");

    while ((opt = getopt(argc, argv, "cdi:o:l:s:EDk:ht")) != -1) {
        switch (opt) {
            case 't':
                runTests = 1;
                break;
            case 'c':
                mode = ENCODE;
                break;
            case 'd':
                mode = DECODE;
                break;
            case 'i':
                filenameIn = strdup(optarg);
                break;
            case 'o':
                filenameOut = strdup(optarg);
                break;
            case 'l':
                la_size = atoi(optarg);
                break;
            case 's':
                sb_size = atoi(optarg);
                break;
            case 'E':
                doEncrypt = 1;
                break;
            case 'D':
                doDecrypt = 1;
                break;
            case 'k':
                aesKeyStr = strdup(optarg);
                ASSERT(aesKeyStr != NULL);
                memset(aesKey, 0, 16);
                size_t keylen = strlen(aesKeyStr);
                if (keylen > 16) keylen = 16;
                memcpy(aesKey, aesKeyStr, keylen);
                break;
            case 'h':
            default:
                printf("Usage: ...\n");
                exit(EXIT_SUCCESS);
        }
    }

    if (runTests) {
        fprintf(stderr, "[TEST] Running round-trip compression/encryption test...\n");

        // 1. Compress + Encrypt
        char *encOut = "test_output_encrypted.lz77";
        char *decOut = "test_output_decrypted.txt";

        char *compressCmd[] = {argv[0], "-c",       "-E", "-k",   aesKeyStr,
                               "-i",    filenameIn, "-o", encOut, NULL};

        fprintf(stderr, "[TEST] Compressing and encrypting '%s' to '%s'\n", filenameIn, encOut);

        if (fork() == 0) {
            execv(argv[0], compressCmd);
            perror("[TEST] execv compressCmd failed");
            exit(1);
        }

        wait(NULL);

        // 2. Decrypt + Decompress
        char *decompressCmd[] = {argv[0], "-d",   "-D", "-k",   aesKeyStr,
                                 "-i",    encOut, "-o", decOut, NULL};

        fprintf(stderr, "[TEST] Decrypting and decompressing '%s' to '%s'\n", encOut, decOut);

        if (fork() == 0) {
            execv(argv[0], decompressCmd);
            perror("[TEST] execv decompressCmd failed");
            exit(1);
        }

        wait(NULL);

        // 3. Compare
        char cmpCmd[512];
        snprintf(cmpCmd, sizeof(cmpCmd), "diff -q %s %s", filenameIn, decOut);
        int cmp = system(cmpCmd);
        if (cmp == 0)
            fprintf(stderr, "[TEST] ✅ Round-trip test passed! Files match.\n");
        else
            fprintf(stderr, "[TEST] ❌ Round-trip test FAILED. Files differ!\n");

        free(aesKeyStr);
        free(filenameIn);
        free(filenameOut);
        exit(0);
    }
    // Check must-have args
    ASSERT(filenameIn != NULL);
    ASSERT(filenameOut != NULL);
    ASSERT(mode == ENCODE || mode == DECODE);
    ASSERT(la_size >= MIN_LA_SIZE && la_size <= MAX_LA_SIZE);
    ASSERT(sb_size >= MIN_SB_SIZE && sb_size <= MAX_SB_SIZE);

    if ((doEncrypt || doDecrypt)) {
        ASSERT(aesKeyStr != NULL);
        ASSERT(strlen((char *)aesKey) > 0);
    }

    if (mode == ENCODE) {
        fprintf(stderr, "[ENCODE] Opening input file: %s\n", filenameIn);
        file = fopen(filenameIn, "rb");
        ASSERT(file != NULL);

        intermediate = tmpfile();
        ASSERT(intermediate != NULL);

        fprintf(stderr, "[ENCODE] Compressing...\n");
        bitF = bitIO_open(intermediate, BIT_IO_W);
        ASSERT(bitF != NULL);
        ASSERT(bitF->mode == BIT_IO_W);

        encode(file, bitF, la_size, sb_size);
        fprintf(stderr, "[ENCODE] Compression done.\n");

        bitIO_close(bitF);
        fclose(file);
        rewind(intermediate);
        fseek(intermediate, 0, SEEK_END);
        long size = ftell(intermediate);
        fseek(intermediate, 0, SEEK_SET);  // rewind again
        fprintf(stderr, "[DEBUG] Intermediate file size: %ld bytes\n", size);
        FILE *outFile = fopen(filenameOut, "wb");
        ASSERT(outFile != NULL);

        if (doEncrypt) {
            fprintf(stderr, "[ENCODE] Encrypting data to output file...\n");
            unsigned char inBlock[16] = {0}, outBlock[16];
            size_t read;
            while ((read = fread(inBlock, 1, 16, intermediate)) > 0) {
                ASSERT(read <= 16);
                if (read < 16) memset(inBlock + read, 0, 16 - read);
                aes_encrypt(inBlock, outBlock, aesKey, SIZE_16);
                fwrite(outBlock, 1, 16, outFile);
            }
        } else {
            fprintf(stderr, "[ENCODE] Writing raw compressed data to output file...\n");
            char buf[1024];
            size_t read;
            while ((read = fread(buf, 1, sizeof(buf), intermediate)) > 0) {
                fwrite(buf, 1, read, outFile);
            }
        }

        fclose(outFile);
        fprintf(stderr, "[ENCODE] Done.\n");

    } else if (mode == DECODE) {
        fprintf(stderr, "[DECODE] Preparing intermediate file...\n");
        intermediate = tmpfile();
        ASSERT(intermediate != NULL);

        file = fopen(filenameIn, "rb");
        ASSERT(file != NULL);

        if (doDecrypt) {
            fprintf(stderr, "[DECODE] Decrypting input file...\n");
            unsigned char inBlock[16], outBlock[16];
            size_t read;
            while ((read = fread(inBlock, 1, 16, file)) > 0) {
                ASSERT(read <= 16);
                aes_decrypt(inBlock, outBlock, aesKey, SIZE_16);
                fwrite(outBlock, 1, 16, intermediate);
            }
        } else {
            fprintf(stderr, "[DECODE] Copying input file as-is to intermediate...\n");
            char buf[1024];
            size_t read;
            while ((read = fread(buf, 1, sizeof(buf), file)) > 0) {
                fwrite(buf, 1, read, intermediate);
            }
        }

        fclose(file);
        rewind(intermediate);
        fseek(intermediate, 0, SEEK_END);
        long size = ftell(intermediate);
        fseek(intermediate, 0, SEEK_SET);  // rewind again
        fprintf(stderr, "[DEBUG] Intermediate file size: %ld bytes\n", size);
        file = fopen(filenameOut, "wb");
        ASSERT(file != NULL);

        bitF = bitIO_open(intermediate, BIT_IO_R);
        ASSERT(bitF != NULL);
        ASSERT(bitF->mode == BIT_IO_R);

        fprintf(stderr, "[DECODE] Starting decompression...\n");
        decode(bitF, file);
        bitIO_close(bitF);
        fclose(file);
        fprintf(stderr, "[DECODE] Done.\n");

    } else {
        fprintf(stderr, "[ERROR] Must specify encode (-c) or decode (-d)\n");
        goto error;
    }

    free(filenameIn);
    free(filenameOut);
    free(aesKeyStr);
    return 0;

error:
    fprintf(stderr, "[FATAL] Aborting with error.\n");
    if (file) fclose(file);
    if (intermediate) fclose(intermediate);
    if (bitF) bitIO_close(bitF);
    if (filenameIn) free(filenameIn);
    if (filenameOut) free(filenameOut);
    if (aesKeyStr) free(aesKeyStr);
    return 1;
}
