// Niko Vuorinen 218357
// This code is doing encoding and decoding to gray and rgb pictures
// There is some problems with encoding, probably AC-coding isn't working right
// and I have no idea why it is not working.
// Decoding has some bugs because it prints only white dots.


#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "bitstream.h"
#include "dct.h"

#define ZIGZAGI { 1, 0, 0, 1, 2, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 5, 6, 7, 7, 6, 7}

#define ZIGZAGJ { 0, 1, 2, 1, 0, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 4, 5, 6, 7, 7, 6, 5, 6, 7, 7}

void offset8bit(double **picture, int heigth, int width);
void offsetantibit(double ***picture, int heigth, int width, int rgb);
void block(double **im, int heigth, int width, bitstream *output, int k);
void quant(double **matrix, unsigned int x, unsigned int y, int k);
void zigzag(double **matrix, bitstream *stream);
void rgbstuff(double ***image, int heigth, int width);
void antiblock(double **im, bitstream *stream, int heigth, int width, int k);
void rgbantistuff(double ***image, int heigth, int width);

int main(int argc, char **argv)
{
   //int im[256][256];
   double ***im; // Variable initialization
   double **picture;
   int width = atoi(argv[4]);
   int heigth = atoi(argv[5]);
   int size = 0;
   int rgb = 1;
   unsigned int i = 0;
   unsigned int j = 0;
   unsigned int k = 0;
   char *input = argv[2];
   char *output = argv[3];
   char *command = argv[1];
   char *decode = "decode";
   char *decodergb = "decodergb";
   char *encodergb = "encodergb";
   bitstream *stream;      
   if(strcmp(command,decodergb) == 0 || strcmp(command, encodergb) == 0)
   {
      rgb = 3;
   }      
   //input = argv[2];
   //output = argv[3];
   //width = (int)(argv[4][0] - '0');
   //heigth = (int)(argv[5][0] - '0');
   im = (double***) malloc(width*sizeof(double**)); // 8 bits per pixel
   for(i = 0; i < width; ++i) 
   {
     im[i] = (double**)malloc(heigth*sizeof(double*)); // Now I have matrix
     for(j = 0; j < heigth; ++j)
     {
        im[i][j] = (double*)malloc(rgb*sizeof(double));
     }
   }
   picture = (double**)malloc(width*sizeof(double*));
   for(i = 0; i < width; i++)
   {
      picture[i] = (double*)malloc(width*sizeof(double));
   }
   if(strcmp(command, decode) == 0 || strcmp(command, decodergb) == 0)
   { // Decoder
      stream = open_input_bitstream(input);
      for(k = 0; k < rgb; k++)
      {
         if(k == 1) // Korjaa myöhemmin
         {
            width = (width/2);
            heigth = (heigth/2);
         }
         antiblock(picture, stream, width, heigth, k);
	 for(j = 0; j < heigth; j++)
	 {
	   for(i = 0; i < width; i++)
	   {
	     im[i][j][k] = round(picture[i][j]);
	   }
	 }
      }
      close_bitstream(stream);
      if(rgb == 3)
      {
         rgbantistuff(im, width*2, heigth*2); // Two times because the size is "halfed" 
      }
      stream = open_output_bitstream(output);
      offsetantibit(im, heigth, width, rgb);
      for(j = 0; j < heigth; j++)
      {
         for(i = 0; i < width; i++)
         {
            for(k = 0; k < rgb; k++)
            {
               putbits(stream, (int)im[i][j][k], 8); // Finding bits from $
            }
         }
      }
      close_bitstream(stream);
      free(im);
      free(picture);
      return 0;
   }
   stream = open_input_bitstream(input);
   for(j = 0; j < heigth; j++)
   {   
        for(i = 0; i < width; i++)
        {
            for(k = 0; k < rgb; k++)
            {
	            im[i][j][k] = (double)getbits(stream, 8); // Finding bits from picture
            }
        }
   }
   close_bitstream(stream); // Freeing memory
   if(rgb == 3) // Värikuvien juttuja
   {
      rgbstuff(im, heigth, width);
   }
   stream = open_output_bitstream(output);
   for(k = 0; k < rgb; k++)
   {
      for(j = 0; j < heigth; j++)
      {
         for(i = 0; i < width; i++)
         { // Sijoitetaan i ja j tonne, tod.näk. toimi oikein
            picture[i][j] = im[i][j][k];
         }
      }
      if(k == 1)
      { // Puolitetaan koko kun koko on puolikas
         width = width/2;
         heigth = heigth/2;
      }           
      offset8bit(picture, heigth, width); // Doing offset with void
      block(picture, heigth, width, stream, k);
   }
   close_bitstream(stream);
   free(im); // Freeing memory
   free(picture);
   return 0;
}

