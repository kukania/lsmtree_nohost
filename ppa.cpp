#include"ppa.h"
#include"LR_inter.h"
#include"lsmtree.h"
#include"delete_set.h"
#include<string.h>
#include<pthread.h>
KEYT DELETEDKEY;
extern lsmtree *LSM;
extern uint64_t *oob;
#ifdef NOGC_TEST
KEYT temp_test_key;
#endif
void segment_init(segment *input,int start_block_n, int size, bool isdata){
	DELETEDKEY=MAXPAGE+DTPBLOCK*PAGENUM; //move to out place
#ifdef NOGC_TEST
	printf("[NOGC TEST!!!!!!!!!]\n");
#endif
	input->size=size;
	input->blocks=(block*)malloc(sizeof(block)*(size+1));
	input->ppa=new std::queue<KEYT>();
	//input->ppa=(queue*)malloc(sizeof(queue));
	//queue_init(input->ppa);
	/*block init*/
	for(int i=0; i<size+1; i++){
		for(int j=0; j<PAGENUM/8; j++){
			input->blocks[i].bitset[j]=0xFF;
		}
		input->blocks[i].invalid_n=0;
		input->blocks[i].number=start_block_n+i;
		input->blocks[i].erased=true;
	}
	input->reserve_block=start_block_n+size;
	/*ppa input*/
	int target_number=start_block_n*PAGENUM;
	for(int i=start_block_n; i<start_block_n+size; i++){
		for(int j=0; j<PAGENUM; j++){
			input->ppa->push(target_number++);
			//queue_push(input->ppa,target_number++);
		}
	}
	/*mutex init*/
	pthread_mutex_init(&input->ppa_lock,NULL);

	input->isdata=isdata;
	input->start_block_n=start_block_n;
	input->reserve_block_point=0;
}

KEYT getPPA(segment *input, void *req){
#ifdef NOGC_TEST
	return temp_test_key++;
#endif
	pthread_mutex_lock(&input->ppa_lock);
	while(!input->ppa->size()){
	//while(!input->ppa->size){
		if(input->isdata){
			printf("delete data call!!!!\n");
			delete_trim_process_data(input);
		}
		else{
			printf("delete header call!!!!\n");
			delete_trim_process_header(input);
		}
	}
	KEYT res=input->ppa->front();
	input->ppa->pop();
	//KEYT res=queue_pop(input->ppa);
	pthread_mutex_unlock(&input->ppa_lock);
	if(res==17317)
		printf("here!\n");
	return res;
}
KEYT getRPPA(segment *input, void *req){
	KEYT res=input->reserve_block * PAGENUM;
	res+=input->reserve_block_point++;
	if(res==17317)
		printf("here!\n");
	return res;
}
void freePPA(segment *input, KEYT in){
	//queue_push(input->ppa,in);
	input->ppa->push(in);
}
void segment_free(segment *input){
	free(input->blocks);
	free(input);
}

void segment_block_oob_clear(segment *input, int block_num){
	int number=input->blocks[block_num].number;
	for(int i=number*PAGENUM; i<number*PAGENUM+PAGENUM; i++){
		oob[i]=0;
	}
}

void segment_block_init(segment * input, int block_num){
	for(int j=0; j<PAGENUM/8; j++){
		input->blocks[block_num].bitset[j]=0xFF;
	}
	input->blocks[block_num].invalid_n=0;
	input->blocks[block_num].erased=true;
}

void segment_block_change(segment *input, int target_block){
	KEYT res=input->reserve_block * PAGENUM;
	for(int i=input->reserve_block_point; i<PAGENUM; i++){
		freePPA(input,res+i);
	}
	input->reserve_block=input->blocks[target_block].number;
	block temp;
	memcpy(&temp,&input->blocks[target_block],sizeof(block));
	memcpy(&input->blocks[target_block],&input->blocks[input->size],sizeof(block));
	memcpy(&input->blocks[input->size],&temp,sizeof(block));
	input->reserve_block_point=0;
}

void segment_block_free_ppa(segment *input, int block_num){
	int number=input->blocks[block_num].number;
	for(int i=number*PAGENUM; i<(number+1)*PAGENUM; i++){
		freePPA(input,i);
	}
}
