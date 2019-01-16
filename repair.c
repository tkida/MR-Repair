/*

re-pair --  A dictionary-based compression based on the recursive paring.
Copyright (C) 2011-current_year Shirou Maruyama

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Author's contact: Shirou Maruyama, Dept. of Informatics, Kyushu University. 744 Nishi-ku, Fukuoka-shi, Fukuoka 819-0375, Japan. shiro.maruyama@i.kyushu-u.ac.jp

*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
//#include "common.h"
#include "repair.h"


typedef struct _sequence {
  CODE code;
  uint next;
  uint prev;
} SEQ;

typedef struct _pair {
  CODE left;
  CODE right;
  uint freq;
  uint f_pos;
  uint b_pos;
  struct _pair *h_next;
  struct _pair *p_next;
  struct _pair *p_prev;
} PAIR;

typedef struct _maximal_repeat {
  PAIR *origin_pair;
  uint first_pos;
  uint last_pos;
  uint length;
} MAXREP;

typedef struct _repair_data_structures {
  SEQ *seq;
  PAIR **h_first;
  PAIR **p_que;
  uint p_max;
  uint text_length;
  uint num_pairs;
  uint h_num;
} RDS;


static uint expandCode(DICT *dict, CODE code, FILE *output);

static RDS *createRDS(FILE *input);
static void initRDS(RDS *rds);
static void destructRDS(RDS *rds);

static DICT *createDictStructure(uint text_length);
static void getCompSeq(RDS *rds, DICT *dict);

static PAIR *createPair(RDS *rds, CODE left, CODE right, uint f_pos);
static void reconstructPairHash(RDS *rds);
static PAIR *locatePair(RDS *rds, CODE left, CODE right);
static PAIR *getMaxPair(RDS *rds);
static void incrementPair(RDS *rds, PAIR *target);
static void decrementPairAtThePosition(RDS *rds, uint l_pos);
static void destructPair(RDS *rds, PAIR *target);
static void destructAllUniquePairs(RDS *rds);

static void insertPair_PQ(RDS *rds, PAIR *target);
static void removePair_PQ(RDS *rds, PAIR *target);

static uint leftPos_SQ(RDS *rds, uint pos);
static uint rightPos_SQ(RDS *rds, uint pos);
static void removeLink_SQ(RDS *rds, uint target_pos);

static MAXREP findMaximalRepeat(RDS *rds, PAIR *max_pair);
static CODE addMaximalRepeatAsNewRule(RDS *rds, DICT *dict, MAXREP maximal_repeat);
static uint replaceMaximalRepeat(RDS *rds, MAXREP maximal_repeat, CODE new_code);
static void updateBlockToNewcode(RDS *rds, uint leftmost_pos, uint rightmost_pos, uint r_pos, CODE new_code);
static void incrementNewPairs(RDS *rds, uint l_pos, uint block_head_pos, uint r_pos, CODE new_code);



#if true
#define hash_val(P, A, B) (((A)*(B))%primes[P])
#else
uint hash_val(uint h_num, CODE left, CODE right) {
  return (left * right) % primes[h_num];
}
#endif

#ifdef NDEBUG
#define LOG(args...) /* none */
#else
#define LOG(args...) fprintf(stderr, args)
#endif


//=========================================================================================
// Global functions
//
//----------------------------------------------------------------------
DICT *CreateDictByRepair(FILE *input)
{
  RDS  *rds;
  DICT *dict;
  PAIR *max_pair;
  MAXREP maximal_repeat;
  CODE new_code;
  uint cseqlen;

  rds  = createRDS(input);
  dict = createDictStructure(rds->text_length);
  cseqlen = rds->text_length;
  
  printf("Generating CFG..."); fflush(stdout);
  while ((max_pair = getMaxPair(rds)) != NULL) {
	maximal_repeat = findMaximalRepeat(rds, max_pair);
    new_code = addMaximalRepeatAsNewRule(rds, dict, maximal_repeat);
    cseqlen -= replaceMaximalRepeat(rds, maximal_repeat, new_code);
  }

  getCompSeq(rds, dict);
  destructRDS(rds);
  puts("Finished!");

  return dict;
}

