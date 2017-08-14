#include"bptree.h"
#include"lsmtree.h"
#include"skiplist.h"
#include"measure.h"
#include"threading.h"
#include"LR_inter.h"
#include"lsm_cache.h"
#include"delete_set.h"
#include<time.h>
#include<pthread.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<semaphore.h>
#include<sys/time.h>
#include<limits.h>
#ifdef ENABLE_LIBFTL
#include "libmemio.h"
memio_t* mio;
#endif


#ifdef THREAD
extern threadset processor;
extern delete_set *dset;
extern MeasureTime mt;
extern uint64_t* oob;
pthread_mutex_t sst_lock;
pthread_mutex_t mem_lock;

MeasureTime mem;
MeasureTime last;
MeasureTime buf;
MeasureTime bp;
MeasureTime find;

int cache_hit;
int pros_hit2;
int pros_hit;
int mem_hit;
/*
0: memtable
1~LEVELN : level
LEVELN+1=last
 */
#endif


KEYT write_data(lsmtree *LSM,skiplist *input,lsmtree_gc_req_t *req){
	return skiplist_write(input,req,LSM->dfd,LSM->dfd);
}
int write_meta_only(lsmtree *LSM, skiplist *data,lsmtree_gc_req_t * input){
	return skiplist_meta_write(data,LSM->dfd,input);
}

lsmtree* init_lsm(lsmtree *res){
	pthread_mutex_init(&sst_lock,NULL);
	pthread_mutex_init(&mem_lock,NULL);

	measure_init(&mem);
	measure_init(&last);
	measure_init(&buf);
	measure_init(&bp);
	measure_init(&find);

	res->memtree=(skiplist*)malloc(sizeof(skiplist));
	res->memtree=skiplist_init(res->memtree);
	res->buf.data=NULL;
	res->sstable=NULL;
	res->buf.lastB=NULL;
	for(int i=0;i<LEVELN; i++){
		res->buf.disk[i]=(level*)malloc(sizeof(level));
		if(i==0)
			level_init(res->buf.disk[i],MUL);
		else
			level_init(res->buf.disk[i],res->buf.disk[i-1]->m_size*MUL);
	}
#if !defined(ENABLE_LIBFTL)
	if(SEQUENCE){
		res->dfd=open("data/skiplist_data.skip",O_RDWR|O_CREAT|O_TRUNC,0666);
	}
	else{
		res->dfd=open("data/skiplist_data_r.skip",O_RDWR|O_CREAT|O_TRUNC,0666);
	}
	if(res->dfd==-1){
		printf("file open error!\n");
		return NULL;
	}
#endif
#ifdef ENABLE_LIBFTL
	if ( (mio = memio_open() ) == NULL ) {
		printf("memio_open() failed\n");
		return NULL;
	}
#endif
	return res;
}
void buffer_free(buffer *buf){
	free(buf->data);
	for(int i=0; i<LEVELN; i++){
		if(buf->disk[i]!=NULL) level_free(buf->disk[i]);
	}
	if(buf->lastB!=NULL)
		skiplist_meta_free(buf->lastB);
}

void lsm_free(lsmtree *input){
	pthread_mutex_destroy(&sst_lock);
	pthread_mutex_destroy(&mem_lock);

	skiplist_free(input->memtree);
	if(input->sstable!=NULL)
		skiplist_free(input->sstable);
	buffer_free(&input->buf);
#if !defined(ENABLE_LIBFTL)
	close(input->dfd);
#endif
#ifdef ENABLE_LIBFTL
	memio_close(mio);
#endif
	free(input);
}
void lsm_clear(lsmtree *input){
	skiplist_free(input->memtree);
	input->memtree=(skiplist*)malloc(sizeof(skiplist));
	input->memtree=skiplist_init(input->memtree);
}
bool put(lsmtree *LSM,KEYT key, char *value,lsmtree_req_t *req){
	if(key==0)
		printf("key is 0!\n");
	while(1){
		pthread_mutex_lock(&mem_lock);
		if(LSM->memtree->size<KEYN){
			if(value!=NULL)
				skiplist_insert(LSM->memtree,key,value,req,1);
			else
				skiplist_insert(LSM->memtree,key,NULL,req,0);
			pthread_mutex_unlock(&mem_lock);
			return true;
		}
		else{
			pthread_mutex_unlock(&mem_lock);
			continue;
		}
	}
	return false;
}

/*
   MeasureTime mem;
   MeasureTime last;
   MeasureTime buf;
   MeasureTime bp;
   MeasureTime find;
 */
