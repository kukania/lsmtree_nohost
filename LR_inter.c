#include"LR_inter.h"
#include"lsmtree.h"
#include"threading.h"
#include"utils.h"
#include<pthread.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#ifdef ENABLE_LIBFTL
#include "libmemio.h"
#endif
lsmtree *LSM;
threadset processor;
MeasureTime mt;
MeasureTime mas;
MeasureTime mas2;
pthread_mutex_t print_lock;
extern pthread_mutex_t sst_lock;
extern pthread_mutex_t mem_lock;
pthread_mutex_t pl;
pthread_mutex_t endR;

extern MeasureTime bp;
extern MeasureTime buf;
int8_t lr_inter_init(){
	LSM=(lsmtree*)malloc(sizeof(lsmtree));
	threadset_init(&processor);
	threadset_start(&processor);
	pthread_mutex_init(&pl,NULL);
	pthread_mutex_init(&endR,NULL);
	measure_init(&mt);
	measure_init(&mas);
	measure_init(&mas2);
	if(init_lsm(LSM)!=NULL)
		return 0;
	return -1;
}
int8_t lr_inter_free(){
	printf("end called~~\n");
	threadset_end(&processor);
	threadset_clear(&processor);
	pthread_mutex_destroy(&pl);
	lsm_free(LSM);
	return 0;	
}
int8_t lr_gc_make_req(int8_t t_num){
	Entry *result_entry;
	lsmtree_gc_req_t * gc_req;

	pthread_mutex_lock(&sst_lock);
	LSM->sstable=LSM->memtree;
	pthread_mutex_unlock(&sst_lock);

	pthread_mutex_lock(&mem_lock);
	LSM->memtree=(skiplist*)malloc(sizeof(skiplist));
	LSM->memtree=skiplist_init(LSM->memtree);
	pthread_mutex_unlock(&mem_lock);
	
	gc_req=(lsmtree_gc_req_t*)malloc(sizeof(lsmtree_gc_req_t));
	gc_req->type=LR_FLUSH_T;
	gc_req->params[1]=NULL;
	gc_req->params[2]=(void*)LSM->sstable;
	gc_req->params[3]=(void*)LSM;
	gc_req->end_req=lr_gc_end_req;
	gc_req->isgc=true;
	threadset_gc_assign(&processor,gc_req);
	return 0;
}
int make_cnt=1;
int8_t lr_make_req(req_t *r){
	int test_num;
	lsmtree_req_t *th_req=(lsmtree_req_t*)malloc(sizeof(lsmtree_req_t));
#ifdef LIBLSM
	switch(r->type){
#else
		switch(r->type_info->type){
#endif
			case 3:
			case 1://set
				th_req->type=LR_WRITE_T;
				break;
			case 2://get
				th_req->type=LR_READ_T;
				break;
			default:
				return -1;
				break;
		}
		th_req->req=r;
		th_req->end_req=lr_end_req;

#ifdef LIBLSM
		th_req->params[1]=(void*)&r->key;
		th_req->params[2]=(void*)r->value;
		th_req->params[3]=(void*)LSM;
#else
		th_req->params[1]=(void*)&r->key_info->key;
		th_req->params[2]=(void*)r->value_info->value;
		th_req->params[3]=(void*)LSM;
#endif
		th_req->isgc=false;
		/*
		if(th_req->type==LR_WRITE_T)
			MS(&mas2);*/
/*		if(th_req->type==LR_READ_T)
			MS(&mas);*/
		if(th_req->type==LR_WRITE_T)
			threadset_assign(&processor,th_req);		
		else
			threadset_read_assign(&processor,th_req);
		/*if(th_req->type==LR_READ_T)
			MA(&mas);*//*
		if(th_req->type==LR_WRITE_T)
			MA(&mas2);*/
		return 0;
	}

struct timeval max_time;
int big_time_check;
int8_t lr_end_req(lsmtree_req_t *r){
	static int readfree=0;
	lsmtree_req_t *parent;
	lsmtree_gc_req_t *gc_p;
	static bool check_time=false;
	KEYT key;
	char *value;
	switch(r->type){
		case LR_DR_T:
			parent=r->parent;
			parent->keys=r->keys;
			parent->dmatag=r->dmatag;
			//memcpy(parent->res,r->keys,PAGESIZE);
#ifndef NDMA
		//	memio_free_dma(2,r->dmatag);
#else
			free(r->keys);
#endif
			MS(&bp);
			pthread_mutex_unlock(&parent->meta_lock);
			MA(&bp);
			break;
		case LR_DDR_T:
			parent=r->parent;
			parent->end_req(parent);/*
									   if(!check_time){
									   check_time=true;
									   max_time=MR(&r->mt);
									   if(timercmp(&max_time,&test_time,>))
									   big_time_check++;
									   }
									   else{
									   res=MR(&r->mt);
									   if(timercmp(&res,&test_time,>))
									   big_time_check++;
									   if(timercmp(&max_time,&res,<))
									   max_time=res;
									   }*/
			//	ME(&r->mt,"read done");
			r->req=NULL;
			break;
		case LR_DDW_T:
#ifndef NDMA
			memio_free_dma(1,r->req->dmaTag);
#else
			free(r->req->value);
#endif
			break;
		case LR_READ_T:
			pthread_mutex_destroy(&r->meta_lock);
#ifndef NDMA
			memio_free_dma(2,r->req->dmaTag);
#else
			free(r->req->value);
#endif
			break;
		default:
			break;
	}

	if(r->req!=NULL){
#ifndef LIBLSM
		r->req->end_req(r->req);
#else	
		free(r->req);
		r->req=NULL;
#endif
	}
	free(r);
	return 0;
}
int8_t lr_gc_end_req(lsmtree_gc_req_t *r){
	lsmtree_gc_req_t *parent;
	int setnumber,offset;
	switch(r->type){
		case LR_DW_T:
#ifndef NDMA
			memio_free_dma(1,r->dmatag);
#else
			free(r->keys);
#endif
			break;
		case LR_DR_T:
			parent=r->parent;
			setnumber=r->seq_number;
			memcpy(&parent->compt_headers[setnumber],r->keys,PAGESIZE);
			pthread_mutex_lock(&parent->meta_lock);
			parent->now_number++;
			pthread_mutex_unlock(&parent->meta_lock);
#ifndef NDMA
			memio_free_dma(2,r->dmatag);
#else
			free(r->keys);
#endif
			break;
		case LR_FLUSH_T:	
			pthread_mutex_destroy(&r->meta_lock);
			if(r->compt_headers!=NULL)
				free(r->compt_headers);
			break;
	}
	free(r);
}

int8_t lr_gc_req_wait(lsmtree_gc_req_t *input){
	while(1){
		pthread_mutex_lock(&input->meta_lock);
		if(input->now_number==input->target_number){
			pthread_mutex_unlock(&input->meta_lock);
			break;
		}
		pthread_mutex_unlock(&input->meta_lock);
	}
	return 0;
}
int8_t lr_req_wait(lsmtree_req_t *input){
	while(1){
		pthread_mutex_lock(&input->meta_lock);
		if(input->now_number==input->target_number){
			pthread_mutex_unlock(&input->meta_lock);
			break;
		}
		pthread_mutex_unlock(&input->meta_lock);
	}
	return 0;
}
int8_t lr_is_gc_needed(){
	for(int i=LEVELN-1; i>=0; i--){
		if(LSM->buf.disk[i]==NULL)
			break;
		if(LSM->buf.disk[i]->size>=LSM->buf.disk[i]->m_size){
			return i+2;
		}
	}
	if(is_flush_needed(LSM)){
		Entry **iter=level_range_find(LSM->buf.disk[0],LSM->memtree->start,LSM->memtree->end);
		if(iter!=NULL && iter[0]!=NULL){
			free(iter);
			return 1;
		}
		if(iter!=NULL)
			free(iter);
		return 0;
	}
	return -1;
}

