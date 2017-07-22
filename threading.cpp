#include"threading.h"
#include"LR_inter.h"
#include"skiplist.h"
#include"lsmtree.h"
#include"measure.h"
#include<stdlib.h>
#include<limits.h>
#include<stdio.h>
#include<string.h>
#include<inttypes.h>
#include<signal.h>
#include<errno.h>

#define __STDC_FORMAT_MACROS
extern threadset processor;
extern pthread_mutex_t dfd_lock;

pthread_mutex_t read_lock;
pthread_cond_t read_cond;

extern MeasureTime mt;
MeasureTime gt;
MeasureTime *wt;
MeasureTime *at;
MeasureTime rt;
void threading_init(threading *input){
	pthread_mutex_init(&input->activated_check,NULL);
	pthread_cond_init(&input->activated_cond,NULL);
	pthread_mutex_init(&input->terminate,NULL);

	input->isactivated=false;
	input->terminateflag=false;
	input->buf_data=NULL;

	input->cache_hit=0;
	input->header_read=0;
	measure_init(&input->waiting);
}

void threading_clear(threading *input){
	pthread_mutex_destroy(&input->activated_check);
	pthread_mutex_destroy(&input->terminate);
	if(input->buf_data!=NULL)
		free(input->buf_data);
}
void threadset_init(threadset* input){
	measure_init(&gt);
	measure_init(&rt);
	pthread_mutex_init(&input->req_lock,NULL);
	pthread_mutex_init(&input->th_cnt_lock,NULL);
	pthread_mutex_init(&input->gc_lock,NULL);

	pthread_mutex_init(&read_lock,NULL);
	pthread_cond_init(&read_cond,NULL);

	pthread_cond_init(&input->gc_cond,NULL);
	pthread_cond_init(&input->gc_full_cond,NULL);
#ifdef DEBUG_THREAD
	pthread_mutex_init(&input->debug_m,NULL);
#endif

	for(int i=0; i<THREADNUM;i ++){
		threading_init(&input->threads[i]);
		input->threads[i].number=i;
	}
	threading_init(&input->gc_thread);
#ifdef DEUBG_THREAD
	input->counter=0;
	input->errcnt=0;
#endif
	input->activatednum=input->max_act=0;
	input->req_q=new spsc_bounded_queue_t<void*>(THREADQN);
	input->read_q=new mpmc_bounded_queue_t<void*>(THREADQN);
	input->gc_q=new spsc_bounded_queue_t<void*>(2);
	//	input->req_q=create_queue();
}

