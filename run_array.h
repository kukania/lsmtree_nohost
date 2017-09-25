#ifndef __RUN_A_H__
#define __RUN_A_H__
#include"utils.h"
#include"bloomfilter.h"
struct Entry;
struct Node;
struct level;

typedef struct Entry{
	void *start;
	KEYT key;
	KEYT version;
	KEYT end;
	KEYT pbn;
	uint8_t bitset[KEYN/8];
#ifdef BLOOM
	BF *filter
#endif
}Entry;

typedef struct Node{
	void *start;
	int n_num;
	int m_num;
	int e_size;
	char *body;
}Node;

typedef struct level{
	int size;
	int r_num;
	int m_num;//number of entries
	int n_num;
	int r_size;//size of run
	char *body;
}level;

typedef struct iterator{
	Node *now;
	int idx;
}Iter;
level *level_init(level *,int size);
Entry *level_find(level *,KEYT key);
Node *level_insert(level *,Entry);
Entry *level_get_next(Iter *);
Iter *level_get_Iter(level *);
void level_print(level *);
void level_free(levle *);
#endif
