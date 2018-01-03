#include"bptree.h"
#include"lsmtree.h"
#include"skiplist.h"
#include"measure.h"
#include"threading.h"
#include"LR_inter.h"
#include"delete_set.h"
#include"bloomfilter.h"
#include<time.h>
#include<pthread.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<semaphore.h>
#include<sys/time.h>
#include<limits.h>
#include<math.h>
#ifdef ENABLE_LIBFTL
#include "libmemio.h"
memio_t* mio;
#endif

#ifdef CACHE
#include"cache.h"
extern cache *CH;
#endif

#ifdef THREAD
extern threadset processor;
extern MeasureTime mt;
extern uint64_t* oob;
extern delete_set *header_segment;
extern delete_set *data_segment;
extern KEYT DELETEDKEY;
extern pthread_mutex_t gc_read_lock;
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
int header_read;

extern int write_make_check;
extern int write_wait_check;
/*
0: memtable
1~LEVELN : level
LEVELN+1=last
*/
#endif

void lsmtree_save(lsmtree *LSM, char *lo){
	int tfd=open(lo,O_RDWR|O_CREAT|O_TRUNC,0666);
	if(tfd<=0){
		printf("file open error!\n");
	}
	int level_cnt=0;
	for(int i=0; i<LEVELN; i++){
		level *lev=LSM->buf.disk[i];
		if(lev->size!=0){
			level_cnt++;
		}
	}
	if(!write(tfd,&level_cnt,sizeof(level_cnt))){
		printf("not work\n");
	}
	for(int i=0; i<level_cnt; i++){
		level *lev=LSM->buf.disk[i];
		if(lev->size!=0){
			level_save(lev,tfd);
		}
	}
	skiplist_save(LSM->memtree,tfd);
}

void lsmtree_load(lsmtree *LSM, char *lo){
	int tfd=open(lo,O_RDONLY,0666);
	if(tfd<=0){
		printf("file open error!\n");
	}
	int level_cnt;
	if(!read(tfd,&level_cnt,sizeof(level_cnt))){
		printf("not work\n");
	}
	for(int i=0; i<level_cnt; i++){
		level *lev=LSM->buf.disk[i];
		level_load(lev,tfd);
	}
	//skiplist_load(LSM->memtree,tfd);
}

KEYT write_data(lsmtree *LSM,skiplist *input,lsmtree_gc_req_t *req,double fpr){
	return skiplist_write(input,req,LSM->dfd,LSM->dfd,fpr);
}
int write_meta_only(lsmtree *LSM, skiplist *data,lsmtree_gc_req_t * input,double fpr){
	return skiplist_meta_write(data,LSM->dfd,input,fpr);
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

	//setting fprs
	double ffpr=RAF*(1-MUL)/(1-pow(MUL,LEVELN+0));
	uint64_t bits=0;
	//
	for(int i=0;i<LEVELN; i++){
		res->buf.disk[i]=(level*)malloc(sizeof(level));
		if(i==0)
			level_init(res->buf.disk[i],MUL);
		else
			level_init(res->buf.disk[i],res->buf.disk[i-1]->m_size*MUL);
		double target_fpr=pow(MUL,i)*ffpr;
		if(target_fpr>1)
			res->buf.disk[i]=0;
		else{
#ifdef MONKEY_BLOOM
			res->buf.disk[i]->fpr=target_fpr;
#else
			res->buf.disk[i]->fpr=FPR;//pow(MUL,i)*ffpr;
#endif
			printf("[%d] : fpr > %f\n",i,res->buf.disk[i]->fpr);
			bits+=bf_bits(pow(MUL,i)*KEYN,FPR);
		}
		//	printf("[%d]%lf\n",i+1,target_fpr);
	}
	//printf("bits : %lu\n",bits);
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
#ifdef CACHE
		Entry *entry=req->req_entry;
		if(!entry->data){
			entry->data=(sktable*)malloc(sizeof(sktable));
			memcpy(entry->data->meta,sk,PAGESIZE);
			entry->c_entry=cache_insert(CH,entry,req->dmatag);
		}
#endif
		if(temp_key->ppa==DELETEDKEY){
			printf("[%u]deleted\n",temp_key->key);
	#ifdef ENABLE_LIBFTL
			memio_free_dma(2,req->dmatag);
	#else
			free(sk);
	#endif
			return 6;
		}
		skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
	#ifdef ENABLE_LIBFTL
			memio_free_dma(2,req->dmatag);
	#else
			free(sk);
	#endif
		return 1;
	}
