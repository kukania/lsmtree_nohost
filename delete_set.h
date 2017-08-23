#ifndef __DELETE_H__
#define __DELETE_H__
#include "utils.h"
#include"skiplist.h"
#include<limits.h>
typedef struct{
	uint8_t bitset[PAGENUM/8];
	int number;
	int invalid_n;
	bool erased;
}block;

typedef struct{
	block blocks[SEGNUM];
	int cache_level;
	sktable *cache_table;
	Entry *cache_entry;
	KEYT cache_pbn;
}delete_set;

void delete_init(delete_set *);
void delete_ppa(delete_set *set,KEYT ppa);
int delete_trim_process(delete_set *);
void delete_free(delete_set *);
void delete_dirty_set(delete_set *,KEYT ppa);
int delete_get_victim(delete_set *);
inline uint64_t KEYSET(uint64_t &des,uint32_t src){
	uint64_t temp=src;
	des=des | (temp<<32);
	return des;
}

inline uint64_t FLAGSET(uint64_t &des, bool flag){
	des=des | flag;
	return des;
}
inline uint64_t KEYGET(uint64_t des){
	return des>>32;
}
inline uint64_t LEVELGET(uint64_t des){
	uint64_t temp=UCHAR_MAX;
	return des>>8 & temp;
}
inline bool FLAGGET(uint64_t &des){
	return des & 1;
}
#endif
