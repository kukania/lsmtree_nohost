
#include"bptree.h"
#include"lsmtree.h"
#include"skiplist.h"
#include"measure.h"
#include"threading.h"
#include"LR_inter.h"
#include"lsm_cache.h"
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

extern KEYT ppa;

#ifdef THREAD
extern threadset processor;
extern MeasureTime mt;
pthread_mutex_t sst_lock;
pthread_mutex_t mem_lock;

MeasureTime mem;
MeasureTime last;
MeasureTime buf;
MeasureTime bp;
MeasureTime find;

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
			skiplist_insert(LSM->memtree,key,value,req,1);
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
int thread_disk_get(lsmtree *LSM, KEYT key, lsmtree_req_t *req, int level){
	while(1){
		if(LSM->buf.disk[level]!=NULL){
			Entry *temp=level_find(LSM->buf.disk[level],key);
			if(temp==NULL){
				level++;
				continue;
			}
			req->now_number=0;
			req->target_number=2;
			req->flag=level;
			req->res=(sktable *)malloc(sizeof(sktable));
			skiplist_meta_read_n(temp->pbn,LSM->dfd,0,req);
			skiplist_meta_read_n(temp->pbn+1,LSM->dfd,1,req);
			return 1;
		}
		else
			return -1;
	}
}
/*
MeasureTime mem;
MeasureTime last;
MeasureTime buf;
MeasureTime bp;
MeasureTime find;
*/
int meta_read_data;
int thread_get(lsmtree *LSM, KEYT key, threading *input, char *ret,lsmtree_req_t* req){
	req->seq_number=0;
	int tempidx=0;
	snode* res=skiplist_find(LSM->memtree,key);
	if(res!=NULL){
		ret=res->value;
		return 2;
	}

	res=NULL;
	res=skiplist_find(LSM->buf.lastB,key);
	if(res!=NULL){
		ret=res->value;
		return 2;
	}

	//thread_disk_get(LSM,key,req,0);
	void *meta;
	for(int i=0; i<LEVELN; i++){
		keyset *temp_key=NULL;
		if(LSM->buf.disk[i]!=NULL){
			Entry *temp=level_find(LSM->buf.disk[i],key);
			if(temp==NULL){
				continue;
			}
			temp_key=cache_level_find(&input->master->mycache,i,key);
			if(temp_key!=NULL){
				skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
				return 1;
			}
			temp_key=NULL;
				
			pthread_mutex_lock(&req->meta_lock);
			skiplist_meta_read_n(temp->pbn,LSM->dfd,0,req);
			input->header_read++;	
			pthread_mutex_lock(&req->meta_lock);
		//	while(!req->meta->dequeue(&meta)){}
		//	printf("header read!\n");

			sktable *sk=(sktable*)req->keys;
			temp_key=skiplist_keyset_find(sk,key);
			if(temp_key!=NULL){
				skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
				cache_input(&input->master->mycache,i,sk,req->dmatag);
				return 1;
			}
			else{
				memio_free_dma(2,req->dmatag);
				pthread_mutex_unlock(&req->meta_lock);
			}
		}
		else
			break;
	}
	return 0;
}

