#include"LR_inter.h"
#include"measure.h"
#include"lsm_cache.h"
#include"utils.h"
#include"threading.h"
#include"delete_set.h"
#include"ppa.h"
#ifndef ENABLE_LIBFTL
#include"libmemio.h"
#endif
#include<sys/types.h>
#include<sys/stat.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<limits.h>
extern threadset processor;
extern MeasureTime mt;
extern MeasureTime mas;
extern MeasureTime mas2;

extern MeasureTime *wt;
extern MeasureTime *at;
extern MeasureTime gt;
extern MeasureTime rt;
extern MeasureTime last;


extern pthread_mutex_t endR;
extern memio* mio;
extern timeval max_time;
extern int big_time_check;
extern timeval max_time1,adding;
extern int big_time_check1;

extern int pros_hit;
extern int pros_hit2;
extern int cache_hit;
extern int mem_hit;
extern lsmtree *LSM;
extern delete_set *header_segment;
extern delete_set *data_segment;
KEYT *keys;
int cnnt=0;
bool utils_flag;
#define FLAGS 11
/*
void *util_check(void *){
	while(utils_flag){
	//	printf("%d\n",127-mio->tagQ->size());
		//fprintf(stderr,"%f\n",processor.threads[0].flag);
	}
}*/
int main(){
	wt=at=NULL;
	lr_inter_init();
	req_t * req;
	KEYT key;
	measure_init(&mt);
	int fd;
	printf("start!\n");
	measure_start(&mt);
	keys=(KEYT*)malloc(sizeof(KEYT)*INPUTSIZE);
	for(int i=0; i<INPUTSIZE; i++){
		keys[i]=rand()%UINT_MAX+1;
	}
	int cnt=1;
	for(int i=1; i<=INPUTSIZE; i++){
		//printf("%d\n",i);
		req=(req_t*)malloc(sizeof(req_t));
		req->type=1;
		if(SEQUENCE==0){
			key=keys[i-1];
			if(key==1)
			printf("test\n");
		}
		else{
			key=i;
		}
		req->key=key;
#ifdef ENABLE_LIBFTL
		req->dmaTag=memio_alloc_dma(req->type,&req->value);
#else
		req->value=(char*)malloc(PAGESIZE);
#endif
		memcpy(req->value,&key,sizeof(KEYT));
		lr_make_req(req);
	}
	//printf("throw all write req!\n");
	threadset_request_wait(&processor);
	threadset_gc_wait(&processor);
	measure_end(&mt,"write_wait");
	printf("write end!!\n");/*
	for(int i=0; i<INPUTSIZE/10; i++){
		req=(req_t *)malloc(sizeof(req_t));
		req->key=i+1;
		req->value=NULL;
		req->type=3;
		lr_make_req(req);
	}
	threadset_request_wait(&processor);
	threadset_gc_wait(&processor);
	printf("delete end!!\n");
	while(delete_trim_process_data(data_segment)){
		printf("deleted!\n");
	}*//*
	for(int i=0; i<LEVELN; i++){
		printf("\n--------------[level %d]-------\n",i);
		level_print(LSM->buf.disk[i]);
	}*/
	//sleep(10);

	pthread_t thr;
	processor.threads[0].flag=0;
	//pthread_create(&thr,NULL,util_check,NULL);
	printf("read!\n");
	measure_start(&mt);
	//printf("??");
	for(int i=1; i<=INPUTSIZE; i++){
		req=(req_t*)malloc(sizeof(req_t));
		req->type=2;
		utils_flag=true;
		if(SEQUENCE==0){
			key=keys[i-1];
		}
		else{
			key=i;
		}
		req->key=key;
		key=i;
#ifdef ENABLE_LIBFTL
		//printf("main_alloc!\n");
		req->dmaTag=memio_alloc_dma(req->type,&req->value);
#else
		req->value=(char*)malloc(PAGESIZE);
#endif
		lr_make_req(req);
	}
	//printf("throw all read req!\n");
	threadset_read_wait(&processor);
	measure_end(&mt,"read_end");
	utils_flag=false;
	//measure_end(&mt,"read_end");
//	printf("last:%.6f\n",(float)last.adding.tv_usec/1000000);
	threadset_debug_print(&processor);
	//printf("assign write:%.6f\n",(float)mas2.adding.tv_usec/1000000);
//	printf("assign read:%.6f\n",(float)mas.adding.tv_usec/1000000);
	//printf("writetime:%.6f\n",(float)gt.adding.tv_usec/1000000);
//	printf("readtime:%.6f\n",(float)rt.adding.tv_usec/1000000);
//	printf("wt:%.6f\n",(float)wt->adding.tv_usec/1000000);
//	printf("at:%.6f\n",(float)at->adding.tv_usec/1000000);

/*
	printf("1>>max:%ld sec and %.6f\n",max_time1.tv_sec,(float)max_time1.tv_usec/1000000);
	printf("1>>over time (%d): %d\n",INPUTSIZE,big_time_check1);
	printf("1>>avg:%ld sec and %6.6f-%d\n",adding.tv_sec/end_counter,((float)adding.tv_usec/1000000),end_counter);
	printf("max:%ld sec and %.6f\n",max_time.tv_sec,(float)max_time.tv_usec/1000000);
	printf("over time (%d): %d\n",INPUTSIZE,big_time_check);*/
	//lr_inter_free();
	/*
	for(int i=0; i<LEVELN; i++){
		printf("\n--------------[level %d]-------\n",i);
		level_print(LSM->buf.disk[i]);
	}*/
/*
	cache_summary(&processor.mycache);
	printf("pros hit 1 : %d\n",pros_hit);
	printf("pros hit 2 : %d\n",pros_hit2);
	printf("mem hit :%d\n",mem_hit);
	printf("cache_hit : %d\n",cache_hit);
*/
}
