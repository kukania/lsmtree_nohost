#ifndef __UTIL_H__
#define __UTIL_H__
#include<sys/time.h>
#include<stdint.h>
#include<unistd.h>
#include"measure.h"
#define KEYT uint32_t
#define KEYN 1024
#define PAGESIZE (8192)
#define MUL 10
#define LEVELN 8
#define TARGETSIZEVALUE 512
#define GIGASIZE 510
#define MILI 1000000
#define INPUTSIZE 50*MILI//(1024*128*(GIGASIZE))
#define KEYRANGE (1024*128*(256))
#define CACHESIZE (128*(GIGASIZE)) //8kb *CACHESIZE, 128=1MB

//#define NOGC_TEST
#define BLOOM
//#define MONKEY_BLOOM
#define CACHE
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

#define WAITREQN	16
#define WAITMETAN	128

#define PAGENUM (1<<14)
#define SEGSIZE (1<<27)
#define SEGNUM  ((TARGETSIZEVALUE)*8) 
#define MAXPAGE ((TARGETSIZEVALUE)*8*(1<<14)) //1G=8*(1<<14)
#define DTPBLOCK (4)

#define SNODE_SIZE (8192)

#define RAF 1.0 //for monkey bloomfilter
#define FPR 0.2 //normal bloomfilter

#define NODATA UINT_MAX
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
