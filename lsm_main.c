#include"LR_inter.h"
#include"measure.h"
#include"utils.h"
#include"threading.h"
#ifndef NDMA
#include"libmemio.h"
#endif
#include<sys/types.h>
#include<sys/stat.h>
#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
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
extern int end_counter;
int cnnt=0;
int main(){
	wt=at=NULL;
	lr_inter_init();
	req_t * req;
	KEYT key;
	measure_init(&mt);
	int fd;
	printf("start!\n");
	measure_start(&mt);
	int cnt=1;
	for(int i=1; i<=INPUTSIZE; i++){
		//printf("%d\n",i);
		req=(req_t*)malloc(sizeof(req_t));
		req->type=1;
		if(SEQUENCE==0){
			key=rand()%INPUTSIZE+1;
		}
		else{
			key=i;
		}
			req->key=key;
#ifndef NDMA
		req->dmaTag=memio_alloc_dma(req->type,&req->value);
#else
		req->value=(char*)malloc(PAGESIZE);
#endif
		memcpy(req->value,&key,sizeof(KEYT));
		lr_make_req(req);
	}
//	printf("throw all write req!\n");
	threadset_request_wait(&processor);
	threadset_gc_wait(&processor);
	measure_end(&mt,"write_wait");

	wt=(MeasureTime*)malloc(sizeof(MeasureTime));
	at=(MeasureTime*)malloc(sizeof(MeasureTime));
	measure_init(wt);
	measure_init(at);
	printf("read!\n");
	measure_start(&mt);
	//printf("??");
	for(int i=1; i<=INPUTSIZE; i++){
		req=(req_t*)malloc(sizeof(req_t));
		req->type=2;
	//	if(i%1024==0)
	//		printf("%d throw\n",i);
		if(SEQUENCE==0){
			key=rand()%INPUTSIZE+1;
		}
		else{
			key=i;
		}
		req->key=key;
		key=i;
#ifndef NDMA
		//printf("main_alloc!\n");
		req->dmaTag=memio_alloc_dma(req->type,&req->value);
#else
		req->value=(char*)malloc(PAGESIZE);
#endif
		lr_make_req(req);
	}
	//printf("throw all read req!\n");
	threadset_request_wait(&processor);
	measure_end(&mt,"read_end");
	//measure_end(&mt,"read_end");
//	printf("mem:%.6f\n",(float)mem.adding.tv_usec/1000000);
//	printf("last:%.6f\n",(float)last.adding.tv_usec/1000000);
//	printf("buf:%.6f\n",(float)buf.adding.tv_usec/1000000);
//	printf("bp:%.6f\n",(float)bp.adding.tv_usec/1000000);
//	printf("find:%.6f\n",(float)find.adding.tv_usec/1000000);
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
}
