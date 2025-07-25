// lz77.c
/***************************************************************************
 *          Lempel, Ziv Encoding and Decoding
 *
 *   File    : lz77.c
 *
 ***************************************************************************/

/***************************************************************************
 *                             INCLUDED FILES
 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitio.h"
#include "debug.h"
#include "tree.h"

#define DEFAULT_LA_SIZE 15
#define DEFAULT_SB_SIZE 4095
#define N 3
#define MAX_BIT_BUFFER 16

struct token {
    int off, len;
    char next;
};

void writecode(struct token t, struct bitFILE *out, int la_size, int sb_size);
struct token readcode(struct bitFILE *file, int la_size, int sb_size);
struct token match(struct node *tree, int root, unsigned char *window, int la, int la_size);

void encode(FILE *file, struct bitFILE *out, int la, int sb) {
    ASSERT(file != NULL);
    ASSERT(out != NULL);

    int i, root = -1;
    int eof;
    struct node *tree;
    struct token t;
    unsigned char *window;
    int la_size, sb_size = 0;
    int buff_size;
    int sb_index = 0, la_index = 0;
    int LA_SIZE, SB_SIZE, WINDOW_SIZE;

    LA_SIZE = (la == -1) ? DEFAULT_LA_SIZE : la;
    SB_SIZE = (sb == -1) ? DEFAULT_SB_SIZE : sb;
    ASSERT(LA_SIZE > 0);
    ASSERT(SB_SIZE > 0);

    WINDOW_SIZE = (SB_SIZE * N) + LA_SIZE;

    window = calloc(WINDOW_SIZE, sizeof(unsigned char));
    ASSERT(window != NULL);

    tree = createTree(SB_SIZE);
    ASSERT(tree != NULL);

    bitIO_write(out, &SB_SIZE, MAX_BIT_BUFFER);
    bitIO_write(out, &LA_SIZE, MAX_BIT_BUFFER);

    buff_size = fread(window, 1, WINDOW_SIZE, file);
    if (ferror(file)) {
        fprintf(stderr, "Error loading the data in the window.\n");
        free(window);
        destroyTree(tree);
        return;
    }

    eof = feof(file);

    la_size = (buff_size > LA_SIZE) ? LA_SIZE : buff_size;

    while (buff_size > 0) {
        t = match(tree, root, window, la_index, la_size);

        writecode(t, out, LA_SIZE, SB_SIZE);

        for (i = 0; i < t.len + 1; i++) {
            if (sb_size == SB_SIZE) {
                delete(tree, &root, window, sb_index, SB_SIZE);
                sb_index++;
                ASSERT(sb_index <= SB_SIZE * (N - 1));
            } else {
                sb_size++;
            }

            insert(tree, &root, window, la_index, la_size, SB_SIZE);
            la_index++;

            if (eof == 0) {
                if (sb_index == SB_SIZE * (N - 1)) {
                    ASSERT(sb_index + sb_size + la_size <= WINDOW_SIZE);

                    memmove(window, &(window[sb_index]), sb_size + la_size);

                    updateOffset(tree, sb_index, SB_SIZE);

                    sb_index = 0;
                    la_index = sb_size;

                    buff_size += fread(&(window[sb_size + la_size]), 1,
                                       WINDOW_SIZE - (sb_size + la_size), file);

                    if (ferror(file)) {
                        fprintf(stderr, "Error loading the data in the window.\n");
                        free(window);
                        destroyTree(tree);
                        return;
                    }
                    eof = feof(file);
                }
            }

            buff_size--;
            la_size = (buff_size > LA_SIZE) ? LA_SIZE : buff_size;
        }
    }

    destroyTree(tree);
    free(window);
}

void decode(struct bitFILE *file, FILE *out) {
    ASSERT(file != NULL);
    ASSERT(out != NULL);

    struct token t;
    int back = 0, off;
    unsigned char *buffer;
    int SB_SIZE, LA_SIZE, WINDOW_SIZE;

    bitIO_read(file, &SB_SIZE, sizeof(SB_SIZE), MAX_BIT_BUFFER);
    bitIO_read(file, &LA_SIZE, sizeof(LA_SIZE), MAX_BIT_BUFFER);

    ASSERT(SB_SIZE > 0);
    ASSERT(LA_SIZE > 0);

    WINDOW_SIZE = (SB_SIZE * N) + LA_SIZE;

    buffer = calloc(WINDOW_SIZE, sizeof(unsigned char));
    ASSERT(buffer != NULL);

    while (1) {
        t = readcode(file, LA_SIZE, SB_SIZE);

        if (t.off == -1) break;

        if (back + t.len > WINDOW_SIZE - 1) {
            ASSERT(back - SB_SIZE >= 0);
            memcpy(buffer, &(buffer[back - SB_SIZE]), SB_SIZE);
            back = SB_SIZE;
        }

        while (t.len > 0) {
            off = back - t.off;
            ASSERT(off >= 0 && off < WINDOW_SIZE);

            buffer[back] = buffer[off];
            putc(buffer[back], out);

            back++;
            t.len--;
        }
        buffer[back] = t.next;
        putc(buffer[back], out);

        back++;
        ASSERT(back < WINDOW_SIZE * 10);  // Avoid infinite loops — adjust as needed
    }

    free(buffer);
}

struct token match(struct node *tree, int root, unsigned char *window, int la, int la_size) {
    ASSERT(tree != NULL);
    ASSERT(window != NULL);
    ASSERT(la_size >= 0);

    struct token t;
    struct ret r = find(tree, root, window, la, la_size);

    t.off = r.off;
    t.len = r.len;
    t.next = window[la + r.len];  // make sure indexing here is safe

    return t;
}

void writecode(struct token t, struct bitFILE *out, int la_size, int sb_size) {
    ASSERT(out != NULL);
    ASSERT(la_size > 0);
    ASSERT(sb_size > 0);

    bitIO_write(out, &t.off, bitof(sb_size));
    bitIO_write(out, &t.len, bitof(la_size));
    bitIO_write(out, &t.next, 8);
}

struct token readcode(struct bitFILE *file, int la_size, int sb_size) {
    ASSERT(file != NULL);
    ASSERT(la_size > 0);
    ASSERT(sb_size > 0);

    struct token t;
    int ret = 0;

    ret += bitIO_read(file, &t.off, sizeof(t.off), bitof(sb_size));
    ret += bitIO_read(file, &t.len, sizeof(t.len), bitof(la_size));
    ret += bitIO_read(file, &t.next, sizeof(t.next), 8);

    if (ret < (bitof(sb_size) + bitof(la_size) + 8)) {
        if (bitIO_ferror(file) != 0) {
            perror("Error reading bits.\n");
            exit(EXIT_FAILURE);
        }
        t.off = -1;
    }

    return t;
}
