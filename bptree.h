#ifndef __BP__HEADER__
#define __BP__HEADER__
#include"utils.h"
#include"bloomfilter.h"
#include"skiplist.h"
#include"cache.h"
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
	bool iscompactioning;
	uint8_t *bitset;
	double fpr;
#ifdef BLOOM
	BF *filter;
#endif
	struct Node *parent;
#ifdef CACHE
	struct sktable *data;
	struct cache_entry *c_entry;
#endif
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
	pthread_mutex_t level_lock;
}level;
typedef struct iterator{
	Node *now;
	int idx;
}Iter;

Node *level_find_leafnode(level *lev, KEYT key);
Entry *make_entry(KEYT start, KEYT end,KEYT pbn);
Entry *level_entry_copy(Entry *,bool);
level *level_copy(level*);
level* level_init(level*,int size);
Entry *level_find(level*,KEYT key);
Entry *level_get_victim(level *);
Entry **level_range_find(level *,KEYT start,KEYT end);
Node *level_insert(level*,Entry *);
Node *level_delete(level*,KEYT);
Entry *level_getFirst(level *);
Entry *level_get_next(Iter *);
Iter* level_get_Iter(level *);
void level_save(level *,int fd);
void level_load(level *,int fd);
void level_print(level *);
void free_entry(Entry *);
void level_free(level*);
#endif
