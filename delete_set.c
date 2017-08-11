#include<string.h>
#include"delete_set.h"
#include"libmemio.h"
#include"bptree.h"
#include"lsmtree.h"
#include"LR_inter.h"
#include"ppa.h"
extern lsmtree *LSM;
extern memio *mio;
extern KEYT ppa;
uint64_t *oob;
int8_t delete_end_req(lsmtree_req_t *req){
	switch(req->type){
		case LR_DELETE_W:
			free(req);
			break;
		case LR_DELETE_R:
			pthread_mutex_unlock(&req->meta_lock);
			break;
	}
}
lsmtree_req_t * delete_make_req(bool iswrite){
	lsmtree_req_t *req=(lsmtree_req_t *)malloc(sizeof(lsmtree_req_t));
	if(iswrite){
		req->type=LR_DELETE_W;
	}
	else{
		req->type=LR_DELETE_R;
		pthread_mutex_init(&req->meta_lock,NULL);
		pthread_mutex_lock(&req->meta_lock);
	}
	req->end_req=delete_end_req;
	return req;
}
void delete_init(delete_set *set){
	//for test
	oob=(uint64_t *)malloc(2*INPUTSIZE*sizeof(uint64_t));
	for(int i=0; i<SEGNUM; i++){
		for(int j=0; j<PAGENUM/8; j++){
			set->blocks[i].bitset[j]=0;
		}
		set->blocks[i].invalid_n=0;
		set->blocks[i].number=i;
	}
}
void delete_ppa(delete_set *set,KEYT input){
	int block_num=input/PAGENUM;
	int offset_meta=input%PAGENUM;

	int bit_num=offset_meta/8;
	int offset=offset_meta%8;
	
	uint8_t target=1;
	target=target<<offset;

	set->blocks[block_num].bitset[bit_num] |= target;
	set->blocks[block_num].invalid_n++;
}
int delete_get_victim(delete_set *set){
	int invalids=0;
	int block_num=0;
	for(int i=0; i<SEGNUM; i++){
		if(invalids<set->blocks[i].invalid_n){
			invalids=set->blocks[i].invalid_n;
			block_num=i;
		}
	}
	if(invalids==0)
		return -1;
	return block_num;
}
void delete_trim_process(delete_set *set){
	int block_num=delete_get_victim(set);
	int invalids=set->blocks[block_num].invalid_n;

	if(invalids==PAGENUM){ 
		memset(set->blocks[block_num].bitset,0,PAGENUM/8);
		set->blocks[block_num].invalid_n=0;
	}
	else{
		KEYT temp_p;
		for(int i=0 ; i<PAGENUM/8; i++){
			for(int j=0; j<8; j++){
				uint8_t test=1<<j;
				if(!(set->blocks[block_num].bitset[i] & test)){ //0 valid,1 invalid
					temp_p=block_num*PAGENUM+i*8+j;
					uint64_t temp_oob=oob[temp_p];
					int level=LEVELGET(temp_oob);
					KEYT key=KEYGET(temp_oob);
					Entry *header=level_find(LSM->buf.disk[level],key);
					lsmtree_req_t *req=delete_make_req(0);
					sktable *sk=(sktable*)malloc(sizeof(sktable));
					req->res=sk;
					char *temp_p;
					req->dmatag=memio_alloc_dma(2,&temp_p);
					req->data=temp_p;
					memio_read(mio,header->pbn,(uint64_t)(PAGESIZE),(uint8_t *)req->data,1,req,req->dmatag);
					//read header data;
					pthread_mutex_lock(&req->meta_lock);
					pthread_mutex_destroy(&req->meta_lock);

					if(!FLAGGET(temp_oob)){//0 data, 1 header
						keyset *temp_key=skiplist_keyset_find(sk,key);
						char *value=(char *)malloc(PAGESIZE);
						/*page read*/
						req=delete_make_req(0);//read
						req->type=LR_DELETE_PR;
						req->dmatag=memio_alloc_dma(2,&temp_p);
						req->data=temp_p;
						req->params[2]=(void*)value;
						memio_read(mio,temp_key->ppa,(uint64_t)PAGESIZE,(uint8_t *)req->data,1,req,req->dmatag);
						pthread_mutex_lock(&req->meta_lock);
						pthread_mutex_destroy(&req->meta_lock);
						/*page read*/
						
						KEYT new_ppa=getPPA(); 
						req=delete_make_req(1);//write
						req->dmatag=memio_alloc_dma(1,&temp_p);
						req->data=temp_p;
						memcpy(temp_p,value,PAGESIZE);
						memio_write(mio,new_ppa,(uint64_t)PAGESIZE,(uint8_t *)req->data,1,req,req->dmatag);
						temp_key->ppa=new_ppa;
						free(value);
					}
					KEYT new_pba=getPPA(); 
					req=delete_make_req(1);//write
					req->dmatag=memio_alloc_dma(1,&temp_p);
					req->data=temp_p;
					memcpy(temp_p,sk,PAGESIZE);
					memio_write(mio,new_pba,(uint64_t)PAGESIZE,(uint8_t*)req->data,1,req,req->dmatag);
					KEYT temp_pba=header->pbn;
					header->pbn=new_pba;
					delete_ppa(set,temp_pba);
				}
				else
					continue;
			}
		}
	}
	
	//send trim operation;
	memio_trim(mio,block_num*PAGENUM,PAGENUM);
	for(int i=block_num*PAGENUM; i<PAGENUM*block_num+PAGENUM;i++){
		freePPA(i);
	}
	return;
}

void delete_free(delete_set *set){
	free(set);
}