void offsetantibit(double ***picture, int heigth, int width, int rgb)
{
   unsigned int i,j,k = 0;
   for(j=0;j < heigth;j++)
   {
      for(i=0;i < width;i++)
      {
         for(k=0;k < rgb;k++)
         {
            picture[i][j][k] = picture[i][j][k] + 128;
         }
      }
   }
}

void offset8bit(double **picture, int heigth, int width)
{
   unsigned int i = 0;
   unsigned int j = 0;
   
   for(j = 0; j < heigth; ++j)
   {
      for(i = 0; i < width; ++i)
      {
	    picture[i][j] = picture[i][j] - 128; // I guess this is right?
      }
   }
}

void rgbstuff(double ***picture, int heigth, int width)
{
   int i,j,k = 0;
   for(j=0; j < heigth; j++)
   {
      for(i=0; i < width; i++)
      {
         picture[i][j][0] = 0.299 * picture[i][j][0] + 0.587* picture[i][j][1]
           + 0.114*picture[i][j][2];
         picture[i][j][1] = (-0.1687) * picture[i][j][0] + (-0.3313) * 
           picture[i][j][1] + 0.5 * picture[i][j][2];
         picture[i][j][2] = 0.5 * picture[i][j][0] + (-0.4187) * 
           picture[i][j][1] + (-0.0813) * picture[i][j][2];
      }     
   }
   for(k=1; k < 3; k++)
   {
      for(j=0; j < (heigth/2); j++)
      {
         for(i=0; i < (width/2); i++)
         {
            picture[i][j][k] = (picture[2*i][2*j][k] + picture[2*i+1][2*j][k]
              + picture[2*i][2*j+1][k] + picture[2*i+1][2*j+1][k])/4;
         }
      }
   }
}

void quant(double **matrix, unsigned int x, unsigned int y, int k)
{
   //int quant[8][8];
   unsigned int i = 0;
   unsigned int j = 0;
   double qant[8][8] = { // Manually written matrix
      {16, 11, 10, 16, 24, 40, 51, 61}, 
      {12, 12, 14, 19, 26, 58, 60, 55},
      {14, 13, 16, 24, 40, 57, 69, 56 }, 
      {14, 17, 22, 29, 51, 87, 80, 62}, 
      {18, 22, 37, 56, 68, 109, 103, 77}, 
      {24, 35, 55, 64, 81, 104, 113, 92}, 
      {49, 64, 78, 87, 103, 121, 120, 101}, 
      {72, 92, 95, 98, 112, 100, 103, 99} };
   double qant2[8][8] = {
      {17, 18, 24, 47, 99, 99, 99, 99},
      {18, 21, 26, 66, 99, 99, 99, 99},
      {24, 26, 56, 99, 99, 99, 99, 99},
      {47, 66, 99, 99, 99, 99, 99, 99},
      {99, 99, 99, 99, 99, 99, 99, 99},
      {99, 99, 99, 99, 99, 99, 99, 99},
      {99, 99, 99, 99, 99, 99, 99, 99},
      {99, 99, 99, 99, 99, 99, 99, 99} };     
   if(k == 0)
   {
        for(j = 0; j < 8; j++)
        {
            for(i = 0; i < 8; i++)
            { // (int) should do the round-operation
	            matrix[i][j] = round((matrix[i][j] / qant[i][j]));
            }
        }
   }
   else
   {
      for(j=0;j < 8; j++)
      {
         for(i=0;i < 8; i++)
         {
            matrix[i][j] = round((matrix[i][j] / qant2[i][j]));
         }
      }
   }
}