//----------------------------------------------------------------------
void DestructDict(DICT *dict)
{
  free(dict->rule); dict->rule = NULL;
  free(dict->comp_seq); dict->comp_seq = NULL;
  free(dict); dict = NULL;
}

//----------------------------------------------------------------------
void OutputDict(FILE *output, DICT *dict)
{
  uint text_length = dict->text_length;
  uint num_rules = dict->num_rules;
  uint size_rules = dict->size_rules;
  uint seq_length = dict->seq_length;
  uint c, i, j;
  
  printf("text_length = %d\n", text_length);
  printf("num_rules = %d\n", num_rules - CHAR_SIZE);
  printf("size_rules = %d\n", size_rules);
  printf("seq_length = %d\n", seq_length);

  fprintf(output, "%u\n", text_length);
  fprintf(output, "%u\n", num_rules - CHAR_SIZE);
  fprintf(output, "%u\n", seq_length);
  for (i = CHAR_SIZE; i < num_rules; i++) {
	for (j = 0; (c = dict->rule[i].symbols[j]) != DUMMY_CODE; j++) {
	  fprintf(output, "%u\n", c);
	}
	fprintf(output, "%d\n", DUMMY_CODE);
  }
  for (i = 0; i < seq_length; i++) {
	fprintf(output, "%u\n", dict->comp_seq[i]);
  }
}

//----------------------------------------------------------------------
DICT *RestoreDict(FILE *input)
{
  DICT *dict;
  RULE *rule;
  uint text_length;
  uint num_rules;
  uint seq_length;
  uint i,j,c;
  uint rule_length;
  CODE buff[1024] = {};
  
  // read header info from input
  fscanf(input, "%d", &text_length);
  fscanf(input, "%d", &num_rules);
  fscanf(input, "%d", &seq_length);
  printf("text_length = %d\n", text_length);
  printf("num_rules = %d\n", num_rules);
  printf("seq_length = %d\n", seq_length);

  // make init dictionary
  dict = (DICT*)malloc(sizeof(DICT));
  dict->text_length = text_length;
  dict->buff_size = num_rules + CHAR_SIZE;
  dict->rule = (RULE*)malloc(sizeof(RULE)*dict->buff_size);
  dict->seq_length = seq_length;
  dict->num_rules = 0;
  dict->size_rules = 0;
  dict->comp_seq = (CODE*)malloc(1); // for dummy
  
  rule = dict->rule;
  for (i = 0; i < CHAR_SIZE; i++) {
	rule[i].symbols = (CODE*)malloc(sizeof(CODE)*2);
    rule[i].symbols[0] = (CODE)i;
    rule[i].symbols[1] = DUMMY_CODE;
    dict->num_rules++;
  }

  // read rules from input
  for (i = CHAR_SIZE; i < CHAR_SIZE + num_rules; i++) {
	dict->num_rules++;
	if (i == 274) {
	  puts("dummy");
	}
	for (j = 0; ;j++) {
	  fscanf(input, "%d", &c);
	  buff[j] = (CODE)c;
	  if (c == DUMMY_CODE) {
		break;
	  }
	}
	rule_length = j;
	dict->size_rules += rule_length;
	rule[i].symbols = (CODE*)malloc(sizeof(CODE)*(rule_length + 1));
	for (j = 0; ;j++) {
	  rule[i].symbols[j] = buff[j];
	  if (buff[j] == DUMMY_CODE) {
		break;
	  }
	}
  }
  
  return dict;
}

//----------------------------------------------------------------------
void OutputText(FILE *input, FILE *output, DICT *dict)
{
  uint seq_length = dict->seq_length;
  uint current_length = 0;
  uint i;
  CODE c;
  
  for (i = 0; i < seq_length; i++) {
	fscanf(input, "%d", &c);
    current_length += expandCode(dict, c, output);
  }
  printf("output %d bytes\n", current_length);
}


