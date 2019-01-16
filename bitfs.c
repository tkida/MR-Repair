#include <string.h>
#include <stdlib.h>
#include "bitfs.h"

void writebuf(OBITFS *bfs);
void readbuf(IBITFS *bfs);

void obitfs_init(OBITFS *bfs, FILE *fp)
{
  bfs->fp = fp;
  memset(bfs->buf, 0, BFS_BUFFER_SIZE);
  bfs->pos = 0;
  bfs->bpos = 0;
}

int obitfs_put(OBITFS *bfs, unsigned int data, unsigned int len)
{
  unsigned int d;
  unsigned int t;
#ifdef NDEBUG
  len = (len - 1) & 0x0000001F;
  len++;
#else
  if(len > 32 || len == 0){
    fputs("len is greater than 32.\n", stderr);
    exit(EXIT_FAILURE);
  }
#endif
  if(len != 32) data &= (1 << len) - 1;
  t = 8 - bfs->bpos;
  while(t <= len){
    bfs->buf[bfs->pos] |= data >> (len - t);
    len -= t;
    t = 8;
    bfs->bpos = 0;
    bfs->pos++;
    if(bfs->pos == BFS_BUFFER_SIZE) writebuf(bfs);
  }
  if(len){
    bfs->buf[bfs->pos] |= data << (8 - bfs->bpos - len);
    bfs->bpos += len;
  }
}

void writebuf(OBITFS *bfs)
{
  if(bfs->bpos) bfs->pos++;
  fwrite(bfs->buf, 1, bfs->pos, bfs->fp);
  memset(bfs->buf, 0, BFS_BUFFER_SIZE);
  bfs->pos = 0;
  bfs->bpos = 0;
}

void obitfs_finalize(OBITFS *bfs)
{
  writebuf(bfs);
}



void ibitfs_init(IBITFS *bfs, FILE *fp)
{
  bfs -> fp = fp;
  readbuf(bfs);
}

unsigned int ibitfs_get(IBITFS *bfs, unsigned int len)
{
  unsigned int d = 0;
  unsigned int t = 8 - bfs->bpos;
#ifdef NDEBUG
  len = (len - 1) & 0x0000001F;
  len++;
#else
  if(len > 32 || len == 0){
    fputs("len is greater than 32.", stderr);
    exit(EXIT_FAILURE);
  }
#endif
  while(t <= len){
    d <<= t;
    d |= bfs->buf[bfs->pos] & ((1 << t) - 1);
    bfs->pos++;
    if(bfs->pos == bfs->rest) readbuf(bfs);
    len -= t;
    bfs->bpos = 0;
    t = 8;
  }
  if(len){
    d <<= len;
    d |= (bfs->buf[bfs->pos] >> (8 - bfs->bpos - len)) & ((1 << len) - 1);
    bfs->bpos += len;
  }
  return d;
}

void readbuf(IBITFS *bfs)
{
  bfs->rest = fread(bfs->buf, 1, BFS_BUFFER_SIZE, bfs->fp);
  bfs->pos = 0;
  bfs->bpos = 0;
}

void ibitfs_finalize(IBITFS *bfs){}