int get(lsmtree *LSM,KEYT key,char *ret){/*
											int tempidx=0;
											snode* res=skiplist_find(LSM->memtree,key);
											if(res!=NULL){
											memcpy(ret,res->value,PAGESIZE);
											return 1;
											}

											if(LSM->buf.data!=NULL){
											if(skiplist_keyset_read(skiplist_keyset_find(LSM->buf.data,key),ret,LSM->dfd)){
											return 1;	
											}
											}

											for(int i=0; i<LEVELN; i++){
											if(LSM->buf.disk[i]!=NULL){
											Entry *temp=level_find(LSM->buf.disk[i],key);
											if(temp==NULL) continue;
											sktable *readed=skiplist_meta_read(temp->pbn,LSM->dfd);
											if(skiplist_keyset_read(skiplist_keyset_find(readed,key),ret,LSM->dfd)){
											if(LSM->buf.data!=NULL){
											free(LSM->buf.data);
											}
											LSM->buf.data=readed;	
											return 1;
											}
											free(readed);
											}
											else
											break;
											}

											res=NULL;
											res=skiplist_find(LSM->buf.lastB,key);
											if(res!=NULL){
											keyset read_temp;
											read_temp.key=res->key;
											read_temp.ppa=res->ppa;
											skiplist_keyset_read(&read_temp,ret,LSM->dfd);
											return 1;
											}*/
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
	if(src==NULL){
		target=ent;
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
	Entry **iter=level_range_find(des,target->key,target->end);

	bool check_getdata=false;
	KEYT *delete_set=NULL;
	int deleteIdx=0;
	int allnumber=0;
	req->compt_headers=NULL;
	if(iter!=NULL && iter[0]!=NULL){
		//printf("send read!\n");
		delete_set=(KEYT*)malloc(sizeof(KEYT)*(des->m_size));
		for(int i=0; iter[i]!=NULL; i++) allnumber++;
		allnumber++;//for target;
		check_getdata=true;
		req->now_number=0;
		req->target_number=allnumber;
		req->compt_headers=(sktable*)malloc(sizeof(sktable)*allnumber);
		int counter=0;
		for(int i=0; iter[i]!=NULL ;i++){
			Entry *temp_e=iter[i];
			delete_set[deleteIdx++]=temp_e->key;
			if(temp_e->key > INPUTSIZE)
				printf("??");
			skiplist_meta_read_c(temp_e->pbn,LSM->dfd,counter++,req);
		}
		skiplist_meta_read_c(target->pbn,LSM->dfd,counter++,req);
	}

	if(iter!=NULL)
		free(iter);
	if(!check_getdata){
		level_insert(des,target);
	}
	else{
		printf("waiting:%d!\n",wn++);
		lr_gc_req_wait(req);//wait for all read;

		if(src!=NULL)
			free(target);

		int getIdx=0;
		skiplist *t;
		snode *temp_s;
		for(int i=0; i<allnumber; i++){
			sktable *sk=&req->compt_headers[i];
			for(int k=0; k<KEYN; k++){
				temp_s=skiplist_insert(last,sk->meta[k].key,NULL,NULL,true);
				temp_s->ppa=sk->meta[k].ppa;
			}
		}

		for(int i=0; i<deleteIdx; i++){
			level_delete(des,delete_set[i]);
		}

		while((t=skiplist_cut(last,KEYN))){
			Entry* temp_e=make_entry(t->start, t->end, write_meta_only(LSM,t,req));
			if(temp_e->pbn>INT_MAX){
				printf("compaction wirte!\n");
				sleep(10);
			}
			level_insert(des,temp_e);
			skiplist_meta_free(t);
		}
	}
	if(delete_set!=NULL){
		free(delete_set);
	}
	LSM->buf.lastB=last;
	return true;
}
bool merge(lsmtree *LSM,KEYT target,skiplist *list){/*
													   pthread_mutex_lock(&merge_lock);
													   KEYT t=target--;
													   Entry *target_entry=make_entry(list->start,list->end,write_data(LSM,list));

													   while(1){
													   if(t==0){
													   if(LSM->buf.disk[0]==NULL){
													   LSM->buf.disk[0]=(level*)malloc(sizeof(level));
													   level_init(LSM->buf.disk[0],MUL);
													   LSM->buf.disk[0]->number=1;
													   }
													   if(LSM->buf.disk[0]->size<LSM->buf.disk[0]->m_size){
													   compaction(LSM,NULL,LSM->buf.disk[0],target_entry);
													   }
													   else{
													   t++; continue;
													   }
													   pthread_mutex_unlock(&merge_lock);
													   return true;
													   }
													   else{
													   if(LSM->buf.disk[t]==NULL){
													   LSM->buf.disk[t]=(level*)malloc(sizeof(level));
													   level_init(LSM->buf.disk[t],LSM->buf.disk[t-1]->size*MUL);
													   LSM->buf.disk[t]->number=t+1;
													   }
													   if(LSM->buf.disk[t]->size<LSM->buf.disk[t]->m_size){
													   compaction(LSM,LSM->buf.disk[t-1],LSM->buf.disk[t],NULL);
													   t--;
													   }
													   else{
													   t++;
													   }
													   if(target==t)
													   break;
													   continue;
													   }
													   }
													   pthread_mutex_unlock(&merge_lock);*/
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
