#ifndef __BP__HEADER__
#define __BP__HEADER__
#include"utils.h"
#include"bloomfilter.h"
struct Entry; struct Node;
typedef union Child{
	struct Entry *entry;
	struct Node *node;
}Child;

typedef struct Entry{
	KEYT key;
	KEYT version;
	KEYT end;
	KEYT pbn;
	bool gc_cache;
	uint8_t *bitset;
#ifdef BLOOM
	BF *filter;
#endif
	struct Node *parent;
}Entry;

typedef struct Node{
	bool leaf;
	short count;
	KEYT separator[MAXC];
	Child children[MAXC+1];
	struct Node *parent;
}Node;

typedef struct level{
	Node *root;
	int number;
	int size;
	int m_size;
	int depth;
	double fpr;
	KEYT start;
	KEYT end;
	KEYT version;
}level;
typedef struct iterator{
	Node *now;
	int idx;
}Iter;

Node *level_find_leafnode(level *lev, KEYT key);//
Entry *make_entry(KEYT start, KEYT end,KEYT pbn);
Entry *level_entry_copy(Entry *);
level* level_init(level*,int size);
Entry *level_find(level*,KEYT key);
Entry *level_get_victim(level *);//
Entry **level_range_find(level *,KEYT start,KEYT end);
Node *level_insert(level*,Entry *);
Node *level_delete(level*,KEYT);
Entry *level_getFirst(level *);//
Entry *level_get_next(Iter *);
Iter* level_get_Iter(level *);
void level_print(level *);
void free_entry(Entry *);//
void level_free(level*);
#endif