//=========================================================================================
// Local functions
//
//----------------------------------------------------------------------
static uint expandCode(DICT *dict, CODE code, FILE *output) {
  uint expanded_length = 0;
  uint i, c;
  
  if (code == 35) {
	puts("");
  }

  if (code < CHAR_SIZE) {
	putc((int)code, output);
	return 1;
  }

  for (i = 0; (c = dict->rule[code].symbols[i]) != DUMMY_CODE; i++) {
	expanded_length += expandCode(dict, c, output);
  }
  return expanded_length;
}

//----------------------------------------------------------------------
static RDS *createRDS(FILE *input)
{
  uint text_length;
  uint i;
  SEQ *seq;
  CODE c;
  uint h_num;
  PAIR **h_first;
  uint p_max;
  PAIR **p_que;
  RDS *rds;

  fseek(input,0,SEEK_END);
  text_length = ftell(input);
  rewind(input);
  seq = (SEQ*)malloc(sizeof(SEQ)*text_length);

  printf("text size = %d(bytes)\n", text_length);

  i = 0;
  while ((c = getc(input)) != EOF) {
    seq[i].code = c;
    seq[i].next = DUMMY_POS;
    seq[i].prev = DUMMY_POS;
    i++;
  }

  h_num = INIT_HASH_NUM;
  h_first = (PAIR**)malloc(sizeof(PAIR*)*primes[h_num]);
  for (i = 0; i < primes[h_num]; i++) {
    h_first[i] = NULL;
  }

  p_max = (uint)ceil(sqrt((double)text_length));
  p_que = (PAIR**)malloc(sizeof(PAIR*)*p_max);
  for (i = 0; i < p_max; i++) {
    p_que[i] = NULL;
  }
  
  rds = (RDS*)malloc(sizeof(RDS));
  rds->text_length = text_length;
  rds->seq = seq;
  rds->num_pairs = 0;
  rds->h_num = h_num;
  rds->h_first = h_first;
  rds->p_max = p_max;
  rds->p_que = p_que;

  initRDS(rds);

  return rds;
}

//----------------------------------------------------------------------
static void initRDS(RDS *rds)
{
  uint i;
  SEQ *seq = rds->seq;
  uint text_length = rds->text_length;
  CODE A, B;
  PAIR *pair;

  for (i = 0; i < text_length - 1; i++) {
    A = seq[i].code;
    B = seq[i+1].code;
    if ((pair = locatePair(rds, A, B)) == NULL) {
      pair = createPair(rds, A, B, i);
    }
    else {
      seq[i].prev = pair->b_pos;
      seq[i].next = DUMMY_POS;
      seq[pair->b_pos].next = i;
      pair->b_pos = i;
      incrementPair(rds, pair);
    }
  }
  
  destructAllUniquePairs(rds);
}

//----------------------------------------------------------------------
static void destructRDS(RDS *rds)
{
  free(rds->seq); rds->seq = NULL;
  free(rds->h_first); rds->h_first = NULL;
  free(rds->p_que); rds->p_que = NULL;
  free(rds); rds = NULL;
}

//----------------------------------------------------------------------
static DICT *createDictStructure(uint text_length)
{
  uint i;
  RULE *rule;
  DICT *dict = (DICT*)malloc(sizeof(DICT));
  dict->text_length = text_length;
  dict->buff_size = INIT_DICTIONARY_SIZE;
  dict->rule = (RULE*)malloc(sizeof(RULE)*dict->buff_size);
  dict->seq_length = 0;
  dict->comp_seq = NULL;
  dict->num_rules = 0;
  dict->size_rules = 0;

  rule = dict->rule;
  for (i = 0; i < CHAR_SIZE; i++) {
	rule[i].symbols = (CODE*)malloc(sizeof(CODE)*2);
    rule[i].symbols[0] = (CODE)i;
    rule[i].symbols[1] = DUMMY_CODE;
    dict->num_rules++;
  }
  dict->num_usedrules = dict->num_rules;

  return dict;
}

