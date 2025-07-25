// lz77.h
/***************************************************************************
 *          Lempel, Ziv Encoding and Decoding
 *
 *   File    : lz77.h
 *
 ***************************************************************************/

/***************************************************************************
 *                         FUNCTIONS DECLARATION
 ***************************************************************************/
#ifndef lz77_h
#define lz77_h
void encode(FILE *file, struct bitFILE *out, int la, int sb);
void decode(struct bitFILE *file, FILE *out);
#endif
