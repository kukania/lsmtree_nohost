#include"LR_inter.h"
#include"measure.h"
#include"lsm_cache.h"
#include"utils.h"
#include"threading.h"
#include"delete_set.h"
#include"ppa.h"
#ifdef ENABLE_LIBFTL
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
extern MeasureTime mem;
extern MeasureTime last;
extern MeasureTime buf;
extern MeasureTime bp;
extern MeasureTime find;


extern pthread_mutex_t endR;
extern memio* mio;
extern timeval max_time;
extern int big_time_check;
extern timeval max_time1,adding;
extern int big_time_check1;
extern int meta_read_data;

extern int pros_hit;
extern int pros_hit2;
extern int cache_hit;
extern int mem_hit;
extern lsmtree *LSM;
extern delete_set *header_segment;
extern delete_set *data_segment;
extern KEYT *keys;
int cnnt=0;
bool utils_flag;
#define FLAGS 11

void *util_check(void *){
	while(utils_flag){
	}
}

int main(){
	wt=at=NULL;
	lr_inter_init();
	req_t * req;
	KEYT key;
	measure_init(&mt);
	int fd;
	printf("start!\n");
	MS(&mt);
	
	int cnt=1;
	char *temp_value=(char*)malloc(PAGESIZE);
	for(int i=1; i<=INPUTSIZE; i++){
		//printf("%d\n",i);
		req=(req_t*)malloc(sizeof(req_t));
		req->type=1;
		if(i%1024==1){
		//	printf("%d\n",i);
		}
		if(SEQUENCE==0){
			key=keys[i-1];
		}
		else{
			key=i;
		}
		req->key=key;
		char *temp_t;
#ifdef ENABLE_LIBFTL
		req->dmaTag=memio_alloc_dma(req->type,&temp_t);
#else
		temp_t=(char*)malloc(PAGESIZE);
#endif
		req->value=temp_t;
		lr_make_req(req);
	}
	//printf("throw all write req!\n");
	threadset_request_wait(&processor);
	threadset_gc_wait(&processor);
	threadset_request_wait(&processor);
	//lr_wait();
	MT(&mt);
	//measure_end(&mt,"write_wait");
	printf("write end!!\n");
	/*
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
	while(delete_trim_process_header(header_segment)){
		printf("deleted!\n");
	}*//*
	for(int i=0; i<LEVELN; i++){
		printf("\n--------------[level %d]-------\n",i);
		level_print(LSM->buf.disk[i]);
	}*/
	//sleep(10);
	
	
	int tfd=open("data/meta.data",O_RDWR|O_CREAT|O_TRUNC,0666);
	int level_cnt=0;
	for(int i=0; i<LEVELN; i++){
		level *lev=LSM->buf.disk[i];
		if(lev->size!=0){
			level_cnt++;
		}
	}
	write(tfd,&level_cnt,sizeof(level_cnt));
	for(int i=0; i<level_cnt; i++){
		level *lev=LSM->buf.disk[i];
		if(lev->size!=0){
			level_save(lev,tfd);
		}
	}
	skiplist_save(LSM->memtree,tfd);
/*
	processor.threads[0].flag=0;
	printf("read!\n");
	pthread_t thr;
	utils_flag=true;
	//pthread_create(&thr,NULL,util_check,NULL);
	MS(&mt);
	//printf("??");
	for(int i=1; i<=INPUTSIZE; i++){
		req=(req_t*)malloc(sizeof(req_t));
		req->type=2;
		if(i%1024==1){
		//	printf("%d\n",i);
		}
		if(SEQUENCE==0){
	//		key=INT_MAX;
			key=keys[i-1];
	//		key=rand()%INT_MAX;
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
	MT(&mt);
	printf("meta_read_data:%d\n",meta_read_data);
	//measure_end(&mt,"read_end");
	printf("mem:%.6f\n",(float)mem.adding.tv_usec/1000000);
//	printf("last:%.6f\n",(float)last.adding.tv_usec/1000000);
	printf("buf:%.6f\n",(float)buf.adding.tv_usec/1000000);
	printf("bp:%.6f\n",(float)bp.adding.tv_usec/1000000);
	printf("find:%.6f\n",(float)find.adding.tv_usec/1000000);
	threadset_debug_print(&processor);
	//printf("assign write:%.6f\n",(float)mas2.adding.tv_usec/1000000);
//	printf("assign read:%.6f\n",(float)mas.adding.tv_usec/1000000);
	//printf("writetime:%.6f\n",(float)gt.adding.tv_usec/1000000);
//	printf("readtime:%.6f\n",(float)rt.adding.tv_usec/1000000);
//	printf("wt:%.6f\n",(float)wt->adding.tv_usec/1000000);
//	printf("at:%.6f\n",(float)at->adding.tv_usec/1000000);
*/
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
	return 0;
}
