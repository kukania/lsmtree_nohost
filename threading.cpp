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
#include<unistd.h>

#define __STDC_FORMAT_MACROS
extern threadset processor;
extern pthread_mutex_t dfd_lock;

pthread_mutex_t read_lock;
pthread_cond_t read_cond;
pthread_mutex_t write_lock;
pthread_cond_t write_cond;

extern MeasureTime mt;
extern MeasureTime bp;
MeasureTime gt;
MeasureTime *wt;
MeasureTime *at;
MeasureTime rt;

int gc_end_check;
int read_end_check;
int write_make_check;
extern int write_wait_check;
pthread_mutex_t write_check_lock;
int temp_check;
void threading_init(threading *input){
	pthread_mutex_init(&write_check_lock,NULL);

	pthread_mutex_init(&input->terminate,NULL);

	input->terminateflag=false;
	input->buf_data=NULL;

	input->cache_hit=0;
	input->header_read=0;
	input->notfound=0;
	measure_init(&input->waiting);

	for(int i=0; i<WAITREQN; i++){
		input->pre_req[i]=NULL;
		input->entry[i]=NULL;
	}
	input->flag=0;
}

void threading_clear(threading *input){
	pthread_mutex_destroy(&input->terminate);
	if(input->buf_data!=NULL)
		free(input->buf_data);
}
void threadset_init(threadset* input){
	gc_end_check=read_end_check=write_make_check=0;

	measure_init(&gt);
	measure_init(&rt);
	pthread_mutex_init(&input->req_lock,NULL);
	pthread_mutex_init(&input->th_cnt_lock,NULL);
	pthread_mutex_init(&input->gc_lock,NULL);
	pthread_mutex_init(&input->read_lock,NULL);

#ifdef M_QUEUE

#else
	pthread_mutex_lock(&input->gc_lock);
#endif

	pthread_mutex_init(&read_lock,NULL);
	pthread_cond_init(&read_cond,NULL);
	pthread_mutex_init(&write_lock,NULL);
	pthread_cond_init(&write_cond,NULL);

	pthread_cond_init(&input->gc_cond,NULL);
	pthread_cond_init(&input->gc_full_cond,NULL);
#ifdef DEBUG_THREAD
	pthread_mutex_init(&input->debug_m,NULL);
#endif

	for(int i=0; i<THREADNUM;i ++){
		threading_init(&input->threads[i]);
		input->threads[i].master=input;
		input->threads[i].number=i;
	}
	threading_init(&input->gc_thread);
#ifdef DEUBG_THREAD
	input->counter=0;
	input->errcnt=0;
#endif
#ifdef MULTIQ
	input->req_q=new mpmc_bounded_queue_t<void*>(THREADQN);
	input->read_q=new mpmc_bounded_queue_t<void*>(THREADQN);
	input->gc_q=new mpmc_bounded_queue_t<void*>(2);
#else
	#ifdef M_QUEUE
		input->req_q=new std::queue<void *>();
		input->read_q=new std::queue<void *>();
		input->gc_q=new std::queue<void *>();
	#else
		input->req_q=new spsc_bounded_queue_t<void*>(THREADQN);
		input->read_q=new spsc_bounded_queue_t<void*>(THREADQN);
		input->gc_q=new spsc_bounded_queue_t<void*>(2);
	#endif
#endif
	//	input->req_q=create_queue();

	cache_init(&input->mycache);
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

	cache_clear(&input->mycache);
}
bool read_flag;
void* thread_gc_main(void *input){
	int number=0;
	threadset *master=(threadset*)input;
	threading *myth=&master->gc_thread;
	while(1){
		lsmtree_gc_req_t *lsm_req;
		void *req_data;
#ifdef M_QUEUE
		while(!master->gc_q->size()){}
		pthread_mutex_lock(&master->gc_lock);
		req_data=master->gc_q->front();
		master->gc_q->pop();
		pthread_mutex_unlock(&master->gc_lock);
#else
		while(!master->gc_q->dequeue(&req_data)){
			pthread_mutex_lock(&master->gc_lock);
		}
#endif
		if(read_flag) break;
		lsm_req=(lsmtree_gc_req_t*)req_data;
		if(lsm_req==NULL){
			pthread_mutex_lock(&myth->terminate);
			if(myth->terminateflag){
				pthread_mutex_unlock(&myth->terminate);
				break;
			}
			pthread_mutex_unlock(&myth->terminate);
			continue;
		}
		pthread_mutex_lock(&myth->terminate);
		if(myth->terminateflag){
			pthread_mutex_unlock(&myth->terminate);
			break;
		}
		else{
			pthread_mutex_unlock(&myth->terminate);
			//printf("[%d]doing!!",number);
			lsmtree *LSM=(lsmtree*)lsm_req->params[3];
			level* src,*des;
			skiplist* data;
			char *res;
			KEYT *key;
			Entry *result_entry;
			bool flag=false;
			//lsm_req->flag for oob
			for(int i=LEVELN-1; i>=0; i--){
				if(LSM->buf.disk[i]->size==0)
					continue;
				if(i!=0){
					if(LSM->buf.disk[i]->size + LSM->buf.disk[i-1]->m_size >= LSM->buf.disk[i]->m_size){
						if(i==LEVELN-1){
						
						}
						else{
							src=LSM->buf.disk[i];
							des=LSM->buf.disk[i+1];
							pthread_mutex_init(&lsm_req->meta_lock,NULL);
							lsm_req->flag=i+2-1;//lsm_req->flag for oob
							flag=i+2-1;
							compaction(LSM,src,des,NULL,lsm_req);
						}
					}
				}
				
				if(LSM->buf.disk[i]->size >= LSM->buf.disk[i]->m_size){
					if(i==LEVELN-1){
					}
					else{
						if(!flag)
							pthread_mutex_init(&lsm_req->meta_lock,NULL);
						src=LSM->buf.disk[i];
						des=LSM->buf.disk[i+1];
						lsm_req->flag=i+2-1;//lsm_req->flag for oob
						flag=i+2;
						compaction(LSM,src,des,NULL,lsm_req);
					}
				}
			}
			if(!flag){
				pthread_mutex_init(&lsm_req->meta_lock,NULL);
			}
			data=(skiplist*)lsm_req->params[2];
			lsm_req->now_number=0;
			lsm_req->target_number=0;
			lsm_req->flag=1-1;//lsm_req->flag for oob
			result_entry=make_entry(data->start,data->end,0);
//			result_entry->bitset=data->bitset;
//			skiplist_meta_free(data);
			lsm_req->data=(char*)data;
			compaction(LSM,NULL,LSM->buf.disk[0],result_entry,lsm_req);
			lsm_req->end_req(lsm_req);
			//printf("[%d]done-----\n",number++);
		}
	}
	return NULL;
}
bool check_flll=0;
void* thread_main(void *input){
	int cnt=0;
	//
	cpu_set_t cpuset;
   	//CPU_ZERO(&cpuset);
   	//CPU_SET(2, &cpuset);
	pthread_t current_thread = pthread_self();
//	pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
	//nice(-20);
	//*/
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
	bool header_flag=false;
	while(1){
		lsmtree_req_t *lsm_req;
		void *data=NULL;
#ifdef M_QUEUE
		bool isread=false;
		while(1){
			if(master->read_q->size()){
				isread=true;
				break;
			}
			if(master->req_q->size()){
				isread=false;
				break;
			}
		}
		if(isread){
			pthread_mutex_lock(&master->read_lock);
			data=master->read_q->front();
			master->read_q->pop();
			pthread_mutex_unlock(&master->read_lock);
		}
		else{
			pthread_mutex_lock(&master->req_lock);
			data=master->req_q->front();
			master->req_q->pop();
			pthread_mutex_unlock(&master->req_lock);
		}
#else
		while(1){
			if(!master->read_q->dequeue(&data)){
				if(!master->req_q->dequeue(&data)){
					continue;
				}
				break;
			}
			break;
		}
#endif
		/*
		if(check_flll==0){
			check_flll=true;
			MS(&gt);
		}
		else{
			MT(&gt);
			MS(&gt);
		}*/
		lsm_req=(lsmtree_req_t*)data;
		pthread_mutex_lock(&myth->terminate);
		if(myth->terminateflag){
			pthread_mutex_unlock(&myth->terminate);
			break;
		}
		else{
			pthread_mutex_unlock(&myth->terminate);
			KEYT *key=(KEYT*)lsm_req->params[1];
			myth->flag=1;
			char *value;		
			lsmtree *LSM=(lsmtree*)lsm_req->params[3];
			int test_num;

			switch(lsm_req->type){
				case DISK_READ_T:
					value=(char*)lsm_req->params[2];
					test_num=thread_level_get(LSM,*key,myth,value,lsm_req,lsm_req->flag);
					if(test_num<=0){
	//					printf("[%u]not_found\n",*key);
						myth->notfound++;
#ifdef SERVER
						lsm_req->req->type_info->type=-1;
#endif
						lsm_req->end_req(lsm_req);
					}
					if(test_num==6)
						lsm_req->end_req(lsm_req);
					break;
				case LR_READ_T:
					value=(char*)lsm_req->params[2];
					pthread_mutex_init(&lsm_req->meta_lock,NULL);
					test_num=thread_get(LSM,*key,myth,value,lsm_req);
					if(test_num==0){
						//printf("[%u]not_found\n",*key);
						myth->notfound++;
#ifdef SERVER
						lsm_req->req->type_info->type=-1;
#endif
						lsm_req->end_req(lsm_req);
					}
					if(test_num==2)
						lsm_req->end_req(lsm_req);
					if(test_num==4)
						header_flag=true;
					if(test_num==6)
						lsm_req->end_req(lsm_req);
					break;
				case LR_WRITE_T:
					if(is_flush_needed(LSM)){
						lr_gc_make_req(0);
					}
					value=(char*)lsm_req->params[2];
					put(LSM,*key,value,lsm_req);
					break;
				case LR_DELETE_T:
					if(is_flush_needed(LSM)){
						lr_gc_make_req(0);
					}
					value=NULL;
					put(LSM,*key,NULL,lsm_req);
					break;
				default:
					break;
			}
			myth->flag=0;
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
	if(req==NULL)
		printf("assign NULL\n");
#ifdef M_QUEUE
	while(input->read_q->size()>THREADQN){
	
	}
	pthread_mutex_lock(&input->read_lock);
	input->read_q->push((void*)req);
	pthread_mutex_unlock(&input->read_lock);
#else
	while(!input->read_q->enqueue(req)){}
#endif
	pthread_cond_broadcast(&read_cond);
}
/*
   MeasureTime wt;
   MeasureTime at;
   */
void threadset_assign(threadset* input, lsmtree_req_t *req){
#ifdef M_QUEUE
	while(input->req_q->size()>THREADQN){
	
	}
	pthread_mutex_lock(&input->req_lock);
	input->req_q->push((void*)req);
	pthread_mutex_unlock(&input->req_lock);
#else
	while(!input->req_q->enqueue(req)){
	}
#endif

	pthread_cond_broadcast(&write_cond);
}
void threadset_gc_assign(threadset* input ,lsmtree_gc_req_t *req){
#ifdef M_QUEUE
	while(input->gc_q->size()>1){}
	pthread_mutex_lock(&input->gc_lock);
	input->gc_q->push((void*)req);
	pthread_mutex_unlock(&input->gc_lock);
#else
	while(!input->gc_q->enqueue(req)){}
#endif
}
void threadset_gc_wait(threadset *input){
	while(gc_end_check){}
}
void threadset_read_wait(threadset *input){
	while(read_end_check<=INPUTSIZE-20){
		//printf("%d\n",read_end_check);
		
	}
}
void threadset_request_wait(threadset *input){
	while(write_make_check-1030>write_wait_check){
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
		printf("header_read : %d\n",temp.header_read);
		printf("------------------------\n");

		all_cache_hit+=temp.cache_hit;
		all_header_read+=temp.header_read;
	}

	printf("\n\n======================\n");
	printf("all summary\n");
	printf("cache_hit : %d\n",all_cache_hit);
	printf("NOTFOUND : %.3f\n",(float)input->threads[0].notfound/INPUTSIZE);
	printf("header_read: %d [%d]\n",all_header_read,INPUTSIZE-KEYN);
	printf("RAFl: %.3f\n",(float)all_header_read/(INPUTSIZE-KEYN));
	printf("waiting time : %.6f\n",all_waiting_time);
	printf("======================\n");

}
