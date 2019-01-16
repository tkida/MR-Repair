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

#ifndef REPAIR_H
#define REPAIR_H

#include "basics.h"



typedef struct _rule {
  CODE *symbols;
} RULE;

typedef struct _dictionary {
  uint text_length;
  uint num_rules;
  uint num_usedrules;
  uint size_rules;
  RULE *rule;
  uint seq_length;
  CODE *comp_seq;
  uint buff_size;
} DICT;



DICT *CreateDictByRepair(FILE *input);
void DestructDict(DICT *dict);
void OutputDict(FILE *output, DICT *dict);
DICT *RestoreDict(FILE *input);
void OutputText(FILE *input, FILE *output, DICT *dict);

#endif
