#ifndef __BLOOM_H__
#define __BLOOM_H__
#include<stdlib.h>
typedef struct{
	int k;
	int m;
	int n;
	double p;
	char *body;
}BF;

BF* bf_init(int entry,double fpr);
void bf_free(BF *);
inline void BITSET(char &input, char offset){
	char test=1;
	test<<=offset;
	input=input|test;
}
inline bool BITGET(char input, char offset){
	char test=1;
	test<<=offset;
	return input&test;
}
#endif