//----------------------------------------------------------------------
static void getCompSeq(RDS *rds, DICT *dict)
{
  uint i, j;
  SEQ *seq = rds->seq;
  uint seq_length;
  CODE *comp_seq;

  i = 0; seq_length = 0;
  while (i < rds->text_length) {
    if (seq[i].code == DUMMY_CODE) {
      i = seq[i].prev;
      continue;
    }
    seq_length++;
    i++;
  }

  comp_seq = (CODE*)malloc(sizeof(CODE)*seq_length);
  i = j = 0;
  while (i < rds->text_length) {
    if (seq[i].code == DUMMY_CODE) {
      i = seq[i].prev;
      continue;
    }
    comp_seq[j++] = seq[i].code;
    i++;
  }
  dict->comp_seq = comp_seq;
  dict->seq_length = seq_length;
}

//----------------------------------------------------------------------
static PAIR *createPair(RDS *rds, CODE left, CODE right, uint f_pos)
{
  PAIR *pair = (PAIR*)malloc(sizeof(PAIR));
  uint h;
  PAIR *q;

  pair->left  = left;
  pair->right = right;
  pair->freq = 1;
  pair->f_pos = pair->b_pos = f_pos;
  pair->p_prev = pair->p_next = NULL;

  rds->num_pairs++;
  insertPair_PQ(rds, pair);

  // insert the new pair(left,right) to the head of h_first[h]
  if (rds->num_pairs >= primes[rds->h_num]) {
    reconstructPairHash(rds);
  }
  h = hash_val(rds->h_num, left, right);
  q = rds->h_first[h];
  rds->h_first[h] = pair;
  pair->h_next = q;

  return pair;
}

//----------------------------------------------------------------------
static void reconstructPairHash(RDS *rds)
{
  PAIR *p, *q;
  uint i, h;

  puts("reconstructig hash table."); fflush(stdout);
  rds->h_num++;
  rds->h_first =  
    (PAIR**)realloc(rds->h_first, sizeof(PAIR*)*primes[rds->h_num]);
  for (i = 0; i < primes[rds->h_num]; i++) {
    rds->h_first[i] = NULL;
  }
  for (i = 1; ; i++) {
    if (i == rds->p_max) i = 0;
    p = rds->p_que[i];
    while (p != NULL) {
      p->h_next = NULL;
      h = hash_val(rds->h_num, p->left, p->right);
      q = rds->h_first[h];
      rds->h_first[h] = p;
      p->h_next = q;
      p = p->p_next;
    }
    if (i == 0) break;
  }
}

//----------------------------------------------------------------------
static PAIR *locatePair(RDS *rds, CODE left, CODE right) {
  uint h = hash_val(rds->h_num, left, right);
  PAIR *p = rds->h_first[h];

  while (p != NULL) {
    if (p->left == left && p->right == right) {
      return  p;
    }
    p = p->h_next;
  }
  return NULL;
}

//----------------------------------------------------------------------
static PAIR *getMaxPair(RDS *rds)
{
  static uint i = 0;
  PAIR **p_que = rds->p_que;
  PAIR *p, *max_pair;
  uint max;

  if (p_que[0] != NULL) { // if there is pairs whose frequency is >= p_max
    p = p_que[0];
    max = 0; max_pair = NULL;
    while (p != NULL) {
      if (max < p->freq) {
        max = p->freq;
		max_pair = p;
      }
      p = p->p_next;
    }
  } else {
    max_pair = NULL;
    if (i == 0) i = rds->p_max-1;
    for (; i > 1; i--) {
      if (p_que[i] != NULL) {
		max_pair = p_que[i]; // pop a pair from the p_que[i]
		break;
      }
    }
  }
  return max_pair;
}

//----------------------------------------------------------------------
static void incrementPair(RDS *rds, PAIR *target)
{
  if (target->freq >= rds->p_max) {
    target->freq++;
  } else {
	removePair_PQ(rds, target);
	target->freq++;
	insertPair_PQ(rds, target);
  }
}

