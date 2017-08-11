#include"ppa.h"
#include"delete_set.h"
#include"LR_inter.h"
extern delete_set *dset;
std::queue<KEYT> ppa;
void initPPA(){
	for(int i=0; i<MAXPAGE; i++){
		ppa.push(i);	
	}
}
KEYT getPPA(){
	while(ppa.size()==0){
		if(delete_get_victim(dset)<0){
			lr_gc_make_req(1);
		}	
		delete_trim_process(dset);
	}
	KEYT res=ppa.front();
	ppa.pop();
	return res;
}
void freePPA(KEYT in){
	ppa.push(in);
}
