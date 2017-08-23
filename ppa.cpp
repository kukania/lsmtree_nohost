#include"ppa.h"
#include"delete_set.h"
#include"LR_inter.h"
#include"lsmtree.h"
extern delete_set *dset;
extern lsmtree *LSM;
KEYT DELETEDKEY;
std::queue<KEYT> ppa;
pthread_mutex_t physical_gc_lock;
void initPPA(){
	pthread_mutex_init(&physical_gc_lock,NULL);
	DELETEDKEY=MAXPAGE;
	for(int i=1; i<MAXPAGE; i++){
		ppa.push(i);	
	}
}
KEYT getPPA(void *req){
	pthread_mutex_lock(&physical_gc_lock);
	while(ppa.size()<2048){
		if(delete_get_victim(dset)<0){
			for(int i=LEVELN-1; i>=1; i--){
				if(LSM->buf.disk[i]->size==0)
					continue;
				for(int j=i-1; j>=0; j--){
					lsmtree_gc_req_t *req=(lsmtree_gc_req_t*)malloc(sizeof(lsmtree_gc_req_t));
					req->end_req=lr_gc_end_req;
					level * src=LSM->buf.disk[j];
					level *des=LSM->buf.disk[i];
					req->now_number=0;
					req->target_number=0;
					pthread_mutex_init(&req->meta_lock,NULL);
					compaction(LSM,src,des,NULL,req);
					req->end_req(req);
				}
			}
		}	
		delete_trim_process(dset);
	}
	KEYT res=ppa.front();
	ppa.pop();
	pthread_mutex_unlock(&physical_gc_lock);
	return res;
}
void freePPA(KEYT in){
	ppa.push(in);
}
