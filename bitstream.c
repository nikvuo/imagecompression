#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bitstream.h"

/* bit stream functions */

bitstream *
open_input_bitstream(const char *path)
{
  bitstream *bs = (bitstream *) malloc(sizeof(bitstream));

  if (bs != (bitstream *) NULL)
    {
      bs->stream = fopen(path,"rb");
      if (bs->stream == (FILE *) NULL)
        {
          free(bs);
          return (bitstream *) NULL;
        }
      bs->output = 0;
      bs->waiting_byte = 0;
      bs->mask = 1;
    }
  return bs;
}

bitstream *
open_output_bitstream(const char *path)
{
  bitstream *bs = (bitstream *) malloc(sizeof(bitstream));

  if (bs != (bitstream *) NULL)
    {
      bs->stream = fopen(path,"wb");
      if (bs->stream == (FILE *) NULL)
        {
          free(bs);
          return (bitstream *) NULL;
        }
      bs->output = 1;
      bs->waiting_byte = 0;
      bs->mask = 0x80;
    }
  return bs;
}

void
close_bitstream(bitstream *bs)
{
  if (bs != (bitstream *) NULL)
    {
      if (bs->output)
	{
	  if (bs->mask != 0x80)
	    {
	      fputc(bs->waiting_byte,bs->stream);
	    }
	}
      fclose(bs->stream);
    }
  free(bs);
}

static int
getbit(bitstream *bs)
{
  if (bs == (bitstream *) NULL)
    {
      printf("TRYING TO READ FROM AN ILLEGAL BIT STREAM!\n");
      fflush(stdout);
      return 0;
    }
  if (bs->mask == 1)
    {
      bs->waiting_byte = fgetc(bs->stream);
      bs->mask = 0x80;
    }
  else
    {
      bs->mask >>= 1;
    }
  return (bs->waiting_byte & bs->mask) != 0;
}

unsigned int
getbits(bitstream *bs, int numbits)
{
  int i;
  unsigned int value = 0;

  for (i = 0; i < numbits; i++)
    {
      value <<= 1;
      if (getbit(bs))
	{
	  value |= 1;
	}
    }
  return value;
}

static void
putbit(bitstream *bs, int value)
{
  if (bs == (bitstream *) NULL)
    {
      printf("TRYING TO WRITE TO AN ILLEGAL BIT STREAM!\n");
      fflush(stdout);
      return;
    }
  if (value)
    {
      bs->waiting_byte |= bs->mask;
    }
  bs->mask >>= 1;
  if (bs->mask == 0)
    {
      fputc(bs->waiting_byte,bs->stream);
      bs->waiting_byte = 0;
      bs->mask = 0x80;
    }
}

void
putbits(bitstream *bs, unsigned int value, int numbits)
{
  unsigned int mask;

  if (numbits <= 0)
    {
      return;
    }
  for (mask = 1 << (numbits - 1); mask != 0; mask >>= 1)
    {
      putbit(bs,(value & mask) != 0);
    }
}

/* huffman tables */

static const char *
dc_code[12] =
{
  "010",
  "011",
  "100",
  "00",
  "101",
  "110",
  "1110",
  "11110",
  "111110",
  "1111110",
  "11111110",
  "111111110"
};