#ifdef ENABLE_LIBFTL
	memio_free_dma(2,req->dmatag);
#else
	free(req->keys);
#endif

	bool metaflag;
	bool returnflag=0;
	int level=0;
	for(int i=l+1; i<LEVELN; i++){
		metaflag=false;
		req->flag=i;
		pthread_mutex_lock(&LSM->buf.disk[i]->level_lock);
		Entry *temp=level_find(LSM->buf.disk[i],key);
		level=i;
		if(temp==NULL){
			pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
			continue;
		}
#ifdef CACHE
		if(temp->data){
			temp_key=skiplist_keyset_find(temp->data,key);
			if(temp_key!=NULL){
				input->cache_hit++;
				//cache_update(CH,temp);
				if(temp_key->ppa==DELETEDKEY){
					printf("[%u]deleted\n",temp_key->key);
					pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
					return 6;
				}
				skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
				pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
				return 1;
			}
			pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
			continue;
		}
#endif
		if(temp==NULL){
			pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
			continue;
		}
#ifdef BLOOM
		else if(!bf_check(temp->filter,key)){
			pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
			continue;
		}
#endif
		if(metaflag){
			pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
			continue;
		}

		input->header_read++;
		req->req_entry=temp;
		skiplist_meta_read_n(temp->pbn,LSM->dfd,0,req);
		pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
		return 3;
	}/*
		printf("%d-----------------------------\n",key);
		sktable_print(sk);
		printf("------------------------------\n");*/
	pthread_mutex_unlock(&LSM->buf.disk[level]->level_lock);
	return 0;
}
int thread_get(lsmtree *LSM, KEYT key, threading *input, char *ret,lsmtree_req_t* req){
	req->seq_number=0;
	int tempidx=0;
	snode* res=skiplist_find(LSM->memtree,key);
	if(res!=NULL){
		mem_hit++;
		ret=res->value;
		if(!res->vflag){
			printf("[%u]deleted\n",res->key);
		}
		return 2;
	}
	int flag=0;
	keyset *temp_key=NULL;
	static int __ppa=0;
	bool metaflag;
	req->type=DISK_READ_T;
	int level=0;
	for(int i=0; i<LEVELN; i++){
		metaflag=false;
		req->flag=i;
		pthread_mutex_lock(&LSM->buf.disk[i]->level_lock);
		Entry *temp=level_find(LSM->buf.disk[i],key);
		level=i;
		int return_flag=3;
		if(temp==NULL){
			pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
			continue;
		}
#ifdef CACHE
		if(temp->data){
			temp_key=skiplist_keyset_find(temp->data,key);
			if(temp_key!=NULL){
				input->cache_hit++;
				//cache_update(CH,temp);
				if(temp_key->ppa==DELETEDKEY){
					printf("[%u]deleted\n",temp_key->key);
					pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
					return 6;
				}
				skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
				pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
				return 1;
			}
			pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
			continue;
		}
#endif
		
#ifdef BLOOM
		else if(!bf_check(temp->filter,key)){
			pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
			continue;
		}
#endif
		if(SEQUENCE){
			for(int j=0; j<WAITREQN; j++){
				if(input->pre_req[j]!=NULL){
					if(temp==input->entry[j]){
						return_flag=4;
						pthread_mutex_lock(&input->pre_req[j]->meta_lock);
						sktable *sk=(sktable *)input->pre_req[j]->keys;
						temp_key=skiplist_keyset_find(sk,key);
						pthread_mutex_unlock(&input->pre_req[j]->meta_lock);
						if(temp_key!=NULL){
							if(temp_key->ppa==DELETEDKEY){
								printf("[%u]deleted\n",temp_key->key);
								pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
								return 6;
							}
							skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
							pros_hit++;
							pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
							return return_flag;
						}
						else{
							metaflag=true;
							break;
						}
					}
				}
			}
		}
		if(SEQUENCE){
			for(int j=0; j<WAITREQN; j++){
				if(input->pre_req[j]==NULL){
					input->pre_req[j]=req;
					input->entry[j]=temp;
					//				input->pre_req[j]->lock_test=1;
					pthread_mutex_lock(&req->meta_lock);
					break;
				}
			}
		}
		input->header_read++;	
		req->req_entry=temp;
		skiplist_meta_read_n(temp->pbn,LSM->dfd,0,req);
		pthread_mutex_unlock(&LSM->buf.disk[i]->level_lock);
		return return_flag;
	}
	pthread_mutex_unlock(&LSM->buf.disk[level]->level_lock);
	return 0;
}
lsmtree* lsm_reset(lsmtree* input){
	input->memtree=(skiplist*)malloc(sizeof(skiplist));
	input->memtree=skiplist_init(input->memtree);
	return input;
}
level *target_des;
int compaction__;
int allnumber;
bool compaction(lsmtree *LSM, level *src, level *des, Entry *ent, lsmtree_gc_req_t *req){
	KEYT s_start,s_end;
	int src_cnt=0;
	int noh=0;
	Entry **src_list=NULL;
	Entry **src_origin_list=NULL;
	if(src==NULL){
		skiplist *temp_skip=(skiplist*)req->data;
		skiplist_data_write(temp_skip,LSM->dfd,req);
#ifdef CACHE
		ent->data=skiplist_to_sk_extra(temp_skip,des->fpr,&ent->bitset,&ent->filter);
#endif
		src_list=(Entry**)malloc(sizeof(Entry*)*(1+1));
		src_list[0]=ent;
		src_list[1]=NULL;
		s_start=ent->key;
		s_end=ent->end;
	}
	else{
		src_list=(Entry**)malloc(sizeof(Entry*)*(src->size+1));
		src_origin_list=(Entry**)malloc(sizeof(Entry*)*(src->size+1));
		Iter *level_iter=level_get_Iter(src);
		Entry *iter_temp;
		while((iter_temp=level_get_next(level_iter))!=NULL){
			src_origin_list[src_cnt]=iter_temp;
			src_list[src_cnt]=level_entry_copy(iter_temp,false);
			src_cnt++;
		}
		free(level_iter);
		src_list[src_cnt]=NULL;
		src_origin_list[src_cnt]=NULL;
		s_start=src->start;
		s_end=src->end;
	}

	Entry **iter=level_range_find(des,s_start,s_end);
	if(iter==NULL || iter[0]==NULL){ //no overlap
		target_des=level_copy(des);
		if(src==NULL){
			skiplist *temp_skip=(skiplist*)req->data;
			ent->pbn=skiplist_meta_write(temp_skip,LSM->dfd,req,des->fpr);
#ifdef CACHE
			free(temp_skip->bitset);
	#ifdef BLOOM
			bf_free(temp_skip->filter);
	#endif
			ent->c_entry=cache_insert(CH,ent,0);
#else
	#ifdef BLOOM
			ent->filter=temp_skip->filter;
	#endif
			ent->bitset=temp_skip->bitset;
#endif
			skiplist_free(temp_skip);
			level_insert(target_des,ent);
		}
		else{
#ifndef MONKEY_BLOOM
			for(int i=0; src_list[i]!=NULL; i++){
				level_insert(target_des,src_list[i]);
			}//normal no overlap end
#else
			req->now_number=0;
			req->target_number=0;
			req->compt_headers=(sktable*)malloc(sizeof(sktable)*src_cnt);
			for(int i=0; src_list[i]!=NULL; i++){
	#ifdef CACHE
				if(src_list[i]->data==NULL){
					skiplist_meta_read_c(src_list[i]->pbn,LSM->dfd,i,req);
					req->target_number++;;
				}
	#else
				skiplist_meta_read_c(src_list[i]->pbn,LSM->dfd,i,req);
				req->target_number++;
	#endif
			}
			lr_gc_req_wait(req);
			sktable *temp_sk;
			for(int i=0; src_list[i]!=NULL; i++){
				if(src_list[i]->data!=NULL)
					temp_sk=src_list[i]->data;
				else
					temp_sk=&req->compt_headers[i];
				
				bf_free(src_list[i]->filter);
				src_list[i]->filter=bf_init(KEYN,des->fpr);
				for(int j=0; j<KEYN; j++){
					bf_set(src_list[i]->filter,temp_sk->meta[j].key);
				}
				level_insert(target_des,src_list[i]);
			}
			free(req->compt_headers);
#endif
		}
		free(iter);
	}
	else{
		free(iter);
		target_des=(level*)malloc(sizeof(level));
		target_des=level_init(target_des,des->m_size);
		target_des->fpr=des->fpr;

		skiplist *last=NULL;
		Entry *last_one=NULL;
		Entry *temp_last_one=NULL;
		//overlap
		for(int i=0; src_list[i]!=NULL; i++){
			Entry *target=src_list[i];
			if(src!=NULL)
				src_origin_list[i]->iscompactioning=true;
			iter=level_range_find(des,target->key,target->end);
			if(iter==NULL || iter[0]==NULL){//not overlap with in target;
				level_insert(target_des,target);
			}
			else{
				//copmaction skiplist swap
				if(last==NULL){
					last=(skiplist*)malloc(sizeof(skiplist));
					last=skiplist_init(last);
				}/*
				else{
					skiplist *temp_last=(skiplist*)malloc(sizeof(skiplist));
					temp_last=skiplist_init(temp_last);
					snode *temp=last->header->list[1];
					snode *temp_s;
					while(temp!=last->header){
						temp_s=skiplist_insert(temp_last,temp->key,NULL,NULL,temp->vflag);
						if(temp_s)
							temp_s->ppa=temp->ppa;
						temp=temp->list[1];
					}
					skiplist_free(last);
					last=temp_last;
				}*/

				int desnumber=0;
				req->now_number=0;
				req->target_number=0;
				for(int j=0; iter[j]!=NULL; j++) desnumber++; //des 
				req->compt_headers=(sktable*)malloc(sizeof(sktable)*(desnumber+1));
				for(int j=0; iter[j]!=NULL; j++){
					Entry *temp_e=iter[j];
					temp_e->iscompactioning=true;
					if(last_one!=NULL){
						if(last_one==iter[j])
							continue;
					}
				#ifdef CACHE
					if(temp_e->data==NULL){
						skiplist_meta_read_c(temp_e->pbn,LSM->dfd,j,req);
						req->target_number++;		
					}
				#else
					skiplist_meta_read_c(temp_e->pbn,LSM->dfd,j,req);
					req->target_number++;
				#endif
					noh++;
				}
				if(src!=NULL){
					skiplist_meta_read_c(src_origin_list[i]->pbn,LSM->dfd,desnumber,req);
					req->target_number++;
					noh++;
				}
				KEYT limit=iter[desnumber-1]->key;
				temp_last_one=iter[desnumber-1];
				lr_gc_req_wait(req);
				sktable *sk;
				uint8_t *temp_bit;
				snode *temp_s;
				//des insert into skiplist
				for(int j=0; j<desnumber; j++){
					if(last_one!=NULL){
						if(last_one==iter[j])
							continue;
					}
#ifdef CACHE
					if(iter[j]->data!=NULL){
						sk=iter[j]->data;
					}
					else{
#endif
						sk=&req->compt_headers[j];
#ifdef CACHE
					}
#endif
					temp_bit=iter[j]->bitset;
					for(int k=0; k<KEYN; k++){
						int bit_n=k/8;
						int off=k%8;
						if(sk->meta[k].key==NODATA) continue;
						if(temp_bit[bit_n] & (1<<off)){
							temp_s=skiplist_insert(last,sk->meta[k].key,NULL,NULL,true);
						}
						else{
							temp_s=skiplist_insert(last,sk->meta[k].key,NULL,NULL,false);
						}
						if(temp_s!=NULL){
							temp_s->ppa=sk->meta[k].ppa;
						}
					}
					delete_ppa(header_segment,iter[j]->pbn);
				}

				//src insert into skiplist
				if(src==NULL){
#ifdef CACHE
					skiplist *temp_skip=(skiplist*)req->data;
					skiplist_free(temp_skip);
					temp_bit=ent->bitset;
					sk=ent->data;
					for(int k=0; k<KEYN; k++){
						int bit_n=k/8;
						int off=k%8;
						if(sk->meta[k].key==NODATA) continue;
						if(temp_bit[bit_n] & (1<<off)){
							temp_s=skiplist_insert(last,sk->meta[k].key,NULL,NULL,true);
						}
						else{
							temp_s=skiplist_insert(last,sk->meta[k].key,NULL,NULL,false);
						}
						if(temp_s!=NULL){
							temp_s->ppa=sk->meta[k].ppa;
						}
					}
#else
					skiplist *temp_skip=(skiplist*)req->data;
					snode *temp_s1=temp_skip->header->list[1];
					snode *temp_s2;
					while(temp_s1!=temp_skip->header){
						temp_s2=skiplist_insert(last, temp_s1->key,NULL,NULL,temp_s1->vflag);
						if(temp_s2){
							if(temp_s1->ppa==127){
								printf("here in compaction\n");
							}
							temp_s2->ppa=temp_s1->ppa;
						}
						temp_s1=temp_s1->list[1];
					}
					skiplist_free(temp_skip);
#endif
				}
				else{
					sk=&req->compt_headers[desnumber];
					temp_bit=target->bitset;
					for(int k=0; k<KEYN; k++){
						int bit_n=k/8;
						int off=k%8;
						if(sk->meta[k].key==NODATA) continue;
						if(temp_bit[bit_n] & (1<<off)){
							temp_s=skiplist_insert(last,sk->meta[k].key,NULL,NULL,true);
						}
						else{
							temp_s=skiplist_insert(last,sk->meta[k].key,NULL,NULL,false);
						}
						if(temp_s!=NULL)
							temp_s->ppa=sk->meta[k].ppa;
					}
					delete_ppa(header_segment,src_origin_list[i]->pbn);
				}
				skiplist *t;
				if(src_list[i+1]==NULL){
					while((t=skiplist_cut(last,KEYN> last->size? last->size:KEYN,UINT_MAX))){
						Entry *temp_e=make_entry(t->start,t->end,write_meta_only(LSM,t,req,des->fpr));
						temp_e->bitset=t->bitset;
#ifdef CACHE
						temp_e->data=skiplist_to_sk(t);
						temp_e->c_entry=cache_insert(CH,temp_e,0);
#endif
					
#ifdef BLOOM
						temp_e->filter=t->filter;
#endif	
						level_insert(target_des,temp_e);
						skiplist_free(t);
					}
					skiplist_free(last);
				}
				else{
					while((t=skiplist_cut(last,KEYN,limit))){
						Entry *temp_e=make_entry(t->start,t->end,write_meta_only(LSM,t,req,des->fpr));
						temp_e->bitset=t->bitset;
#ifdef CACHE
						temp_e->data=skiplist_to_sk(t);
						temp_e->c_entry=cache_insert(CH,temp_e,0);
#endif
					
#ifdef BLOOM
						temp_e->filter=t->filter;
#endif	
						level_insert(target_des,temp_e);
						skiplist_free(t);
					}
				}
				free(req->compt_headers);
				free_entry(target);
			}
			last_one=temp_last_one;
			free(iter);
		}
		//not compacted node's are copied, and inserted into target_des
		Iter *des_iter=level_get_Iter(des);
		Entry *des_temp,*copied_entry;
		while((des_temp=level_get_next(des_iter))!=NULL){
			if(!des_temp->iscompactioning){
				copied_entry=level_entry_copy(des_temp,false);
				level_insert(target_des,copied_entry);
			}
		}
		free(des_iter);
	}
	allnumber+=noh;
	level **temp_pp=NULL;
	level *temp_src;
	if(src!=NULL){//src level swap
		int m_size=src->m_size;
		for(int i=0; i<LEVELN; i++){
			if(LSM->buf.disk[i]->m_size==m_size){
				temp_pp=&LSM->buf.disk[i];
				break;
			}
		}
		float fpr=src->fpr;
		
		temp_src=(level*)malloc(sizeof(level));
		temp_src=level_init(temp_src,m_size);
		pthread_mutex_lock(&src->level_lock);
		(*temp_pp)=temp_src;
		pthread_mutex_unlock(&src->level_lock);
		temp_src->fpr=src->fpr;
		level_free(src);
	}


	//des level swap
	for(int i=0; i<LEVELN; i++){
		if(LSM->buf.disk[i]->m_size==target_des->m_size){
			temp_pp=&LSM->buf.disk[i];
			break;
		}
	}
	pthread_mutex_lock(&des->level_lock);
	(*temp_pp)=target_des;
	pthread_mutex_unlock(&des->level_lock);
	level_free(des);
	free(src_list);
	free(src_origin_list);
	target_des=NULL;
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
