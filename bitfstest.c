#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "bitfs.h"

int
main()
{
  FILE *fp;
  OBITFS obfs;
  IBITFS ibfs;
  unsigned int i;
  if(NULL == (fp = fopen("test.dat", "wb"))){ exit(EXIT_FAILURE); }
  obitfs_init(&obfs, fp);
  for(i = 3; i < 100; i++){
    obitfs_put(&obfs, i, ceil(log(i+1)/log(2.0)));
  }
  obitfs_finalize(&obfs);
  fclose(fp);
  if(NULL == (fp = fopen("test.dat", "rb"))){ exit(EXIT_FAILURE); }
  ibitfs_init(&ibfs, fp);
  for(i = 3; i < 100; i++){
    printf("%d\n", ibitfs_get(&ibfs, ceil(log(i+1)/log(2.0))));
  }
  fclose(fp);
}
