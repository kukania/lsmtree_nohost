#ifndef __THREAD_H__
#define __THREAD_H__
#include"lsmtree.h"
#include"utils.h"
#include"LR_inter.h"
#include"skiplist.h"
#include"lockfreeq.h"
#include"lfmpmc.h"
#include"lsm_cache.h"
#include<pthread.h>
#include<semaphore.h>
typedef struct threadset threadset;
typedef struct threading{
	pthread_t id;
	pthread_mutex_t activated_check;
	pthread_cond_t activated_cond;
	pthread_mutex_t terminate;
	int number;
	sktable *buf_data;
	int dmatag;
	int level;
	bool isactivated;
	bool terminateflag;

	int cache_hit;
	int header_read;
	MeasureTime waiting;
	threadset *master;
}threading;

typedef struct threadset{
	pthread_mutex_t th_cnt_lock;
	pthread_mutex_t req_lock;
	pthread_mutex_t gc_lock;
	pthread_cond_t gc_cond;
	pthread_cond_t gc_full_cond;
	bool write_flag;
#ifdef DEBUG_THREAD
	pthread_mutex_t debug_m;
	int errcnt;
	int counter;
#endif
	threading threads[THREADNUM];
	threading gc_thread;
	spsc_bounded_queue_t<void *>* req_q;
	mpmc_bounded_queue_t<void *> *read_q;
	spsc_bounded_queue_t<void *>* gc_q;
	//queue *gc_q;
	int activatednum;
	int max_act;
	int sk_target_number;
	int sk_now_number;

	lsm_cache mycache;
}threadset;
void threading_clear(threading *);
void threading_init(threading *);
void threadset_init(threadset*);
void threadset_start(threadset*);
void threadset_assign(threadset*,lsmtree_req_t *);
void threadset_read_assign(threadset *,lsmtree_req_t *);
void threadset_end(threadset*);
void threadset_clear(threadset*);
void threadset_request_wait(threadset*);
void threadset_read_wait(threadset*);
void threadset_gc_assign(threadset*,lsmtree_gc_req_t *);
void threadset_gc_wait(threadset*);
void threadset_debug_print(threadset*);
#endif
