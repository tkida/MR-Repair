# Makefile

SRCS	= repair.c cfg2txt.c decoder.c bits.c chartable.c bitfs.c
REPAIR	= mrrep despair
CFG	= txt2cfg cfg2txt cfg2enc txt2enc enc2txt

OBJS	= $(SRCS:%.c=%.o)
CC	= gcc
CFLAGS	= -O3 -DNDEBUG
//CFLAGS	= -g -Wall -DNDEBUG
LIB	= -lm

all: $(REPAIR)

cfg: $(CFG)

mrrep: main.c repair.o
	$(CC) $(CFLAGS) -DREPAIR -o $@ main.c repair.o $(LIB)

despair: main.c repair.o
	$(CC) $(CFLAGS) -DDESPAIR -o $@ main.c repair.o $(LIB)

txt2enc: main.c repair.o encoder.o bits.o
	$(CC) $(CFLAGS) -DTXT2ENC -o $@ main.c repair.o encoder.o bits.o $(LIB)

enc2txt: main.c decoder.o bits.o
	$(CC) $(CFLAGS) -DENC2TXT -o $@ main.c decoder.o bits.o $(LIB)

txt2cfg: main.c repair.o
	$(CC) $(CFLAGS) -DTXT2CFG -o $@ main.c repair.o $(LIB)

cfg2txt: cfg2txt.o
	$(CC) $(CFLAGS) -o $@ cfg2txt.o $(LIB)

cfg2enc: main.c encoder.o bits.o
	$(CC) $(CFLAGS) -DCFG2ENC -o $@ main.c encoder.o bits.o $(LIB)

clean:
	-rm -f $(REPAIR) $(CFG) $(OBJS)  *~

.c.o:
	$(CC) $(CFLAGS) -c $<

repair.o: repair.h basics.h common.h
cfg2txt.o: basics.h
encoder.o: encoder.h basics.h
decoder.o: decoder.h basics.h
bits.o: bits.h basics.h
bitfs.o: bitfs.h
