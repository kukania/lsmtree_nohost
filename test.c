#include"delete_set.h"
#include"ppa.h"
#include"LR_inter.h"
#include"utils.h"

extern delete_set* dset;
int main(){
	initPPA();
	dset=(delete_set*)malloc(sizeof(delete_set));
	delete_init(dset);
	lsmtree_req_t *temp=(lsmtree_req_t*)malloc(sizeof(lsmtree_req_t));
	for(int i=0; i<INPUTSIZE; i++){
		getPPA((void*)temp);
	}
	for(int i=0; i<INPUTSIZE/2 ;i++){
		delete_ppa(dset,i);
	}
	while(delete_trim_process(dset)){
		printf("deleted!\n");
	}
}
