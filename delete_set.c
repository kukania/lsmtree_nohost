#include<string.h>
#include"delete_set.h"
#include"bptree.h"
#include"lsmtree.h"
#include"LR_inter.h"
#include"ppa.h"
extern lsmtree *LSM;
#ifdef ENABLE_LIBFTL
#include"libmemio.h"
extern memio *mio;
#endif
extern lsmtree* LSM;
extern std::queue<KEYT>ppa;
uint64_t *oob;
lsmtree_req_t * delete_make_req(bool iswrite){
	lsmtree_req_t *req=(lsmtree_req_t *)malloc(sizeof(lsmtree_req_t));
	req->req=NULL;
	if(iswrite){
		req->type=LR_DELETE_PW;
	}
	else{
		req->type=LR_DELETE_PR;
		pthread_mutex_init(&req->meta_lock,NULL);
		pthread_mutex_lock(&req->meta_lock);
	}
	req->end_req=lr_end_req;
	return req;
}
void delete_init(delete_set *set){
	//for test
	oob=(uint64_t *)malloc(MAXPAGE*sizeof(uint64_t));
	memset(oob,0,MAXPAGE*sizeof(uint64_t));
	for(int i=0; i<SEGNUM; i++){
		for(int j=0; j<PAGENUM/8; j++){
			set->blocks[i].bitset[j]=0xFF;
		}
		set->blocks[i].invalid_n=0;
		set->blocks[i].number=i;
		set->blocks[i].erased=true;
	}
	set->cache_table=NULL;
	set->cache_entry=NULL;
	set->cache_level=0;;
}
void delete_ppa(delete_set *set,KEYT input){
	int block_num=input/PAGENUM;
	int offset_meta=input%PAGENUM;

	int bit_num=offset_meta/8;
	int offset=offset_meta%8;

	uint8_t target=1;
	target=target<<offset;
	target=~target;
	set->blocks[block_num].bitset[bit_num] &= target;
	set->blocks[block_num].invalid_n++;
	if(set->blocks[block_num].invalid_n > PAGENUM){
		printf("over block!\n");	
	}
	printf("%u\n",input);
}

