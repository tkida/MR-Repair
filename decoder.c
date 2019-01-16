/* 
 *  Copyright (c) 2011 Shirou Maruyama
 * 
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 * 
 *   1. Redistributions of source code must retain the above Copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above Copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 */


#include "decoder.h"
#include <stdio.h>

#define BUFF_SIZE 32768
char buffer[BUFF_SIZE];
char table[CHAR_SIZE];
uint bufpos = 0;

uint bits (uint n);
//void expandLeaf(RULE *rule, CODE code, FILE *output, USEDCHARTABLE *ut);

uint bits (uint n)
{ uint b = 0;
  while (n)
    { b++; n >>= 1; }
  return b;
}

void expandLeaf(RULE *rule, CODE leaf, FILE *output, USEDCHARTABLE *ut, uint *len) {
  if (leaf < ut->size) {
    buffer[bufpos] = rule[leaf].left;
    (*len)++;
    bufpos++;
    if (bufpos == BUFF_SIZE) {
      fwrite(buffer, 1, BUFF_SIZE, output);
      bufpos = 0;
    }
    return;
  }
  else {
    expandLeaf(rule, rule[leaf].left, output, ut, len);
    expandLeaf(rule, rule[leaf].right, output, ut, len); 
    return;
  }
}

//この関数書き換え
void DecodeCFG(FILE *input, FILE *output) {
  uint i;
  RULE *rule;
  uint num_rules, txt_len, seq_len;
  BITIN *bitin;
  IBITFS ibfs;
  uint exc, sp;
  CODE *stack;
  CODE newcode, leaf;
  uint cod;
  uint bitlen;
  uint currentlen;
  bool paren;
  uint width = 1; // warning: 後で決める必要がある．
  USEDCHARTABLE ut;

  chartable_init(&ut);
  fread(&txt_len, sizeof(uint), 1, input);
  fread(&num_rules, sizeof(uint), 1, input);
  fread(&seq_len, sizeof(uint), 1, input);
  printf("txt_len = %d, num_rules = %d, seq_len = %d\n", 
	 txt_len, num_rules, seq_len);
  chartable_read(&ut, input);

  // width 決定する．
  width = ut.size + num_rules - CHAR_SIZE;
  if(width >= 1) width = ceil(log(width)/log(2.0));
  //width |= width >> 1;
  //width |= width >> 2;
  //width |= width >> 4;
  //width |= width >> 8;
  //width |= width >> 16;
  //width = (width & 0x55555555) + ((width >>  1) & 0x55555555);
  //width = (width & 0x33333333) + ((width >>  2) & 0x33333333);
  //width = (width & 0x0F0F0F0F) + ((width >>  4) & 0x0F0F0F0F);
  //width = (width & 0x00FF00FF) + ((width >>  8) & 0x00FF00FF);
  //width = (width & 0x0000FFFF) + ((width >> 16) & 0x0000FFFF);

  // rule初期値の読み込み
  rule = (RULE*)malloc(sizeof(RULE)*num_rules);
  uint j = 0;
  for (i = 0; i < CHAR_SIZE; i++) {
    if(chartable_test(&ut, (unsigned char)i)){
      rule[j].left = (CODE)i;
      rule[j].right = DUMMY_CODE;
      j++;
    }
  }

  printf("Decoding CFG...");
  fflush(stdout);
  ibitfs_init(&ibfs, input);

  // 各シンボルを読み込んでデコードする
  for(i = ut.size; i < ut.size + num_rules - CHAR_SIZE; i++){
    rule[i].left  = ibitfs_get(&ibfs, width);
    rule[i].right = ibitfs_get(&ibfs, width);
  }
  currentlen = 0;
//  for(i = 0; i < seq_len; i++){
  while(currentlen < txt_len){
    cod = ibitfs_get(&ibfs, width);
    expandLeaf(rule, (CODE) cod, output, &ut, &currentlen);
  }
  fwrite(buffer, 1, bufpos, output);
  printf("Finished!\n");
  free(rule);
}

