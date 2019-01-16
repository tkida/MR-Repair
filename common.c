#include <stdlib.h>
#include <stdio.h>

//------------------------------------------------------------
void safe_scanf(FILE *fp, const char *format, void *var)
{
  int res = 0;
  res = scanf(fp, format, var);
  if (res = 0) {
	fprintf(stderr, "error: scanf failed\n");
	exit(1);
  }
  return;
}


//------------------------------------------------------------
void *safe_malloc(long long n)
{
  void *p;
  if (n == 0) return NULL;
  p = (void*)malloc(n);
  if (p == NULL) {
	fprintf(stderr, "error: malloc failed\n");
	exit(1);
  }
  return p;
}


//------------------------------------------------------------
void *safe_realloc(void *p, long long n)
{
  if (n == 0) {
	free(p);
	return NULL;
  }
  if (p == NULL) return safe_malloc(n);
  p = (void*)realloc(p,n);
  if (p == NULL) {
	fprintf(stderr, "error: realloc failed\n");
	exit(1);
  }
  return p;
}