static const char *
ac_code[16][11] =
{
  /* run = 0 */
  {
    "1010",    /* eob */
    "00",
    "01",
    "100",
    "1011",
    "11010",
    "111000",
    "1111000",
    "1111110110",
    "1111111110000010",
    "1111111110000011"
  },
  /* run = 1 */
  {
    "",
    "1100",
    "111001",
    "1111001",
    "111110110",
    "11111110110",
    "1111111110000100",
    "1111111110000101",
    "1111111110000110",
    "1111111110000111",
    "1111111110001000"
  },
  /* run = 2 */
  {
    "",
    "11011",
    "11111000",
    "1111110111",
    "1111111110001001",
    "1111111110001010",
    "1111111110001011",
    "1111111110001100",
    "1111111110001101",
    "1111111110001110",
    "1111111110001111",
  },
  /* run = 3 */
  {
    "",
    "111010",
    "111110111",
    "11111110111",
    "1111111110010000",
    "1111111110010001",
    "1111111110010010",
    "1111111110010011",
    "1111111110010100",
    "1111111110010101",
    "1111111110010110",
  },
  /* run = 4 */
  {
    "",
    "111011",
    "1111111000",
    "1111111110010111",
    "1111111110011000",
    "1111111110011001",
    "1111111110011010",
    "1111111110011011",
    "1111111110011100",
    "1111111110011101",
    "1111111110011110",
  },
  /* run = 5 */
  {
    "",
    "1111010",
    "1111111001",
    "1111111110011111",
    "1111111110100000",
    "1111111110100001",
    "1111111110100010",
    "1111111110100011",
    "1111111110100100",
    "1111111110100101",
    "1111111110100110",
  },
  /* run = 6 */
  {
    "",
    "1111011",
    "11111111000",
    "1111111110100111",
    "1111111110101000",
    "1111111110101001",
    "1111111110101010",
    "1111111110101011",
    "1111111110101100",
    "1111111110101101",
    "1111111110101110",
  },
  /* run = 7 */
  {
    "",
    "11111001",
    "11111111001",
    "1111111110101111",
    "1111111110110000",
    "1111111110110001",
    "1111111110110010",
    "1111111110110011",
    "1111111110110100",
    "1111111110110101",
    "1111111110110110",
  },
  /* run = 8 */
  {
    "",
    "11111010",
    "111111111000000",
    "1111111110110111",
    "1111111110111000",
    "1111111110111001",
    "1111111110111010",
    "1111111110111011",
    "1111111110111100",
    "1111111110111101",
    "1111111110111110",
  },
  /* run = 9 */
  {
    "",
    "111111000",
    "1111111110111111",
    "1111111111000000",
    "1111111111000001",
    "1111111111000010",
    "1111111111000011",
    "1111111111000100",
    "1111111111000101",
    "1111111111000110",
    "1111111111000111",
  },
  /* run = 10 */
  {
    "",
    "111111001",
    "1111111111001000",
    "1111111111001001",
    "1111111111001010",
    "1111111111001011",
    "1111111111001100",
    "1111111111001101",
    "1111111111001110",
    "1111111111001111",
    "1111111111010000",
  },
  /* run = 11 */
  {
    "",
    "111111010",
    "1111111111010001",
    "1111111111010010",
    "1111111111010011",
    "1111111111010100",
    "1111111111010101",
    "1111111111010110",
    "1111111111010111",
    "1111111111011000",
    "1111111111011001",
  },
  /* run = 12 */
  {
    "",
    "1111111010",
    "1111111111011010",
    "1111111111011011",
    "1111111111011100",
    "1111111111011101",
    "1111111111011110",
    "1111111111011111",
    "1111111111100000",
    "1111111111100001",
    "1111111111100010",
  },
  /* run = 13 */
  {
    "",
    "11111111010",
    "1111111111100011",
    "1111111111100100",
    "1111111111100101",
    "1111111111100110",
    "1111111111100111",
    "1111111111101000",
    "1111111111101001",
    "1111111111101010",
    "1111111111101011",
  },
  /* run = 14 */
  {
    "",
    "111111110110",
    "1111111111101100",
    "1111111111101101",
    "1111111111101110",
    "1111111111101111",
    "1111111111110000",
    "1111111111110001",
    "1111111111110010",
    "1111111111110011",
    "1111111111110100",
  },
  /* run = 15 */
  {
    "111111110111",    /* (run=15,cat=0) */
    "1111111111110101",
    "1111111111110110",
    "1111111111110111",
    "1111111111111000",
    "1111111111111001",
    "1111111111111010",
    "1111111111111011",
    "1111111111111100",
    "1111111111111101",
    "1111111111111110",
  }
};

static int
huffman_tables_initialized = 0;

static int
dc_lengths[12];

static unsigned int
dc_table[12];

static int
ac_lengths[16][11];

static unsigned int
ac_table[16][11];

typedef struct vlc_node_s
{
  struct vlc_node_s *symbol0;
  struct vlc_node_s *symbol1;
  int leaf;
  int value;
} node;

static node *
dc_tree = (node *) NULL;

static node *
ac_tree = (node *) NULL;

/* vlc functions */

static int
expand_tree(const char *code, int value, node *tree)
{
  int length = strlen(code);
  node *treepos = tree;
  int k;

  for (k = 0; k < length; k++)
    {
      char symbol = code[k];

      if (symbol == '0')
	{
	  if (treepos->symbol0 == (node *) NULL)
	    {
	      node *new_symbol = (node *) malloc(sizeof(node));

	      new_symbol->symbol0 = (node *) NULL;
	      new_symbol->symbol1 = (node *) NULL;
	      new_symbol->leaf = 0;
	      new_symbol->value = 0;
              treepos->symbol0 = new_symbol;
	    }
	  treepos = treepos->symbol0;
	}
      else
	{
	  if (treepos->symbol1 == (node *) NULL)
	    {
	      node *new_symbol = (node *) malloc(sizeof(node));

	      new_symbol->symbol0 = (node *) NULL;
	      new_symbol->symbol1 = (node *) NULL;
	      new_symbol->leaf = 0;
	      new_symbol->value = 0;
              treepos->symbol1 = new_symbol;
	    }
	  treepos = treepos->symbol1;
	}
    }
  if (treepos->leaf)
    {
      return 1;
    }
  treepos->leaf = 1;
  treepos->value = value;
  return 0;
}

static void
delete_subtree(node *treepos)
{
  if (treepos != (node *) NULL)
    {
      delete_subtree(treepos->symbol0);
      delete_subtree(treepos->symbol1);
      free(treepos);
    }
}

