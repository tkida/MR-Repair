#ifndef CHARTABLE_H
#define CHARTABLE_H

typedef struct _used_chartable
{
  unsigned char table[32];
  unsigned int size;
} USEDCHARTABLE;

void chartable_init(USEDCHARTABLE *ut);
void chartable_set(USEDCHARTABLE *ut, unsigned char c);
void chartable_write(USEDCHARTABLE *ut, FILE *fp);
void chartable_read(USEDCHARTABLE *ut, FILE *fp);
unsigned int chartable_test(USEDCHARTABLE *ut, unsigned char c);


#endif