//----------------------------------------------------------------------
static void decrementPairAtThePosition(RDS *rds, uint l_pos)
{
  SEQ *seq = rds->seq;
  PAIR *target_pair = NULL;
  uint r_pos;
  CODE l_code, r_code;

  assert(0 <= l_pos && l_pos < rds->text_length - 1);
  l_code = seq[l_pos].code;
  r_pos = rightPos_SQ(rds, l_pos);
  r_code = seq[r_pos].code;

  // update target pair f_pos and b_pos
  if ((target_pair = locatePair(rds, l_code, r_code)) != NULL) { // if not unique pair
	if (target_pair->f_pos == l_pos) {
	  target_pair->f_pos = seq[l_pos].next;
	}
	if (target_pair->b_pos == l_pos) {
	  target_pair->b_pos = seq[l_pos].prev;
	}
	
	// update target count and freq queue position
	if (target_pair->freq > rds->p_max) {
	  (target_pair->freq)--;
	} else {
	  removePair_PQ(rds, target_pair);
	  (target_pair->freq)--;
	  if (target_pair->freq == 0) {
		destructPair(rds, target_pair);
	  } else {
		insertPair_PQ(rds, target_pair);
	  }
	}
  }
}

//----------------------------------------------------------------------
static void destructPair(RDS *rds, PAIR *target)
{
  uint h = hash_val(rds->h_num, target->left, target->right);
  PAIR *p = rds->h_first[h];
  PAIR *q = NULL;

  while (p != NULL) {
    if (p->left == target->left && p->right == target->right) {
      break;
    }
    q = p;
    p = p->h_next;
  }

  assert(p != NULL);
  if (q == NULL) { // if the target(=p) is at the head of h_first[h]
    rds->h_first[h] = p->h_next;
  } else {
    q->h_next = p->h_next;
  }
  free(target); target = NULL;
  rds->num_pairs--;
}

//----------------------------------------------------------------------
static void destructAllUniquePairs(RDS *rds)
{
  PAIR **p_que = rds->p_que;
  PAIR *unique_pair;
  PAIR *next_unique_pair;

  unique_pair = p_que[1];
  p_que[1] = NULL;
  while (unique_pair != NULL) {
    next_unique_pair = unique_pair->p_next;
	removePair_PQ(rds, unique_pair);
    destructPair(rds, unique_pair);
    unique_pair = next_unique_pair;
  }
}

//----------------------------------------------------------------------
static void insertPair_PQ(RDS *rds, PAIR *target)
{
  PAIR *tmp;
  uint p_num = target->freq;
  
  if (p_num >= rds->p_max) {
    p_num = 0;
  }

  // insert the target to the head of the que
  tmp = rds->p_que[p_num];
  rds->p_que[p_num] = target;
  target->p_prev = NULL;
  target->p_next = tmp;
  if (tmp != NULL) {
	tmp->p_prev = target;
  }
}

//----------------------------------------------------------------------
static void removePair_PQ(RDS *rds, PAIR *target)
{
  uint p_num = target->freq;
  
  if (p_num >= rds->p_max) {
    p_num = 0;
  }

  if (target->p_prev == NULL) { // if it is the head of the que
	rds->p_que[p_num] = target->p_next;
    if (target->p_next != NULL) {
      (target->p_next)->p_prev = NULL;
    }
  } else {
    (target->p_prev)->p_next = target->p_next;
    if (target->p_next != NULL) {
      (target->p_next)->p_prev = target->p_prev;
    }
  }
  // remove the target from the que
  target->p_next = NULL;
  target->p_prev = NULL;
}

//----------------------------------------------------------------------
static uint leftPos_SQ(RDS *rds, uint pos)
{
  SEQ *seq = rds->seq;

  assert(pos != DUMMY_POS);
  if (pos == 0) {
    return DUMMY_POS;
  }

  if (seq[pos-1].code == DUMMY_CODE) {
    return seq[pos - 1].next;
  }
  else {
    return pos - 1;
  }
}