int delete_get_victim(delete_set *set){
	int invalids=0;
	int block_num=0;
	for(int i=0; i<SEGNUM; i++){
		if(invalids<set->blocks[i].invalid_n){
			invalids=set->blocks[i].invalid_n;
			block_num=i;
		}
	}
	if(invalids==0)
		return -1;
	return block_num;
}
void delete_change_cache(delete_set *set, Entry *entry, sktable *data,int level){
	if(set->cache_table==NULL){
		set->cache_table=data;
		set->cache_entry=entry;
		set->cache_level=level;
		return;
	}
	sktable* old=set->cache_table;
	lsmtree_req_t *req=delete_make_req(1);
	char *temp_p;
#ifdef ENABLE_LIBFTL
	req->dmatag=memio_alloc_dma(1,&temp_p);
#else
	temp_p=(char*)malloc(PAGESIZE);
#endif
	req->data=temp_p;
	memcpy(temp_p,old,PAGESIZE); //sk is value;
	free(old);

	KEYT new_ppa=getPPA((void*)req);
#ifdef ENABLE_LIBFTL
	memio_write(mio,new_ppa,(uint64_t)PAGESIZE,(uint8_t *)req->data,1,req,req->dmatag);
#else
	lseek64(LSM->dfd,((off64_t)PAGESIZE)*new_ppa,SEEK_SET);
	write(LSM->dfd,temp_p,PAGESIZE);
	req->end_req(req);
#endif
	uint64_t new_oob=0;
	KEYSET(new_oob,set->cache_entry->key);
	LEVELSET(new_oob,set->cache_level+1);
	FLAGSET(new_oob,0);
	oob[new_ppa]=new_oob;

	if(!set->cache_entry->gc_cache){
		delete_ppa(set,set->cache_entry->pbn);
	}
	set->cache_entry->gc_cache=false;
	set->cache_entry->pbn=new_ppa;
	set->cache_table=data;
	set->cache_entry=entry;
	set->cache_level=level;
	return;
}
int delete_trim_process(delete_set *set){
	int block_num=delete_get_victim(set);
	if(block_num==-1){
		return 0;
	}
	int invalids=set->blocks[block_num].invalid_n;

	if(invalids==PAGENUM){ 
		for(int j=0; j<PAGENUM/8; j++){
			set->blocks[block_num].bitset[j]=0xFF;
		}
		set->blocks[block_num].invalid_n=0;
	}
	else{
		KEYT temp_p_key;
		bool stop_flag=false;
		printf("delete target block : %d, invalid_n: %d\n",block_num,invalids);
		for(int i=0 ; i<PAGENUM/8; i++){
			for(int j=0; j<8; j++){
				uint8_t test=1<<j;
				temp_p_key=block_num*PAGENUM+i*8+j;
				if((set->blocks[block_num].bitset[i]&test)){ //1 valid,0 invalid
					lsmtree_req_t *req=delete_make_req(0);
					sktable *sk=(sktable*)malloc(sizeof(sktable));
					req->params[2]=(void*)sk;
					char *temp_p;
#ifdef ENABLE_LIBFTL
					req->dmatag=memio_alloc_dma(2,&temp_p);
#else
					temp_p=(char*)malloc(PAGESIZE);
#endif
					req->data=temp_p;
					pthread_mutex_t *temp_mutex=&req->meta_lock;
#ifdef ENABLE_LIBFTL
					memio_read(mio,temp_p_key,(uint64_t)(PAGESIZE),(uint8_t *)req->data,1,req,req->dmatag);
#else
					lseek64(LSM->dfd,((off64_t)PAGESIZE)*temp_p_key,SEEK_SET);
					read(LSM->dfd,temp_p,PAGESIZE);
					req->end_req(req);
#endif
					//read header data;
					pthread_mutex_lock(temp_mutex);
					pthread_mutex_destroy(temp_mutex);
					uint64_t temp_oob=oob[temp_p_key];
					if(temp_oob==0){
						stop_flag=true;
						free(sk);
						break;
					}
					int level=LEVELGET(temp_oob)-1;//level-1
					KEYT key=KEYGET(temp_oob);
					Entry *header=level_find(LSM->buf.disk[level],key);
					
					if(FLAGGET(temp_oob)){//1 data, 0 header
						if(set->cache_entry==NULL || header!=set->cache_entry){
							sktable *data_sk=(sktable *)malloc(sizeof(sktable));
							/*page read*/
							req=delete_make_req(0);//read
							req->params[2]=(void*)data_sk;
#ifdef ENABLE_LIBFTL
							req->dmatag=memio_alloc_dma(2,&temp_p);
#else
							temp_p=(char*)malloc(PAGESIZE);
#endif
							req->data=temp_p;
							req->params[2]=(void*)data_sk;
							temp_mutex=&req->meta_lock;
#ifdef ENABLE_LIBFTL
							memio_read(mio,header->pbn,(uint64_t)PAGESIZE,(uint8_t *)req->data,1,req,req->dmatag);
#else
							//read
							lseek64(LSM->dfd,((off64_t)PAGESIZE)*header->pbn,SEEK_SET);
							read(LSM->dfd,temp_p,PAGESIZE);
							req->end_req(req);
#endif
							pthread_mutex_lock(temp_mutex);
							pthread_mutex_destroy(temp_mutex); //header read complete
							delete_change_cache(set,header,data_sk,level);
						}
						uint64_t new_oob=0;
						req=delete_make_req(1);//write
						KEYT new_ppa=getPPA((void*)req);//for data;
						KEYSET(new_oob,key);
						LEVELSET(new_oob,level+1);
						FLAGSET(new_oob,1);
#ifdef ENABLE_LIBFTL
						req->dmatag=memio_alloc_dma(1,&temp_p);
#else
						temp_p=(char*)malloc(PAGESIZE);
#endif
						req->data=temp_p;
						memcpy(temp_p,sk,PAGESIZE); //sk is value;
#ifdef ENABLE_LIBFTL
						memio_write(mio,new_ppa,(uint64_t)PAGESIZE,(uint8_t *)req->data,1,req,req->dmatag);
#else
						lseek64(LSM->dfd,((off64_t)PAGESIZE)*new_ppa,SEEK_SET);
						write(LSM->dfd,temp_p,PAGESIZE);
						req->end_req(req);
#endif
						delete_ppa(set,temp_p_key);
						oob[new_ppa]=new_oob;
						keyset *target=skiplist_keyset_find(set->cache_table,key);
						target->ppa=new_ppa;
						continue;
					}
					else{
						if(header==set->cache_entry){
							free(sk);
							header->gc_cache=true;
							continue;
						}
					}

					//header write
					req=delete_make_req(1);//write
					KEYT new_pba=getPPA((void*)req); 
					uint64_t new_oob=0;
					KEYSET(new_oob,header->key);
					LEVELSET(new_oob,level+1);
					FLAGSET(new_oob,0);
#ifdef ENABLE_LIBFTL
					req->dmatag=memio_alloc_dma(1,&temp_p);
#else
					temp_p=(char*)malloc(PAGESIZE);
#endif
					req->data=temp_p;
					memcpy(temp_p,sk,PAGESIZE);
#ifdef ENABLE_LIBFTL
					memio_write(mio,new_pba,(uint64_t)PAGESIZE,(uint8_t*)req->data,1,req,req->dmatag);
#else
					lseek64(LSM->dfd,((off64_t)PAGESIZE)*new_pba,SEEK_SET);
					write(LSM->dfd,temp_p,PAGESIZE);
					req->end_req(req);				
#endif
					KEYT temp_pba=header->pbn;
					header->pbn=new_pba;
					oob[header->pbn]=new_oob;
					delete_ppa(set,temp_pba);
					free(sk);
				}
				else
					continue;
			}
			if(stop_flag)
				break;
		}
		for(int j=0; j<PAGENUM/8; j++){
			set->blocks[block_num].bitset[j]=0xFF;
		}
		set->blocks[block_num].invalid_n=0;
	}
	//send trim operation;
#ifdef ENABLE_LIBFTL
	memio_trim(mio,block_num*PAGENUM,PAGENUM);
#endif
	for(int i=block_num*PAGENUM; i<PAGENUM*block_num+PAGENUM;i++){
		oob[i]=0;
		freePPA(i);
	}
	printf("end of delete :%u\n",ppa.front());
	return 1;
}

void delete_free(delete_set *set){
	free(set);
}
