###########################################
# Makefile Image and Video Processing Techniques project work 1
#

OBJECTS = main.o dct.o bitstream.o

todo: $(OBJECTS)
	gcc -o icodec $(OBJECTS) -lm

main.o: main.c bitstream.h dct.h
	gcc -c main.c

bitstream.o: bitstream.c bitstream.h
	gcc -c bitstream.c

dct.o: dct.c dct.h
	gcc -c dct.c