void zigzag(double **matrix, bitstream *stream)
{
   int i = 0; // Initialization
   int iindex[63] = ZIGZAGI;
   int jindex[63] = ZIGZAGJ;
   int run = 0;
   int cat = 0;
   for(i=0;i < 63; i++)
   { // Kaikki bitit kivasti läpi
      cat = matrix[iindex[i]][jindex[i]];
      if(cat != 0)
      {
         while(15 < run)
         {
            putvlcac(stream, 15, solve_category(0));
            putvli(stream, solve_category(0), 0);
            run = run - 15;
         }
         putvlcac(stream, run, solve_category(cat));
         putvli(stream, solve_category(cat), cat);
         run = 0;
         continue;
      }
      run++;
   }
   if(matrix[7][7] == 0)
   {
      putvlcac(stream, 0, solve_category(0));
      putvli(stream, solve_category(0), 0);
   }   
}

void block(double **im, int heigth, int width, bitstream *stream, int k)
{
   unsigned int R = 8; // Initialization
   unsigned int C = 8;
   double rivi[R*C];
   double line[R*C];
   double **matrix;
   unsigned int x,y = 0;
   unsigned int i,j = 0;
   unsigned int m,n = 0;
   int dcprevious = 0;
   int dcnow = 0;
   int diff = 0;
   int categorydc = 0;
   matrix = (double**) calloc(8,sizeof(double*)); // 8 bits per pixel
   for(i = 0; i < 8; ++i)
   {
      matrix[i] = (double*)malloc(8*sizeof(double));
   }
   init_huffman_tables();
    // Initializes Huffman tables
   for(y = 0; y < heigth; y = y + 8) // for each 8x8 block
   {
      for(x = 0; x < width; x = x + 8)
      {
         for(j = 0; j < 8; j++) // Initialize matrix
	     {
            for(i = 0; i < 8; i++)
	        {
               matrix[i][j] = im[x+i][y+j];
	        }
	     }
         for(m = 0; m < 8; m++)
         {
            for(n = 0; n < 8; n++)
            {
               rivi[n+m*8] = matrix[n][m]; // As fdct doesnt work with matrix
               line[n+m*8] = 0;
            }
         }
         fdct(rivi, line);
         for(m = 0; m < 8; m++)
         {
            for(n = 0;n < 8; n++)
            {
               matrix[n][m] = line[n+m*8]; // Back to matrix
            }
         }
         quant(matrix, R, C, k); // Quantization
         dcnow = (int)matrix[0][0];
         diff = dcnow - dcprevious; // Calculating DC
         dcprevious = dcnow;
         categorydc = solve_category(diff);
         putvlcdc(stream, categorydc); // Save's file Tähän asti toimii
         putvli(stream, categorydc, diff);
         zigzag(matrix, stream);  // Calculation AC Mikä vittu tässä on
      }
   }
   delete_huffman_tables();
}

void antiquant(double **matrix, int k)
{
   unsigned int i = 0;
   unsigned int j = 0;
   double qant[8][8] = {  // Manually written matrix
      {16, 11, 10, 16, 24, 40, 51, 61},
      {12, 12, 14, 19, 26, 58, 60, 55},
      {14, 13, 16, 24, 40, 57, 69, 56 },
      {14, 17, 22, 29, 51, 87, 80, 62},
      {18, 22, 37, 56, 68, 109, 103, 77},
      {24, 35, 55, 64, 81, 104, 113, 92},
      {49, 64, 78, 87, 103, 121, 120, 101},
      {72, 92, 95, 98, 112, 100, 103, 99} };
   double qant2[8][8] = {
      {17, 18, 24, 47, 99, 99, 99, 99},
      {18, 21, 26, 66, 99, 99, 99, 99},
      {24, 26, 56, 99, 99, 99, 99, 99},
      {47, 66, 99, 99, 99, 99, 99, 99},
      {99, 99, 99, 99, 99, 99, 99, 99},
      {99, 99, 99, 99, 99, 99, 99, 99},
      {99, 99, 99, 99, 99, 99, 99, 99},
      {99, 99, 99, 99, 99, 99, 99, 99} };
   if(k == 0)
   {
      for(j = 0; j < 8; j++)
      {
         for(i = 0; i < 8; i++)
         {  // (int) should do the round-operation
            matrix[i][j] = floor((matrix[i][j] * qant[i][j])+0.5);
         }
      }
   }
   else
   { 
      for(j=0;j < 8; j++)
      {
         for(i=0;i < 8; i++)
         {
            matrix[i][j] = floor((matrix[i][j] * qant2[i][j])+0.5);
         }
      }
   }
}

