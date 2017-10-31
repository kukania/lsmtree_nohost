#include"utils.h"
#include"bloomfilter.h"
#include<stdio.h>
#include<stdlib.h>

int main(){
	BF* filter=bf_init(INPUTSIZE,0.2);
	for(int i=0; i<INPUTSIZE/10; i++){
		bf_set(filter,i);
	}

	int cnt=0;
	for(int i=0; i<INPUTSIZE; i++){
		if(bf_check(filter,i)){
			cnt++;
		}
	}
	printf("cnt:%d inputsize:%d\n",cnt,INPUTSIZE/10);
	printf("%f\n",(double)(cnt-INPUTSIZE/10)/INPUTSIZE);
}
