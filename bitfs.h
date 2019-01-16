#ifndef BITFS_H
#define BITFS_H
#include <stdio.h>

//static const unsigned int BFS_BUFFER_SIZE = BUFSIZ;
#define BFS_BUFFER_SIZE BUFSIZ

typedef struct _ibfs
{
  FILE *fp;
  unsigned char buf[BFS_BUFFER_SIZE];
  unsigned int pos;
  unsigned char bpos;
  unsigned int rest;
} IBITFS;

typedef struct _obfs
{
  FILE *fp;
  unsigned char buf[BFS_BUFFER_SIZE];
  unsigned int pos;
  unsigned char bpos;
} OBITFS;

void obitfs_init(OBITFS *bfs, FILE *fp);
int obitfs_put(OBITFS *bfs, unsigned int data, unsigned int len);
void obitfs_finalize(OBITFS *bfs);
void ibitfs_init(IBITFS *bfs, FILE *fp);
unsigned int ibitfs_get(IBITFS *bfs, unsigned int len);
void ibitfs_finalize(IBITFS *bfs);


#endif
