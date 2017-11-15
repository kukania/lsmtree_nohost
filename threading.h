#ifndef __THREAD_H__
#define __THREAD_H__
#include"lsmtree.h"
#include"utils.h"
#include"LR_inter.h"
#include"skiplist.h"
#include"lockfreeq.h"
#include"lfmpmc.h"
#include<pthread.h>
#include<semaphore.h>
#ifdef M_QUEUE
#include<queue>
using namespace std;
#endif
typedef struct threadset threadset;
typedef struct{
	int start;
	int end;
}range;
typedef struct threading{
	pthread_t id;
	pthread_mutex_t terminate;
	int number;
	sktable *buf_data;
	int dmatag;
	int level;
	bool terminateflag;

	int cache_hit;
	int header_read;
	int notfound;
	MeasureTime waiting;
	threadset *master;
	
	int flag;
	lsmtree_req_t *pre_req[WAITREQN];
	Entry *entry[WAITREQN];
}threading;

typedef struct threadset{
	pthread_mutex_t th_cnt_lock;
	pthread_mutex_t req_lock;
	pthread_mutex_t gc_lock;
	pthread_mutex_t read_lock;
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
#ifdef MULTIQ
	mpmc_bounded_queue_t<void *>* req_q;
	mpmc_bounded_queue_t<void *> *read_q;
	mpmc_bounded_queue_t<void *>* gc_q;
#else
	#ifdef M_QUEUE
		queue<void *> *req_q;
		queue<void *> *read_q;
		queue<void *> *gc_q;
	#else
		spsc_bounded_queue_t<void *>* req_q;
		spsc_bounded_queue_t<void *> *read_q;
		spsc_bounded_queue_t<void *>* gc_q;
	#endif
#endif
	//queue *gc_q;
	int activatednum;
	int max_act;
	int sk_target_number;
	int sk_now_number;

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