void rgbantistuff(double ***image, int heigth, int width)
{
   int i,j,k = 0;
   for(k=1;k < 3; k++)
   {
      for(j=0; j < (heigth/2); j++)
      {
         for(i=0; i < (width/2); i++)
         {
            image[width-1-i*2][heigth-1-j*2][k] = 
              image[(heigth/2)-1*i][(width/2)-1*j][k];
            image[width-1-i*2][heigth-1-j*2][k] = 
              image[(heigth/2)-1*i][(width/2)-1*j][k];
            image[width-1-i*2][heigth-1-j*2][k] = 
              image[(heigth/2)-1*i][(width/2)-1*j][k];
            image[width-1-i*2][heigth-1-j*2][k] = 
              image[(heigth/2)-1*i][(width/2)-1*j][k];
         }
      }
   }
   for(j=0; j < heigth; j++)
   {
      for(i=0; i < width; i++)
      {
         image[i][j][0] = 1 * image[i][j][0] + 0 * image[i][j][1] + 1.402 * 
           image[i][j][2];
         image[i][j][1] = 1 * image[i][j][0] + (-0.34414) * image[i][j][1] + 
           (-0.71414) * image[i][j][2];
         image[i][j][2] = 1 * image[i][j][0] + 1.772 * image[i][j][1] + 0 * 
           image[i][j][2];
      }
   }
}       
      
void antiblock(double **im, bitstream *stream, int heigth, int width, int k)
{
   int i,j, n, m = 0;
   int value = 0;
   int diff = 0;
   int *run;
   int *cat;
   int vli;
   int muistiluku = 0;
   int previous = 0;
   int iindex[63] = ZIGZAGI;
   int jindex[63] = ZIGZAGJ;
   double **matrix;
   double rivi[64];
   double line[64];
   matrix = (double**) calloc(8,sizeof(double*)); // 8 bits per pixel
   for(i = 0; i < 8; ++i)
   {
      matrix[i] = (double*)malloc(8*sizeof(double));
   }   
   init_huffman_tables();
   run = (int*)malloc(sizeof(int));
   cat = (int*)malloc(sizeof(int));
   for(j=0; j < heigth; j = j+8)
   {
      for(i=0; i < width; i = i+8)
      {
         muistiluku = 0;
         value = getvlcdc(stream);
         diff = getvli(stream, solve_category(value));
         im[i][j] = diff + previous;
         previous = im[i][j];
         getvlcac(stream, run, cat);
         vli = getvli(stream, solve_category(*cat));
         muistiluku = *run + muistiluku;
         im[i+iindex[muistiluku]][i+jindex[muistiluku]] = vli;
         while(*run != 0 && *cat != 0)
         { 
            getvlcac(stream, run, cat);
            vli = getvli(stream, solve_category(*cat));
            muistiluku = *run + muistiluku;
            im[i+iindex[muistiluku]][i+jindex[muistiluku]] = vli;
         }
	 antiquant(matrix,k);
         for(m = 0; m < 8; m++)
         {
            for(n = 0; n < 8; n++)
            {
               matrix[n][m] = im[i+n][i+m]; 
               rivi[n+m*8] = matrix[n][m]; // As fdct doesnt work with matrix
            }
         }
         for(m = 0; m < 8; m++)
         {
            for(n = 0; n < 8; n++)
            {
                line[n+m*8] = 0;
            }
         }
         idct(rivi, line);
         for(m = 0; m < 8; m++)
         {
            for(n = 0;n < 8; n++)
            {
                matrix[n][m] = line[n+m*8]; // Back to matrix
                im[n+i][m+j] = matrix[n][m];
            }
         }
      }
   }
   free(cat);
   free(run);
   free(matrix);
   delete_huffman_tables();
}