void threadset_clear(threadset *input){
	for(int i=0; i<THREADNUM; i++){
		threading_clear(&input->threads[i]);
	}
	threading_clear(&input->gc_thread);
	pthread_mutex_destroy(&input->req_lock);
	pthread_mutex_destroy(&input->th_cnt_lock);
	pthread_mutex_destroy(&input->gc_lock);

	pthread_mutex_destroy(&read_lock);
	pthread_cond_destroy(&read_cond);
	pthread_cond_destroy(&input->gc_cond);
	pthread_cond_destroy(&input->gc_full_cond);

#ifdef DEBUG_THREAD
	pthread_mutex_destroy(&input->debug_m);
#endif
	lsmtree_req_t *req_temp;
	lsmtree_gc_req_t *gc_temp;
	delete input->req_q;
	delete input->gc_q;
}
void* thread_gc_main(void *input){
	int number=0;
	threadset *master=(threadset*)input;
	threading *myth=&master->gc_thread;
	while(1){
		lsmtree_gc_req_t *lsm_req;
		void *req_data;
		pthread_mutex_lock(&myth->activated_check);
		myth->isactivated=false;
		pthread_cond_broadcast(&myth->activated_cond);
		pthread_mutex_unlock(&myth->activated_check);	

		while(!master->gc_q->dequeue(&req_data)){}
		lsm_req=(lsmtree_gc_req_t*)req_data;
		if(lsm_req==NULL){
			pthread_mutex_lock(&myth->terminate);
			if(myth->terminateflag){
				pthread_mutex_unlock(&myth->terminate);
				//		pthread_mutex_unlock(&master->gc_lock);
				break;
			}
			pthread_mutex_unlock(&myth->terminate);
			//	pthread_mutex_unlock(&master->gc_lock);
			continue;
		}
		pthread_mutex_lock(&myth->terminate);
		if(myth->terminateflag){
			pthread_mutex_unlock(&myth->terminate);
			break;
		}
		else{
			pthread_mutex_lock(&myth->activated_check);
			myth->isactivated=true;
			pthread_cond_broadcast(&myth->activated_cond);
			pthread_mutex_unlock(&myth->activated_check);

			pthread_mutex_unlock(&myth->terminate);
			//printf("[%d]doing!!",number);
			lsmtree *LSM=(lsmtree*)lsm_req->params[3];
			level* src,*des;
			skiplist* data;
			char *res;
			KEYT *key;
			Entry *result_entry;
			bool flag=false;
			for(int i=LEVELN-1; i>=0; i--){
				if(LSM->buf.disk[i]==NULL)
					break;
				if(LSM->buf.disk[i]->size>=LSM->buf.disk[i]->m_size){
					flag=true;
					src=LSM->buf.disk[i];
					des=LSM->buf.disk[i+1];
					lsm_req->now_number=0;
					lsm_req->target_number=0;
					pthread_mutex_init(&lsm_req->meta_lock,NULL);
					compaction(LSM,src,des,NULL,lsm_req);
				}
			}
			if(!flag){
				pthread_mutex_init(&lsm_req->meta_lock,NULL);
			}
			data=(skiplist*)lsm_req->params[2];
			lsm_req->now_number=0;
			lsm_req->target_number=0;
			result_entry=make_entry(data->start,data->end,write_data(LSM,data,lsm_req));
			skiplist_meta_free(data);
			compaction(LSM,NULL,LSM->buf.disk[0],result_entry,lsm_req);
			lsm_req->end_req(lsm_req);
			//printf("[%d]done-----\n",number++);
		}
	}
	return NULL;
}
void* thread_main(void *input){
	int cnt=0;
	threadset *master=(threadset*)input;
	int number;
	for(int i=0; i<THREADNUM; i++){
		if(master->threads[i].id==pthread_self()){
			number=i;
			break;
		}
	}
	threading *myth=&master->threads[number];
	int nullvalue=0;
	bool flag=false;
	while(1){
		lsmtree_req_t *lsm_req;
		void *data;
		pthread_mutex_lock(&myth->activated_check);
		myth->isactivated=false;
		pthread_cond_broadcast(&myth->activated_cond);
		pthread_mutex_unlock(&myth->activated_check);

		if(number==0){
			if(!master->req_q->dequeue(&data)){
				if(!master->read_q->dequeue(&data)){
					continue;
				}
			}
		}
		else{
			if(!master->read_q->dequeue(&data)){
				pthread_cond_wait(&read_cond,&read_lock);
				master->read_q->dequeue(&data);
				pthread_mutex_unlock(&read_lock);
			}
			//	while(!master->read_q->dequeue(&data)){}
		}

		lsm_req=(lsmtree_req_t*)data;
		pthread_mutex_lock(&myth->terminate);
		if(myth->terminateflag){
			pthread_mutex_unlock(&myth->terminate);
			break;
		}
		else{
			pthread_mutex_lock(&myth->activated_check);
			myth->isactivated=true;
			pthread_cond_broadcast(&myth->activated_cond);
			pthread_mutex_unlock(&myth->activated_check);

			pthread_mutex_unlock(&myth->terminate);

			KEYT *key=(KEYT*)lsm_req->params[1];
			//			printf("%lu\n",*key);
			//printf("size:%llu\n",master->req_q->size());
			char *value;		
			lsmtree *LSM=(lsmtree*)lsm_req->params[3];
			int test_num;

			switch(lsm_req->type){
				case LR_READ_T:
					value=(char*)lsm_req->params[2];				
					pthread_mutex_init(&lsm_req->meta_lock,NULL);
					//measure_start(&mt2);
					//MS(&rt);
					test_num=thread_get(LSM,*key,myth,value,lsm_req);
					if(test_num==0){
						printf("[%u]not_found\n",*key);
						lsm_req->end_req(lsm_req);
					}
					if(test_num==2)
						lsm_req->end_req(lsm_req);
					//MA(&rt);
					//printf("%d",cnt++);
					//measure_end(&mt2,"get");
					//	lsm_req->end_req(lsm_req);
					break;
				case LR_WRITE_T:
					//MS(&gt);
					if(is_flush_needed(LSM)){
						lr_gc_make_req(0);
					}
					value=(char*)lsm_req->params[2];
					if(*key>100000000){
						printf("??");
						sleep(10);
					}
					put(LSM,*key,value,lsm_req);
					//MA(&gt);
					break;
				default:
					break;
			}
			//myth->isactivated=false;
			//		ME(&mt2,"put");
		}
	}
	return NULL;
}
void threadset_start(threadset* input){
	for(int i=0; i<THREADNUM; i++){
		input->threads[i].number=i;
		pthread_create(&input->threads[i].id,NULL,&thread_main,(void*)input);	
	}
	pthread_create(&input->gc_thread.id,NULL,&thread_gc_main,(void*)input);
}
void threadset_read_assign(threadset* input, lsmtree_req_t *req){
	while(!input->read_q->enqueue(req)){}
	pthread_cond_broadcast(&read_cond);
}
/*
   MeasureTime wt;
   MeasureTime at;
 */