//----------------------------------------------------------------------
static uint rightPos_SQ(RDS *rds, uint pos)
{
  SEQ *seq = rds->seq;

  assert(pos != DUMMY_POS);
  if (pos == rds->text_length - 1) {
    return DUMMY_POS;
  }

  if (seq[pos + 1].code == DUMMY_CODE) {
    return seq[pos + 1].prev;
  }
  else {
    return pos + 1;
  }
}

//----------------------------------------------------------------------
static void removeLink_SQ(RDS *rds, uint target_pos)
{
  SEQ *seq = rds->seq;
  uint prev_pair_pos, next_pair_pos;

  assert(seq[target_pos].code != DUMMY_CODE);
  prev_pair_pos = seq[target_pos].prev;
  next_pair_pos = seq[target_pos].next;
  if (prev_pair_pos != DUMMY_POS && next_pair_pos != DUMMY_POS) {
    seq[prev_pair_pos].next = next_pair_pos;
    seq[next_pair_pos].prev = prev_pair_pos;
  }
  else if (prev_pair_pos == DUMMY_POS && next_pair_pos != DUMMY_POS) { // if the first occurrence
    seq[next_pair_pos].prev = DUMMY_POS;
  }
  else if (prev_pair_pos != DUMMY_POS && next_pair_pos == DUMMY_POS) { // if the last occurrence
    seq[prev_pair_pos].next = DUMMY_POS;
  }
}

//----------------------------------------------------------------------
static MAXREP findMaximalRepeat(RDS *rds, PAIR *max_pair)
{
  SEQ *seq = rds->seq;
  uint first_pos;
  uint last_pos;
  uint left_stretch = 0;
  uint right_stretch = 0;
  uint i;
  uint l_pos, r_pos;
  uint pos_diff;
  CODE leftside_code, rightside_code;
  MAXREP max_repeat;

  // find leftside maximal
  first_pos = max_pair->f_pos;
  pos_diff = 0;
  while (true) {
	l_pos = leftPos_SQ(rds, first_pos);
	if (l_pos == DUMMY_POS) { // if the candidate maximal repeat is at the head of the input
	  break;
	}
	leftside_code = seq[l_pos].code;
	i = seq[max_pair->f_pos].next;
	while (i <= max_pair->b_pos) {  // only search within [f_pos - left_stretch, b_pos]
	  l_pos = leftPos_SQ(rds, i - pos_diff);
	  if (l_pos == DUMMY_POS || seq[l_pos].code != leftside_code) {
		break;
	  } else {
		i = seq[i].next;
	  }
	}
	if (i > max_pair->b_pos) { // if we can stretch the maximal_repeat by one symbol!
	  left_stretch++;
	  l_pos = leftPos_SQ(rds, first_pos);
	  pos_diff = max_pair->f_pos - l_pos;
	  first_pos = l_pos;
	} else {
	  break;
	}
  }

  // find rightside maximal
  last_pos = rightPos_SQ(rds, max_pair->f_pos);
  pos_diff = last_pos - max_pair->f_pos;
  while (true) {
	r_pos = rightPos_SQ(rds, last_pos);
	if (r_pos == DUMMY_POS) {
	  break;
	}
	rightside_code = seq[r_pos].code;
	i = seq[max_pair->f_pos].next;
	while (i <= max_pair->b_pos) {
	  if (i + pos_diff >= rds->text_length - 1) {
		break;
	  }
	  r_pos = rightPos_SQ(rds, i + pos_diff);
	  if (r_pos == DUMMY_POS || seq[r_pos].code != rightside_code) {
		break;
	  } else {
		i = seq[i].next;
	  }
	}
	if (i > max_pair->b_pos) {
	  right_stretch++;
	  r_pos = rightPos_SQ(rds, last_pos);
	  pos_diff = r_pos - max_pair->f_pos;
	  last_pos = r_pos;
	} else {
	  break;
	}
  }

  // check if the maximal repeat is [awa] or not
  if (left_stretch + right_stretch > 0 && seq[first_pos].code == seq[last_pos].code) {
	if (right_stretch > 0) {
	  last_pos = leftPos_SQ(rds, last_pos);
	  right_stretch--;
	} else { // if (left_stretch > 0)
	  first_pos = rightPos_SQ(rds, first_pos);
	  left_stretch--;
	}
  }
  max_repeat.origin_pair = max_pair;
  max_repeat.first_pos = first_pos;
  max_repeat.last_pos = last_pos;
  max_repeat.length = 2 + left_stretch + right_stretch;
  
  return max_repeat;
}

