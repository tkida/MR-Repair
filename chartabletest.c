#include <stdio.h>
#include <stdlib.h>
#include "chartable.h"

int
main()
{
  USEDCHARTABLE uct;
  FILE *fp;
  unsigned char c;
  chartable_init(&uct);
  chartable_set(&uct, 'a');
  chartable_set(&uct, 'b');
  chartable_set(&uct, 'a');
  chartable_set(&uct, 'a');
  chartable_set(&uct, 'b');
  chartable_set(&uct, 'a');
  chartable_set(&uct, 'b');
  chartable_set(&uct, 'a');
  chartable_set(&uct, 'a');
  chartable_set(&uct, 'b');
  chartable_set(&uct, 'a');
  chartable_set(&uct, 'a');
  chartable_set(&uct, 'b');
  //  for(c = 0; c != 255; c++) printf("%c\t%x\n", c, chartable_test(&uct, c));
  printf("%d\n", uct.size);
  if(NULL == (fp = fopen("ctable.dat", "wb"))) exit(EXIT_FAILURE); 
  chartable_write(&uct, fp);
  fclose(fp);
  if(NULL == (fp = fopen("ctable.dat", "rb"))) exit(EXIT_FAILURE);
  chartable_init(&uct);
  chartable_read(&uct, fp);
  //for(c = 0; c != 255; c++) printf("%c\t%x\n", c, chartable_test(&uct, c));
  printf("%d\n", uct.size);
  return 0;
}
