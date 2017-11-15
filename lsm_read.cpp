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
extern lsmtree *LSM;
extern KEYT *keys;
extern MeasureTime bp;
extern MeasureTime mem;
extern MeasureTime last;
extern MeasureTime buf;
extern MeasureTime find;


int main(){
	MeasureTime mt2;
	measure_init(&mt2);
	lr_inter_init();
	int tfd=open("data/meta.data",O_RDONLY,0666);
	int level_cnt;
	read(tfd,&level_cnt,sizeof(level_cnt));
	for(int i=0; i<level_cnt; i++){
		level *lev=LSM->buf.disk[i];
		level_load(lev,tfd);
	}
	skiplist_load(LSM->memtree,tfd);
	req_t *req;
	KEYT key;
	MS(&mt2);
	for(int i=1; i<=INPUTSIZE; i++){
		req=(req_t*)malloc(sizeof(req_t));
		req->type=2;
		if(i%1024==1){
			printf("%d\n",i);
		}
		if(SEQUENCE==0){
			key=keys[i];
		}
		else{
			key=i;
		}
		req->key=key;
#ifdef ENABLE_LIBFTL
		//printf("main_alloc!\n");
		req->dmaTag=memio_alloc_dma(req->type,&req->value);
#else
		req->value=(char*)malloc(PAGESIZE);
#endif
		lr_make_req(req);
	}
	threadset_read_wait(&processor);
	MT(&mt2);
	threadset_debug_print(&processor);
/*
	for(int i=0; i<LEVELN; i++){
		level *lev=LSM->buf.disk[i];
		if(lev->m_size!=0){
			level_save(lev,tfd);
		}
	}
*/
	printf("bp:%.6f\n",(float)bp.adding.tv_usec/1000000);
	//lr_inter_free();
}
