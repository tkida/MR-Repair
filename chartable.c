#include <stdio.h>
#include <string.h>
#include "chartable.h"

#ifndef LESS_MEM
static unsigned char num[] = {
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};
#endif

void chartable_init(USEDCHARTABLE *ut)
{
  memset(ut->table, 0, 32);
  ut->size = 0;
}

void chartable_set(USEDCHARTABLE *ut, unsigned char c)
{
  unsigned int ind = c / 8;
  unsigned int sft = c % 8;
  unsigned char msk = 1 << sft;
  if(!(ut->table[ind] & msk)) ut->size++;
  ut->table[ind] |= msk;
}

void chartable_write(USEDCHARTABLE *ut, FILE *fp)
{
  fwrite(ut->table, 1, 32, fp);
}

void chartable_read(USEDCHARTABLE *ut, FILE *fp)
{
  fread(ut->table, 1, 32, fp);
  unsigned int i;
#ifdef LESS_MEM
  unsigned int s;
  for(i = 0; i < 32; i++){
    s = ut->table[i];
    s = (s & 0x55) + ((s >> 1) & 0x55);
    s = (s & 0x33) + ((s >> 2) & 0x33);
    s = (s & 0x0F) + ((s >> 4) & 0x0F);
    ut->size += s;
  }
#else
  for(i = 0; i < 32; i++){
    ut->size += num[ut->table[i]];
  }
#endif
}

unsigned int chartable_test(USEDCHARTABLE *ut, unsigned char c)
{
  unsigned int ind = c / 8;
  unsigned int sft = c % 8;
  unsigned char msk = 1 << sft;
  return ut->table[ind] & msk;
}
  
