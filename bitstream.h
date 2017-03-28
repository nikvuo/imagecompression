#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdlib.h>
#include <stdio.h>

/* bit stream functions */

typedef struct
{
  FILE *stream;
  int output;
  unsigned char waiting_byte;
  unsigned char mask;
} bitstream;

extern bitstream *
open_input_bitstream(const char *path);

extern bitstream *
open_output_bitstream(const char *path);

extern void
close_bitstream(bitstream *bs);

extern unsigned int
getbits(bitstream *bs, int numbits);

extern void
putbits(bitstream *bs, unsigned int value, int numbits);

/* vlc functions */

extern void
init_huffman_tables(void);

extern void
delete_huffman_tables(void);

extern int
getvlcdc(bitstream *bs);

extern void
getvlcac(bitstream *bs, int *run, int *category);

extern void
putvlcdc(bitstream *bs, int category);

extern void
putvlcac(bitstream *bs, int run, int category);

/* vli functions */

extern int
solve_category(signed int value);

extern signed int
getvli(bitstream *bs, int category);

extern void
putvli(bitstream *bs, int category, signed int value);

#endif
