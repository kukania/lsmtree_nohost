#ifndef __UTIL_H__
#define __UTIL_H__
#include<sys/time.h>
#include<stdint.h>
#include<unistd.h>
#include"measure.h"
#define KEYT uint32_t
#define KEYN 1024
#define PAGESIZE (8192)
#define MUL 24
#define LEVELN 5
#define INPUTSIZE (1024*128*9)
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
#define SEQUENCE 0
#define READTEST
#define GETTEST

#define CACHENUM 	2
#define CACHETH     100
#define WAITREQN	16
#define WAITMETAN	128

#define PAGENUM (4096)
#define SEGNUM  (32*12) 
#define MAXPAGE (32*12*4096) //1G=32*4096
#define MAXPPH	(2*4096);
#define DTPBLOCK (4)

#define SNODE_SIZE (8192)

#define RAF 1
#define FPR 0.2

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
#ifndef BOOL
#define BOOL
#ifndef CPP
typedef enum{false,true} bool;
#endif
#endif
#endif