void
init_huffman_tables(void)
{
  int i,j;

  if (huffman_tables_initialized)
    {
      return;
    }
  dc_tree = (node *) malloc(sizeof(node));
  dc_tree->symbol0 = (node *) NULL;
  dc_tree->symbol1 = (node *) NULL;
  dc_tree->leaf = 0;
  dc_tree->value = 0;
  for (i = 0; i < 12; i++)
    {
      dc_lengths[i] = strlen(dc_code[i]);
      dc_table[i] = strtoul(dc_code[i],(char **) NULL,2);
      if (dc_lengths[i] > 0)
	{
          if (expand_tree(dc_code[i],i,dc_tree))
            {
              printf("ERROR IN DC VLC TABLE!\n");
              printf("category = %u\n",i);
              printf("code = %s\n",dc_code[i]);
              fflush(stdout);
            }
	}
    }
  ac_tree = (node *) malloc(sizeof(node));
  ac_tree->symbol0 = (node *) NULL;
  ac_tree->symbol1 = (node *) NULL;
  ac_tree->leaf = 0;
  ac_tree->value = 0;
  for (i = 0; i < 16; i++)
    {
      for (j = 0; j < 11; j++)
	{
	  ac_lengths[i][j] = strlen(ac_code[i][j]);
	  ac_table[i][j] = strtoul(ac_code[i][j],(char **) NULL,2);
	  if (ac_lengths[i][j] > 0)
	    {
              if (expand_tree(ac_code[i][j],(i << 4) | j,ac_tree))
                {
                  printf("ERROR IN AC VLC TABLE!\n");
                  printf("run = %u, category = %u\n",i,j);
                  printf("code = %s\n",ac_code[i][j]);
                  fflush(stdout);
                }
	    }
	}
    }
  huffman_tables_initialized = 1;
}

void
delete_huffman_tables(void)
{
  if (!huffman_tables_initialized)
    {
      return;
    }
  delete_subtree(dc_tree);
  delete_subtree(ac_tree);
  huffman_tables_initialized = 0;
}

static int
getvlc(bitstream *bs, node *tree)
{
  node *treepos = tree;

  if (!huffman_tables_initialized)
    {
      printf("UNINITIALIZED HUFFMAN TABLES!\n");
      fflush(stdout);
      return 0;
    }
  while (!treepos->leaf)
    {
      int bit = getbit(bs);

      if (bit)
	{
	  treepos = treepos->symbol1;
	}
      else
	{
	  treepos = treepos->symbol0;
	}
    }
  if (treepos == (node *) NULL)
    {
      printf("ERROR IN VLC DECODING!\n");
      fflush(stdout);
      return 0;
    }
  return treepos->value;
}

int
getvlcdc(bitstream *bs)
{
  return getvlc(bs,dc_tree);
}

void
getvlcac(bitstream *bs, int *run, int *category)
{
  int value = getvlc(bs,ac_tree);

  *run = value >> 4;
  *category = value & 15;
}

void
putvlcdc(bitstream *bs, int category)
{
  if (!huffman_tables_initialized)
    {
      printf("UNINITIALIZED HUFFMAN TABLES!\n");
      fflush(stdout);
      return;
    }
  if ((category < 0) || (category > 11))
    {
      printf("ILLEGAL DC CATEGORY!\n");
      fflush(stdout);
      return;
    }
  if (dc_lengths[category] <= 0)
    {
      printf("ILLEGAL DC CATEGORY!\n");
      fflush(stdout);
      return;
    }
  putbits(bs,dc_table[category],dc_lengths[category]);
}

void
putvlcac(bitstream *bs, int run, int category)
{
  if (!huffman_tables_initialized)
    {
      printf("UNINITIALIZED HUFFMAN TABLES!\n");
      fflush(stdout);
      return;
    }
  if ((run < 0) || (run > 15) || (category < 0) || (category > 10))
    {
      printf("ILLEGAL AC (RUN,CATEGORY) PAIR!\n");
      fflush(stdout);
      return;
    }
  if (ac_lengths[run][category] <= 0)
    {
      printf("ILLEGAL AC (RUN,CATEGORY) PAIR!\n");
      fflush(stdout);
      return;
    }
  putbits(bs,ac_table[run][category],ac_lengths[run][category]);
}

/* vli functions */

int
solve_category(signed int value)
{
  int absvalue = abs(value);
  int category = 0;

  while (absvalue != 0)
    {
      category++;
      absvalue >>= 1;
    }
  return category;
}

signed int
getvli(bitstream *bs, int category)
{
  unsigned int vli_code;

  if ((category < 0) || (category > 11))
    {
      printf("ILLEGAL VLI CATEGORY!\n");
      fflush(stdout);
      return 0;
    }
  if (category == 0)
    {
      return 0;
    }
  vli_code = getbits(bs,category);
  if (vli_code >= (unsigned int) (1 << (category-1)))
    {
      return (signed int) vli_code;
    }
  else
    {
      return ((signed int) vli_code) - (1 << category) + 1;
    }
}

void
putvli(bitstream *bs, int category, signed int value)
{
  unsigned int vli_code;

  if ((category < 0) || (category > 11))
    {
      printf("ILLEGAL VLI CATEGORY!\n");
      fflush(stdout);
      return;
    }
  if (category == 0)
    {
      return;
    }
  if (value >= 0)
    {
      vli_code = (unsigned int) value;
    }
  else
    {
      vli_code = (unsigned int) (value + (1 << category) - 1);
    }
  putbits(bs,vli_code,category);
}



