//----------------------------------------------------------------------
static CODE addMaximalRepeatAsNewRule(RDS *rds, DICT *dict, MAXREP maximal_repeat)
{
  SEQ *seq = rds->seq;
  RULE *rule = dict->rule;
  CODE new_code = dict->num_rules++;
  uint i;
  uint pos;

  rule[new_code].symbols = (CODE*)malloc(sizeof(CODE)*(maximal_repeat.length + 1));
  for (i = 0, pos = maximal_repeat.first_pos; i < maximal_repeat.length; i++) {
	rule[new_code].symbols[i] = seq[pos].code;
	pos = rightPos_SQ(rds, pos);
  }
  rule[new_code].symbols[i] = DUMMY_CODE;
  dict->size_rules += maximal_repeat.length;

  if (dict->num_rules >= dict->buff_size) {
    dict->buff_size *= DICTIONARY_SCALING_FACTOR;
    dict->rule = (RULE*)realloc(dict->rule, sizeof(RULE)*dict->buff_size);
    if (dict->rule == NULL) {
      puts("memory reallocation error at addMaximalRepeatAsNewRule().");
      exit(1);
    }
  }

  return new_code;
}

//----------------------------------------------------------------------
static uint replaceMaximalRepeat(RDS *rds, MAXREP maximal_repeat, CODE new_code)
{
  SEQ *seq = rds->seq;
  PAIR *origin_pair = maximal_repeat.origin_pair;
  uint mr_first_origin_pos = origin_pair->f_pos;
  uint mr_last_origin_pos = origin_pair->b_pos;
  uint mr_current_origin_pos = mr_first_origin_pos;
  uint mr_current_leftmost_pos;
  uint mr_current_rightmost_pos;
  uint mr_next_origin_pos;
  uint left_pos_diff = mr_first_origin_pos - maximal_repeat.first_pos;
  uint right_pos_diff = maximal_repeat.last_pos - mr_first_origin_pos;
  uint l_pos, r_pos;
  uint i_pos, j_pos;
  uint num_replaced = 0;

  LOG("# %u=%u,%u\n", new_code, origin_pair->left, origin_pair->right);

  while (mr_current_origin_pos <= mr_last_origin_pos) {
	mr_current_leftmost_pos = mr_current_origin_pos - left_pos_diff;
	mr_current_rightmost_pos = mr_current_origin_pos + right_pos_diff;
	mr_next_origin_pos = seq[mr_current_origin_pos].next; // save next because current is overwritten
	if (mr_next_origin_pos - left_pos_diff <= mr_current_rightmost_pos) { // if next overlap, then skip
	  mr_next_origin_pos = seq[mr_next_origin_pos].next;
	}

	l_pos = leftPos_SQ(rds, mr_current_leftmost_pos);
	r_pos = rightPos_SQ(rds, mr_current_rightmost_pos);
	// check a[bwc] and decrement ab
	if (l_pos != DUMMY_POS) {
	  decrementPairAtThePosition(rds, l_pos);
	  removeLink_SQ(rds, l_pos);
	}
	// decrement all the pairs within [bwc]
	for (i_pos = mr_current_leftmost_pos; i_pos < mr_current_rightmost_pos; ) {
	  j_pos = rightPos_SQ(rds, i_pos);
	  decrementPairAtThePosition(rds, i_pos);
	  removeLink_SQ(rds, i_pos);
	  seq[i_pos].code = DUMMY_CODE;
	  i_pos = j_pos;
	}
	// check [bwc]d and decrement cd
	if (r_pos != DUMMY_POS) {
	  decrementPairAtThePosition(rds, mr_current_rightmost_pos);
	  removeLink_SQ(rds, mr_current_rightmost_pos);
	}
	seq[mr_current_rightmost_pos].code = DUMMY_CODE;
	
	// update block [bwc] to X then increment aX and Xd
	updateBlockToNewcode(rds, mr_current_leftmost_pos, mr_current_rightmost_pos, r_pos, new_code);
	incrementNewPairs(rds, l_pos, mr_current_leftmost_pos, r_pos, new_code);
	
	num_replaced += maximal_repeat.length - 1;
	mr_current_origin_pos = mr_next_origin_pos;
  }
  destructAllUniquePairs(rds);

#ifndef NDEBUG
  for (int j = 0; j < rds->text_length; j++) {
	LOG("s[%d]=[%d (%d) %d]\n", j, seq[j].prev, seq[j].code, seq[j].next);
  }
  puts("");
#endif

  return num_replaced;
}


