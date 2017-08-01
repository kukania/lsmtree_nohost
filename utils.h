#ifndef __UTIL_H__
#define __UTIL_H__
#include<sys/time.h>
#include<stdint.h>
#include<unistd.h>
#include"measure.h"
#define KEYT uint32_t
#define FILTERSIZE (1000*6)
#define FILTERFUNC 5
#define FILTERBIT ((1000*6)/8)

#define KEYN 1024
#define PAGESIZE (8192)
#define MUL 24
#define LEVELN 5
#define INPUTSIZE (1024*512)
#define BUSYPOINT 0.7
#define THREADQN 1024
#define THREADNUM 1
#define THREAD //-do thread
//#define DEBUG_THREAD 
//#define NOR //- not read data

#define STARTMERGE 0.7
#define ENDMERGE 0.5
#define MAXC 10
#define MAXNODE 250000
#define SEQUENCE 1
#define READTEST
#define GETTEST

#define CACHENUM 	2
#define CACHETH     100
#define WAITREQN	16
#define WAITMETAN	128


#define SNODE_SIZE (8192)
//#define SKIP_BLOCK ((4096+sizeof(int)*3)*1000)//(snode data+ meta)* # of snode
//#define SKIP_META (sizeof(uint64_t)*2*1024)//snode meta * # of snode + size of skiplist


#ifndef NPRINTOPTION
#define MT(t) measure_stamp((t))
#define MS(t) measure_start((t))
#define ME(t,s) measure_end((t),(s))
#define MP(t) measure_pop((t))
#define MC(t) measure_calc((t))
#define MR(t) measure_res((t))
#define MA(t) measure_adding((t))
#else
#define MS(t) donothing(t)
#define ME(t,s) donothing2((t),(s))
#define MP(t) donothing((t))
#define MC(t) donothing((t))
#endif

#ifndef CPP
#ifndef BOOL
#define BOOL
typedef enum{false,true} bool;
#endif
#endif

#endif
