#include<stdlib.h>
#include<string.h>
#include"libmemio.h"
#include "lsm_cache.h"

void cache_init(lsm_cache *input){
	for(int i=0; i<LEVELN; i++){
		for(int j=0; j<CACHENUM; j++){
			input->caches[i][j].content=NULL;
			input->caches[i][j].hit=0;
		}
	}
	input->time_bit=0;
}

void cache_clear(lsm_cache *input){
	for(int i=0; i<LEVELN; i++){
		for(int j=0; j<CACHENUM; j++){
			free(input->caches[i][j].content);
		}
	}
}
void cache_input(lsm_cache* input,int l,sktable* sk,int tag){
	int value;

	cache *victim=&input->caches[l][0];
	uint64_t stand=input->caches[l][0].check_bit;
	value=0;
	for(int i=1; i<CACHENUM; i++){
		if(stand>input->caches[l][i].check_bit){
			stand=input->caches[l][i].check_bit;
			victim=&input->caches[l][i];
			value=i;
		}
	}
	//memcpy(victim->content,sk,PAGESIZE);
	if(victim->content!=NULL){
		memio_free_dma(2,victim->dmatag);
	}
	victim->content=sk;
	victim->dmatag=tag;
	victim->check_bit=input->time_bit++;
}

keyset* cache_level_find(lsm_cache* input,int l,KEYT k){
	keyset *res;
	for(int j=0;j<CACHENUM; j++){
		if(input->caches[l][j].content==NULL)
			continue;
		res=skiplist_keyset_find(input->caches[l][j].content,k);
		if(res!=NULL) {
			input->caches[l][j].check_bit=input->time_bit++;
			input->caches[l][j].hit++;
			return res;
		}
	}
	return NULL;
}
void cache_summary(lsm_cache* input){
	int sum=0;
	for(int i=0; i<LEVELN; i++){
		for(int j=0; j<CACHENUM; j++){
			sum+=input->caches[i][j].hit;
		}
	}
	printf("hit:%d\n",sum);
}