//----------------------------------------------------------------------
static void updateBlockToNewcode(RDS *rds, uint leftmost_pos, uint rightmost_pos, uint r_pos, CODE new_code)
{
  SEQ *seq = rds->seq;

  seq[leftmost_pos].code = new_code;
  if (r_pos != DUMMY_POS) { // if X is not at the end of text
	if (leftmost_pos + 2 == r_pos) { // if block size == 2
	  seq[rightmost_pos].prev = r_pos;
	  seq[rightmost_pos].next = leftmost_pos;
	} else {
	  seq[leftmost_pos + 1].prev = r_pos;
	  seq[leftmost_pos + 1].next = DUMMY_POS;
	  seq[r_pos - 1].prev = DUMMY_POS;
	  seq[r_pos - 1].next = leftmost_pos;
	}
  } else { // X is at the end of text
	if (leftmost_pos + 2 == rds->text_length) { // if block size == 2
	  seq[rightmost_pos].prev = DUMMY_POS;
	  seq[rightmost_pos].next = leftmost_pos;
	} else {
	  seq[leftmost_pos + 1].prev = DUMMY_POS;
	  seq[leftmost_pos + 1].next = DUMMY_POS;
	  seq[rds->text_length - 1].prev = DUMMY_POS;
	  seq[rds->text_length - 1].next = leftmost_pos;
	}
  }
}

//----------------------------------------------------------------------
static void incrementNewPairs(RDS *rds, uint l_pos, uint block_pos, uint r_pos, CODE new_code)
{
  SEQ *seq = rds->seq;
  PAIR *target_pair = NULL;
  CODE l_code, r_code;
  
  // create or increment aX
  if (l_pos != DUMMY_POS) {
	l_code = seq[l_pos].code;
	if ((target_pair = locatePair(rds, l_code, new_code)) == NULL) {
	  createPair(rds, l_code, new_code, l_pos);
	  seq[l_pos].prev = DUMMY_POS;
	  seq[l_pos].next = DUMMY_POS;
	} else {
	  incrementPair(rds, target_pair);
	  seq[target_pair->b_pos].next = l_pos;
	  seq[l_pos].prev = target_pair->b_pos;
	  seq[l_pos].next = DUMMY_POS;
	  target_pair->b_pos = l_pos;
	}
  }
  // create or increment Xb
  if (r_pos != DUMMY_POS) {
	r_code = seq[r_pos].code;
	if ((target_pair = locatePair(rds, new_code, r_code)) == NULL) {
	  createPair(rds, new_code, r_code, block_pos);
	  seq[block_pos].prev = DUMMY_POS;
	  seq[block_pos].next = DUMMY_POS;
	} else {
	  incrementPair(rds, target_pair);
	  seq[target_pair->b_pos].next = block_pos;
	  seq[block_pos].prev = target_pair->b_pos;
	  seq[block_pos].next = DUMMY_POS;
	  target_pair->b_pos = block_pos;
	}
  }
}
