#pragma once

void safe_scanf(FILE *fp, const char *format, void *var);
#define scanf(fp, f, var) safe_scanf(fp, f, var)

void *safe_malloc(long long n);
#define malloc(n) safe_malloc(n)

void *safe_realloc(void *p, long long n);
#define realloc(p, n) safe_realloc(p, n)