void threadset_assign(threadset* input, lsmtree_req_t *req){
	while(!input->req_q->enqueue(req)){
	}
}
void threadset_gc_assign(threadset* input ,lsmtree_gc_req_t *req){
	while(!input->gc_q->enqueue(req)){}
}
void threadset_gc_wait(threadset *input){
	bool emptyflag=false;
	while(1){
		pthread_mutex_lock(&input->gc_thread.activated_check);
		if(input->gc_thread.isactivated){
			pthread_cond_wait(&input->gc_thread.activated_cond,&input->gc_thread.activated_check);
		}
		else{
			if(emptyflag){
				pthread_mutex_unlock(&input->gc_thread.activated_check);
				return;
			}
		}
		pthread_mutex_unlock(&input->gc_thread.activated_check);
		if(input->gc_q->isempty()){
			emptyflag=true;
		}
	}
}
void threadset_read_wait(threadset *input){
	bool emptyflag=false;
	int i=0;
	while(1){
		pthread_mutex_lock(&input->threads[i].activated_check);
		if(input->threads[i].isactivated){
			pthread_cond_wait(&input->threads[i].activated_cond,&input->threads[i].activated_check);
			pthread_mutex_unlock(&input->threads[i].activated_check);
		}
		else{
			pthread_mutex_unlock(&input->threads[i].activated_check);
			if(emptyflag){
				i++;
				if(i==THREADNUM)
					return;
			}
		}
		if(input->read_q->isempty()){
			emptyflag=true;
		}
	}
}
void threadset_request_wait(threadset *input){
	bool emptyflag=false;
	while(1){
		pthread_mutex_lock(&input->threads[0].activated_check);
		if(input->threads[0].isactivated){
			pthread_cond_wait(&input->threads[0].activated_cond,&input->threads[0].activated_check);
		}
		else{
			if(emptyflag){
				pthread_mutex_unlock(&input->threads[0].activated_check);
				return;
			}
		}
		pthread_mutex_unlock(&input->threads[0].activated_check);
		if(input->req_q->isempty()){
			emptyflag=true;
		}
	}
}
void threadset_end(threadset *input){
	threadset_gc_wait(input);
	pthread_mutex_lock(&input->gc_thread.terminate);
	input->gc_thread.terminateflag=true;
	pthread_mutex_unlock(&input->gc_thread.terminate);
	pthread_join(input->gc_thread.id,NULL);

	threadset_request_wait(input);
	for(int i=0; i<THREADNUM; i++){
		pthread_mutex_lock(&input->threads[i].terminate);
		input->threads[i].terminateflag=true;
		pthread_mutex_unlock(&input->threads[i].terminate);
		pthread_join(input->threads[i].id,NULL);
	}
	return;
}

void threadset_debug_print(threadset *input){
	int all_cache_hit=0;
	int all_header_read=0;
	float all_waiting_time=0.0f;
	threading temp;
	for(int i=0; i<THREADNUM; i++){
		temp=input->threads[i];
		printf("\n[threadnumber:%d]\n",i);
		printf("cache_hit : %d\n",temp.cache_hit);
		printf("header_read : %d\n",temp.header_read);
		printf("waiting time : %.6f\n",(float)temp.waiting.adding.tv_usec/1000000);
		printf("------------------------\n");

		all_cache_hit+=temp.cache_hit;
		all_header_read+=temp.header_read;
		all_waiting_time+=(float)temp.waiting.adding.tv_usec/1000000;
	}

	printf("\n\n======================\n");
	printf("all summary\n");
	printf("cache_hit : %d\n",all_cache_hit);
	printf("header_read: %d\n",all_header_read);
	printf("waiting time : %.6f\n",all_waiting_time);
	printf("======================\n");

}
