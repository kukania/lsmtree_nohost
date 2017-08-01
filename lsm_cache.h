#ifndef __CACHE_H__
#define __CACHE_H__
#include "skiplist.h"
#include "utils.h"

#include<pthread.h>
typedef struct {
	sktable *content;
	uint64_t check_bit;
	int dmatag;
	int hit;
	bool cpyflag;
}cache;
typedef struct{
	cache caches[LEVELN][CACHENUM];
	int all_hit;
	uint64_t time_bit;
}lsm_cache;

void cache_init(lsm_cache *);
void cache_clear(lsm_cache *);
keyset* cache_level_find(lsm_cache *,int level,KEYT key);
void cache_input(lsm_cache *,int level,sktable *,int);
void cache_summary(lsm_cache* input);
//void cache_update(lsm_cache *,KEYT key, char *value);
#endif
