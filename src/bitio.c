// bitio.c
/***************************************************************************
 *          Lempel, Ziv Encoding and Decoding
 *
 *   File    : bitio.c
 *
 ***************************************************************************/
/***************************************************************************
 *                             INCLUDED FILES
 ***************************************************************************/
#include "bitio.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

#define BIT_IO_BUFFER 4096

struct bitFILE {
    FILE *file;
    int mode;
    int bytepos;
    int bitpos;
    int read;
    unsigned char *buffer;
};

int bitof(int n) {
    ASSERT(n > 0);  // can't compute log(0) or negative!
    return (int)(ceil(log(n) / log(2)));
}

int bitIO_feof(struct bitFILE *bitF) {
    ASSERT(bitF != NULL);
    ASSERT(bitF->file != NULL);

    if (feof(bitF->file) && bitF->bytepos == bitF->read) return 1;
    return 0;
}

int bitIO_ferror(struct bitFILE *bitF) {
    ASSERT(bitF != NULL);
    ASSERT(bitF->file != NULL);

    return ferror(bitF->file);
}

int write_buffer(struct bitFILE *bitF) {
    ASSERT(bitF != NULL);
    ASSERT(bitF->buffer != NULL);
    ASSERT(bitF->file != NULL);

    int ret = fwrite(bitF->buffer, 1, bitF->bytepos, bitF->file);
    if (ret != bitF->bytepos) {
        perror("Error writing buffer");
        return -1;
    }
    bitF->bytepos = 0;
    bitF->bitpos = 0;
    memset(bitF->buffer, 0, BIT_IO_BUFFER);
    return 0;
}

int read_buffer(struct bitFILE *bitF) {
    ASSERT(bitF != NULL);
    ASSERT(bitF->buffer != NULL);
    ASSERT(bitF->file != NULL);

    bitF->read = fread(bitF->buffer, 1, BIT_IO_BUFFER, bitF->file);
    if (bitF->read < BIT_IO_BUFFER && ferror(bitF->file)) {
        return -1;
    }
    bitF->bytepos = 0;
    bitF->bitpos = 0;
    return 0;
}

struct bitFILE *bitIO_open(FILE *file, int mode) {
    ASSERT(file != NULL);
    ASSERT(mode == BIT_IO_W || mode == BIT_IO_R);

    struct bitFILE *bitF = calloc(1, sizeof(struct bitFILE));
    ASSERT(bitF != NULL);

    bitF->mode = mode;
    bitF->bytepos = 0;
    bitF->bitpos = 0;
    bitF->file = file;

    bitF->buffer = calloc(BIT_IO_BUFFER, sizeof(unsigned char));
    ASSERT(bitF->buffer != NULL);

    if (mode == BIT_IO_R) {
        int res = read_buffer(bitF);
        if (res == -1) {
            fprintf(stderr, "Failed to read initial buffer.\n");
            free(bitF->buffer);
            free(bitF);
            return NULL;
        }
    }
    return bitF;
}

int bitIO_close(struct bitFILE *bitF) {
    if (bitF == NULL || bitF->file == NULL) return -1;

    if (bitF->mode == BIT_IO_W) {
        if (bitF->bitpos > 0) {
            bitF->bytepos++;
        }
        if (write_buffer(bitF) != 0) {
            fprintf(stderr, "Failed to write buffer on close.\n");
        }
    }
    fclose(bitF->file);
    free(bitF->buffer);
    free(bitF);

    return 0;
}

int bitIO_write(struct bitFILE *bitF, void *info, int nbit) {
    ASSERT(bitF != NULL);
    ASSERT(bitF->file != NULL);
    ASSERT(bitF->mode == BIT_IO_W);
    ASSERT(info != NULL);
    ASSERT(nbit >= 0);

    int i;
    int byte_pos = 0, bit_pos = 0;
    unsigned char mask;
    unsigned char *input = (unsigned char *)info;

    for (i = 0; i < nbit; i++) {
        mask = 1 << bit_pos;

        if ((input[byte_pos] & mask) != 0) {
            bitF->buffer[bitF->bytepos] |= (1 << bitF->bitpos);
        }

        bit_pos++;
        if (bit_pos == 8) {
            bit_pos = 0;
            byte_pos++;
        }

        bitF->bitpos++;
        if (bitF->bitpos == 8) {
            bitF->bitpos = 0;
            bitF->bytepos++;
            if (bitF->bytepos == BIT_IO_BUFFER) {
                int res = write_buffer(bitF);
                (void)res;
                ASSERT(res == 0);  // flush should succeed
            }
        }

        if (bitIO_ferror(bitF) != 0) break;
    }
    return i;
}

int bitIO_read(struct bitFILE *bitF, void *info, int info_s, int nbit) {
    ASSERT(bitF != NULL);
    ASSERT(bitF->file != NULL);
    ASSERT(bitF->mode == BIT_IO_R);
    ASSERT(info != NULL);
    ASSERT(info_s > 0);
    ASSERT(nbit >= 0);

    memset(info, 0, info_s);

    int i;
    int byte_pos = 0, bit_pos = 0;
    unsigned char mask;

    for (i = 0; i < nbit && (bitIO_feof(bitF) != 1); i++) {
        mask = 1 << bitF->bitpos;

        if ((bitF->buffer[bitF->bytepos] & mask) != 0) {
            *(unsigned char *)(info + byte_pos) |= (1 << bit_pos);
        }

        byte_pos = (bit_pos < 7) ? byte_pos : (byte_pos + 1);
        bit_pos = (bit_pos < 7) ? (bit_pos + 1) : 0;

        bitF->bytepos = (bitF->bitpos < 7) ? bitF->bytepos : (bitF->bytepos + 1);
        bitF->bitpos = (bitF->bitpos < 7) ? (bitF->bitpos + 1) : 0;

        if (bitF->bytepos == BIT_IO_BUFFER) {
            int res = read_buffer(bitF);
            (void)res;
            ASSERT(res == 0);  // buffer refill must succeed
        }

        if (bitIO_ferror(bitF) != 0) break;
    }

    return i;
}
