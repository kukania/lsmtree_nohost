#include"LR_inter.h"
#include"measure.h"
#include"utils.h"
#include"threading.h"
#include"delete_set.h"
#include"ppa.h"
#ifdef ENABLE_LIBFTL
#include"libmemio.h"
extern memio* mio;
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
extern timeval max_time;
extern int big_time_check;
extern timeval max_time1,adding;
extern int big_time_check1;
extern int meta_read_data;

extern int pros_hit;
extern int pros_hit2;
extern int cache_hit;
extern int mem_hit;
extern int write_wait_check;
extern int write_make_check;
extern int read_end_check;
extern int mixed_req_cnt;
extern int allnumber;

extern lsmtree *LSM;
extern delete_set *header_segment;
extern delete_set *data_segment;
extern KEYT *keys;
int cnnt=0;
bool utils_flag;
#define FLAGS 11

#ifdef CACHE
#include"cache.h"
extern cache *CH;
#endif

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
	for(int i=1; i<=INPUTSIZE; i++){
		//printf("%d\n",i);
		req=(req_t*)malloc(sizeof(req_t));
		req->type=1;
		if(i%1000000==0){
			MT(&mt);
			MS(&mt);
		}
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
	/*
	threadset_request_wait(&processor);
	threadset_gc_wait(&processor);
	threadset_request_wait(&processor);*/
	//while(mio->tagQ->size()!=128){}
	MT(&mt);
	//lr_wait();
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
/*
	char location[]="data/meta.data";
	lsmtree_save(LSM,location);*/
	
	processor.threads[0].flag=0;
	printf("read!\n");
	pthread_t thr;
	utils_flag=true;
	write_wait_check=0;
	write_make_check=0;
	read_end_check=0;
	//pthread_create(&thr,NULL,util_check,NULL);
	threadset_debug_init(&processor);
	MS(&mt);
	//printf("??");
	int divide=INPUTSIZE/3;
	for(int i=1; i<=INPUTSIZE*2; i++){
		req=(req_t*)malloc(sizeof(req_t));
		mixed_req_cnt++;/*
		if(rand()%10==0)
			req->type=1;
		else*/
			req->type=2;
		if(i%1000000==0){
			MT(&mt);
			threadset_debug_print(&processor);
			MS(&mt);
		}
		if(i%1024==1){
	//		printf("%d\n",i);
		}
		if(SEQUENCE==0){
			//key=keys[i-1];
			key=rand()%(KEYRANGE)+1;
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
	//threadset_read_wait(&processor);
	//threadset_mixed_wait();
	//while(mio->tagQ->size()!=128){}
	MT(&mt);/*
	printf("meta_read_data:%d\n",meta_read_data);
	//measure_end(&mt,"read_end");
	printf("mem:%ld && %.6f\n",mem.adding.tv_sec,(float)mem.adding.tv_usec/1000000);
//	printf("last:%.6f\n",(float)last.adding.tv_usec/1000000);
	printf("buf:%ld && %.6f\n",buf.adding.tv_sec,(float)buf.adding.tv_usec/1000000);
	printf("bp:%ld && %.6f\n",bp.adding.tv_sec,(float)bp.adding.tv_usec/1000000);
	printf("find:%ld && %.6f\n",find.adding.tv_sec,(float)find.adding.tv_usec/1000000);
	printf("\nallnumber: %d\n",allnumber);
*/
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
//	lr_inter_free();
	return 0;
}
