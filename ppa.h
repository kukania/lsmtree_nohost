#ifndef __PPN_H__
#define __PPN_H__
//#include"normal_queue.h"
#include"utils.h"
#include<queue>
typedef struct{
	uint8_t bitset[PAGENUM/8];
	int number;
	int invalid_n;
	bool erased;
}block;

typedef struct{
	block *blocks;
	//queue *ppa;
	std::queue<KEYT> *ppa;
	pthread_mutex_t ppa_lock;
	int reserve_block;
	int reserve_block_point;
	int start_block_n;
	int size;
	bool isdata;
}segment;
typedef segment delete_set;

void segment_init(segment *,int start_block_nr,int size,bool isdata);
KEYT getPPA(segment*,void *);
KEYT getRPPA(segment *,void *);
void freePPA(segment*,KEYT);
void segment_block_init(segment *, int block_num);
void segment_block_change(segment *,int target_block);
void segment_free(segment*);
#endif