int meta_read_data;
int thread_level_get(lsmtree *LSM,KEYT key, threading *input, char *ret, lsmtree_req_t *req, int l){
	if(l>=LEVELN)
		return 0;
	keyset *temp_key=NULL;

	for(int i=0; i<WAITREQN; i++){
		if(input->pre_req[i]==req){
			input->pre_req[i]=NULL;
			input->entry[i]=NULL;
			break;
		}
	}
	sktable *sk=(sktable*)req->keys;
	temp_key=skiplist_keyset_find(sk,key);
	if(temp_key!=NULL){
		skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
		cache_input(&input->master->mycache,l,sk,req->dmatag);
		return 1;
	}

#ifdef ENABLE_LIBFTL
	memio_free_dma(2,req->dmatag);
#else
	free(req->keys);
#endif

	bool metaflag;
	bool returnflag=0;
	for(int i=l+1; i<LEVELN; i++){
		metaflag=false;
		req->flag=i;
		
		Entry *temp=level_find(LSM->buf.disk[i],key);
		if(temp==NULL){
			continue;
		}
		
		temp_key=cache_level_find(&input->master->mycache,i,key);
		if(temp_key!=NULL){
			skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
			return 1;
		}

		for(int j=0; j<WAITREQN; j++){
			if(input->pre_req[j]!=NULL){
				if(temp==input->entry[j]){
					pthread_mutex_lock(&input->pre_req[j]->meta_lock);
					sktable *sk=(sktable*)input->pre_req[j]->keys;
					temp_key=skiplist_keyset_find(sk,key);
					pthread_mutex_unlock(&input->pre_req[j]->meta_lock);
					if(temp_key!=NULL){
						skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
						pros_hit2++;
						return 1;
					}else{
						metaflag=true;
					}
				}
			}
		}
		if(metaflag){
			continue;
		}
		for(int j=0; j<WAITREQN; j++){
			if(input->pre_req[j]==NULL){
				input->pre_req[j]=req;
				input->entry[j]=temp;
				pthread_mutex_lock(&req->meta_lock);
				break;
			}
		}
		input->header_read++;
		skiplist_meta_read_n(temp->pbn,LSM->dfd,0,req);
		return 3;
	}
	return returnflag;
}
int thread_get(lsmtree *LSM, KEYT key, threading *input, char *ret,lsmtree_req_t* req){
	req->seq_number=0;
	int tempidx=0;
	snode* res=skiplist_find(LSM->memtree,key);
	if(res!=NULL){
		mem_hit++;
		ret=res->value;
		return 2;
	}

	res=NULL;
	res=skiplist_find(LSM->buf.lastB,key);
	if(res!=NULL){
		mem_hit++;
		ret=res->value;
		return 2;
	}

	//thread_disk_get(LSM,key,req,0);

	int flag=0;
	keyset *temp_key=NULL;

	bool metaflag;
	req->type=DISK_READ_T;
	for(int i=0; i<LEVELN; i++){
		metaflag=false;
		req->flag=i;

		Entry *temp=level_find(LSM->buf.disk[i],key);
		int return_flag=3;
		if(temp==NULL){
			if(i==1)
				printf("level_find fail!\n");
			continue;
		}
		temp_key=cache_level_find(&input->master->mycache,i,key);

		if(temp_key!=NULL){
			skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
			return 1;
		}


		for(int j=0; j<WAITREQN; j++){
			if(input->pre_req[j]!=NULL){
				if(temp==input->entry[j]){
					pthread_mutex_lock(&input->pre_req[j]->meta_lock);
					return_flag=4;
					sktable *sk=(sktable *)input->pre_req[j]->keys;
					temp_key=skiplist_keyset_find(sk,key);
					pthread_mutex_unlock(&input->pre_req[j]->meta_lock);
					if(temp_key!=NULL){
						skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
						pros_hit++;
						return return_flag;
					}
					else{
						metaflag=true;
						break;
					}
				}
			}
		}
		if(metaflag){
			req->dummy=temp;
			req->seq_number+=111;
			continue;
		}
		for(int j=0; j<WAITREQN; j++){
			if(input->pre_req[j]==NULL){
				input->pre_req[j]=req;
				input->entry[j]=temp;
				pthread_mutex_lock(&req->meta_lock);
				break;
			}
		}
		input->header_read++;
		skiplist_meta_read_n(temp->pbn,LSM->dfd,0,req);
		return return_flag;
	}
	return 0;
}
lsmtree* lsm_reset(lsmtree* input){
	input->memtree=(skiplist*)malloc(sizeof(skiplist));
	input->memtree=skiplist_init(input->memtree);
	return input;
}
bool compaction(lsmtree *LSM,level *src, level *des,Entry *ent,lsmtree_gc_req_t * req){
	static int wn=0;
	Entry *target;
	KEYT s_start,s_end;
	if(src==NULL){
		target=ent;
		s_start=ent->key;
		s_end=ent->end;
	}
	else{
		target=level_get_victim(src);
		if(target->pbn>INT_MAX){
			printf("compaction_target\n");
			sleep(10);
		}
		Entry *target2=level_entry_copy(target);
		level_delete(src,target->key);
		target=target2;
	}
	skiplist *last=LSM->buf.lastB;
	if(last==NULL){
		last=(skiplist*)malloc(sizeof(skiplist));
		skiplist_init(last);
	}
	Entry **iter=level_range_find(des,src->s_start,src->s_end);

	bool check_getdata=false;
	KEYT *delete_sets=NULL;
	KEYT *delete_pbas=NULL;
	uint8_t **delete_bitset=NULL;
	int deleteIdx=0;
	int allnumber=0;
	req->compt_headers=NULL;
	if(iter!=NULL && iter[0]!=NULL){
		//printf("send read!\n");
		delete_sets=(KEYT*)malloc(sizeof(KEYT)*(des->m_size));
		delete_pbas=(KEYT*)malloc(sizeof(KEYT)*(des->m_size));
		delete_bitset=(uint8_t **)malloc(sizeof(uint8_t*)*(des->m_size));
		for(int i=0; iter[i]!=NULL; i++) allnumber++;
		allnumber++;//for target;
		check_getdata=true;
		req->now_number=0;
		req->target_number=allnumber;
		req->compt_headers=(sktable*)malloc(sizeof(sktable)*allnumber);
		int counter=0;
		for(int i=0; iter[i]!=NULL ;i++){
			Entry *temp_e=iter[i];
			delete_pbas[counter]=temp_e->pbn;
			delete_bitset[counter]=temp_e->bitset;
			delete_sets[deleteIdx++]=temp_e->key;
			if(temp_e->key > INT_MAX){
				printf("des print!!!\n");
				level_print(des);
			}
			skiplist_meta_read_c(temp_e->pbn,LSM->dfd,counter++,req);
		}
		delete_pbas[counter]=target->pbn;
		delete_bitset[counter]=target->bitset;
		skiplist_meta_read_c(target->pbn,LSM->dfd,counter++,req);
	}

	if(iter!=NULL)
		free(iter);
	if(!check_getdata){
		level_insert(des,target);
	}
	else{
		//	printf("waiting:%d!\n",wn++);
		lr_gc_req_wait(req);//wait for all read;

		//update victim's oob
		sktable *victim_sk=&req->compt_headers[allnumber-1];
		victim_sk->value=(char*)malloc(PAGESIZE * KEYN);
		req->now_number=0;
		req->target_number=KEYN;
		for(int i=0; i<KEYN; i++){
			skiplist_keyset_read_c(&victim_sk->meta[i],&victim_sk->value[PAGESIZE*i],LSM->dfd,req);
		}
		lr_gc_req_wait(req);
		skiplist_sk_data_write(victim_sk,LSM->dfd,req);
		free(victim_sk->value);

		if(src!=NULL)
			free(target);

		int getIdx=0;
		skiplist *t;
		snode *temp_s;
		for(int i=0; i<allnumber; i++){
			sktable *sk=&req->compt_headers[i];
			uint8_t *temp_bit=delete_bitset[i];
			for(int k=0; k<KEYN; k++){
				int bit_n=k/8;
				int off=k%8;
				if(temp_bit[bit_n] & (1<<off)){
					temp_s=skiplist_insert(last,sk->meta[k].key,NULL,NULL,true);
				}
				else{
					temp_s=skiplist_insert(last,sk->meta[k].key,NULL,NULL,false);
				}
				if(temp_s!=NULL)
					temp_s->ppa=sk->meta[k].ppa;
			}
			delete_ppa(dset,delete_pbas[i]);
		}

		for(int i=0; i<deleteIdx; i++){
			level_delete(des,delete_sets[i]);
		}

		while((t=skiplist_cut(last,KEYN))){
			Entry* temp_e=make_entry(t->start, t->end, write_meta_only(LSM,t,req));
			temp_e->bitset=t->bitset;
			if(temp_e->pbn>INT_MAX){
				printf("compaction wirte!\n");
				sleep(10);
			}
			level_insert(des,temp_e);
			skiplist_meta_free(t);
		}
	}
	if(delete_sets!=NULL){
		free(delete_sets);
	}
	if(delete_pbas!=NULL){
		free(delete_pbas);
	}
	if(delete_bitset!=NULL){
		free(delete_bitset);
	}
	LSM->buf.lastB=last;
	return true;
}
bool is_compt_needed(lsmtree *input, KEYT level){
	if(input->buf.disk[level-1]->size<input->buf.disk[level-1]->m_size){
		return false;
	}
	return true;
}
bool is_flush_needed(lsmtree *input){
	pthread_mutex_lock(&mem_lock);
	if(input->memtree->size<KEYN){
		pthread_mutex_unlock(&mem_lock);
		return false;
	}
	else{
		pthread_mutex_unlock(&mem_lock);
		return true;
	}
}
