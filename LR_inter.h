#ifndef __LR_INTER_H__
#define __LR_INTER_H__

#include<stdint.h>
#include<pthread.h>
#include"skiplist.h"
#include"utils.h"
//#include"lockfreeq.h"
#ifndef LIBLSM
//#include"request.h"
#endif
/**request type***/
#define LR_READ_T 	32
#define BUFFER_READ 33
#define DISK_READ_T 34
#define LR_WRITE_T 	8
#define LR_COMP_T	4
#define LR_FLUSH_T	16
#define LR_DW_T		1
#define LR_DR_T		3
#define LR_DDR_T	0
#define LR_DDW_T    2
/***/
typedef struct sktable sktable;
typedef struct skiplist skiplist;
typedef struct keyset keyset;
#ifdef LIBLSM
typedef struct req_t{
	int dmaTag;
	char *value;
	uint64_t key;
	uint8_t type;
	int8_t (*end_req)(void*);
}req_t;
#else
	typedef struct req_t req_t;
#endif
typedef struct lsmtree_gc_req_t{
	bool isgc;
	req_t *req;//always NULL
	uint8_t type;
	uint8_t flag;
	void *params[4];
	int8_t (*end_req)(struct lsmtree_gc_req_t *);
	pthread_mutex_t meta_lock;
	uint64_t now_number;
	uint64_t target_number;
	uint64_t seq_number;
	sktable *res;
 	keyset *keys;
	struct lsmtree_gc_req_t *parent;
	int dmatag;
	sktable *compt_headers;
	skiplist * skip_data;

	MeasureTime mt;
}lsmtree_gc_req_t;

typedef struct lsmtree_req_t{
	bool isgc;
	req_t *req;
	uint8_t type;
	uint8_t flag;
	void *params[4];
	int8_t (*end_req)(struct lsmtree_req_t *);
	pthread_mutex_t meta_lock;
	uint64_t now_number;
	uint64_t target_number;
	uint64_t seq_number;
	sktable *res;
 	keyset *keys;
	struct lsmtree_req_t *parent;
	int dmatag;
	struct lsmtree_gc_req_t *gc_parent;
	char *dummy;

	MeasureTime mt;
}lsmtree_req_t;

int8_t lr_make_req(req_t *);
int8_t lr_end_req(lsmtree_req_t *);
int8_t lr_gc_make_req(int8_t);
int8_t lr_gc_end_req(lsmtree_gc_req_t *);
int8_t lr_req_wait(lsmtree_req_t *);
int8_t lr_gc_req_wait(lsmtree_gc_req_t *);
int8_t lr_inter_init();
int8_t lr_inter_free();
int8_t lr_is_gc_needed();
#endif
