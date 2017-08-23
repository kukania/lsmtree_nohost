#include"bloomfilter.h"
#include"measure.h"
#include<math.h>
#include<stdio.h>
#include<string.h>

extern MeasureTime mem;
KEYT hashfunction(KEYT key){
	key = ~key + (key << 15); // key = (key << 15) - key - 1;
	key = key ^ (key >> 12);
	key = key + (key << 2);
	key = key ^ (key >> 4);
	key = key * 2057; // key = (key + (key << 3)) + (key << 11);
	key = key ^ (key >> 16);
	return key;
}
BF* bf_init(int entry, double fpr){
	BF *res=(BF*)malloc(sizeof(BF));
	res->n=entry;
	res->m=ceil((res->n * log(fpr)) / log(1.0 / (pow(2.0, log(2.0)))));
	res->k=round(log(2.0) * (double)res->m / res->n);
	int targetsize=res->m/8;
	if(res->m%8)
		targetsize++;
	res->body=(char*)malloc(targetsize);
	memset(res->body,0,targetsize);
	res->p=fpr;
	res->targetsize=targetsize;
	return res;
}

uint64_t bf_bits(int entry, double fpr){
	uint64_t n=entry;
	uint64_t m=ceil((n * log(fpr)) / log(1.0 / (pow(2.0, log(2.0)))));
	int targetsize=m/8;
	if(m%8)
		targetsize++;
	return targetsize;
}
void bf_set(BF *input, KEYT key){
	KEYT temp=hashfunction(key);
	for(int i=0; i<input->k; i++){
		KEYT h=hashfunction(key+i);
		h%=input->m;
		int block=h/8;
		int offset=h%8;
		BITSET(input->body[block],offset);
	}
}

bool bf_check(BF* input, KEYT key){
	KEYT temp=hashfunction(key);
	for(int i=0; i<input->k; i++){
		KEYT h=hashfunction(key+i);
		h%=input->m;
		int block=h/8;
		int offset=h%8;
		if(!BITGET(input->body[block],offset))
			return false;
	}
	return true;
}
void bf_free(BF *input){
	free(input->body);
	free(input);
}
/*
int main(){
	int check=0;
	printf("test\n");
	BF *test=bf_init(KEYN,0.01);
	for(int i=0; i<1024; i++){
		bf_set(test,i);
	}

	for(int i=0; i<100000; i++){
		if(bf_check(test,i)){
			printf("%d\n",i);
			check++;
		}
	}

	printf("%d\n",check);
}*/
