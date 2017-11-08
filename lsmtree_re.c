#include"lsmtree.h"
#include"delete_set.h"
#include"LR_inter.h"
#include"lsm_cache.h"
#include"bloomfilter.h"
#include"skiplist.h"
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


extern threadset processor;
extern MeasureTime mt;
extern uint64_t* oob;
extern delete_set *header_segment;
extern delete_set *data_segment;
extern KEYT DELETEDKEY;
pthread_mutex_t sst_lock;
pthread_mutex_t mem_lock;

KEYT write_data(lsmtree *LSM,skiplist *input,lsmtree_gc_req_t *req,double fpr){
	return skiplist_write(input,req,LSM->dfd,LSM->dfd,fpr);
}

int write_meta_only(lsmtree *LSM, skiplist *data,lsmtree_gc_req_t * input,double fpr){
	return skiplist_meta_write(data,LSM->dfd,input,fpr);
}
lsmtree *init_lsm(lsmtree *res){
	pthread_mutex_init(&sst_lock,NULL);
	pthread_mutex_init(&mem_lock,NULL);

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
			level_init(res->buf.disk[i],MUL,ISTIERING);
		else
			level_init(res->buf.disk[i],res->buf.disk[i-1]->m_num*MUL,ISTIERING);
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
bool tiering(lsmtree *LSM, level *src, level *des, Entry *ent, lsmtree_gc_req_t *req){
	if(src==NULL){ // target is ent
		skiplist *ent_data=(skiplist*)req->data;
		ent->pbn=write_data(LSM,ent_data,req,des->fpr);
		memcpy(&ent->bitset,ent_data->bitset,sizeof(ent->bitset));
#ifdef BLOOM
		ent->filter=ent_data->filter;
#endif
		level_insert(des,ent);
		skiplist_meta_free(ent_data);
		free(ent);
		return true;
	}
	//target is all src
	skiplist *temp=(skiplist*)malloc(sizeof(skiplist));
	temp=skiplist_init(temp);

	req->now_number=0;
	req->target_number=src->n_num;
	req->compt_headers=(sktable*)malloc(sizeof(sktable)*src->n_num);
	Entry **entries=(Entry**)malloc(sizeof(Entry*)*src->n_num);

	int counter=0;
	Iter *iter=level_get_Iter(src);
	Entry *temp_ent;
	while((temp_ent=level_get_next(iter))!=NULL){
		entries[counter]=temp_ent;
		skiplist_meta_read_c(temp_ent->pbn,LSM->dfd,counter++,req);
	}
	//insert result des;
	lr_gc_req_wait(req);

	for(int i=0; i<counter;i++){ // insert all data to temp
		sktable *sk=&req->compt_headers[i];
		Entry *t_ent=entries[i];
		snode *temp_s;
		for(int k=0; k<KEYN; k++){
			int bit_n=k/8;
			int off=k%8;
			if(sk->meta[k].key==NODATA) continue;
			if(t_ent->bitset[bit_n] & (1<<off)){
				temp_s=skiplist_insert(temp,sk->meta[k].key,NULL,NULL,true);
			}
			else{
				temp_s=skiplist_insert(temp,sk->meta[k].key,NULL,NULL,false);
			}
			if(temp_s)
				temp_s->ppa=sk->meta[k].ppa;
		}
		delete_ppa(header_segment,t_ent->pbn);
	}

	level_clear(src);

	skiplist *t;
	while((t=skiplist_cut(temp,KEYN > temp->size? temp->size:KEYN))){
		Entry *temp_e=level_make_entry(t->start, t->end,write_meta_only(LSM,t,req,des->fpr));
		memcpy(&temp_e->bitset,t->bitset,sizeof(ent->bitset));
#ifdef BLOOM
		temp_e->filter=t->filter;
#endif
		level_insert(des,temp_e);
		skiplist_meta_free(t);
	}

	free(req->compt_headers);
	free(entries);
	req->compt_headers=NULL;
	skiplist_free(temp);
	free(iter);
	return true;
}
bool leveling(lsmtree *LSM, level *src, level *des, Entry *ent, lsmtree_gc_req_t *req){
	KEYT start,end;
	if(src==NULL){
		start=ent->key;
		end=ent->end;
		skiplist *ent_data=(skiplist*)req->data;
		skiplist_data_write(ent_data,LSM->dfd,req);
	}
	else{
		start=src->start;
		end=src->end;
	}

	if(level_check_overlap(des,start,end)){
		int counter=src?src->n_num:0;
		counter+=des->n_num;

		skiplist *temp=(skiplist*)malloc(sizeof(skiplist));
		temp=skiplist_init(temp);

		req->now_number=0;
		req->target_number=counter;
		req->compt_headers=(sktable*)malloc(sizeof(sktable)*counter);

		Entry **entries=(Entry**)malloc(sizeof(Entry*)*counter);
		int cnt=0;
		Iter *iter=level_get_Iter(des);
		Entry *temp_ent;
		while((temp_ent=level_get_next(iter))!=NULL){
			entries[cnt]=temp_ent;
			skiplist_meta_read_c(temp_ent->pbn,LSM->dfd,cnt++,req);
		}
		free(iter);

		if(src){
			iter=level_get_Iter(des);
			while((temp_ent=level_get_next(iter))!=NULL){
				entries[cnt]=temp_ent;
				skiplist_meta_read_c(temp_ent->pbn,LSM->dfd,cnt++,req);
			}
			free(iter);
		}

		lr_gc_req_wait(req); //wait to read all data 

		for(int i=0; i<counter++; i++){
			sktable *sk=&req->compt_headers[i];
			Entry *t_ent=entries[i];
			snode *temp_s;
			for(int k=0; k<KEYN; k++){
				int bit_n=k/8;
				int off=k%8;
				if(sk->meta[k].key==NODATA) continue;
				if(t_ent->bitset[bit_n] & (1<<off)){
					temp_s=skiplist_insert(temp,sk->meta[k].key,NULL,NULL,true);
				}
				else{
					temp_s=skiplist_insert(temp,sk->meta[k].key,NULL,NULL,false);
				}
				if(temp_s)
					temp_s->ppa=sk->meta[k].ppa;
			}
			delete_ppa(header_segment,t_ent->pbn);
		}

		if(src!=NULL){
			snode *temp_s;
			skiplist *temp_skip=(skiplist*)req->data;
			snode *temp_iter=temp_skip->header->list[1];			
			while(temp_iter!=temp_skip->header){
				temp_s=skiplist_insert(temp,temp_iter->key,NULL,NULL,temp_iter->vflag);
				if(temp_s!=NULL)
					temp_s->ppa=temp_iter->ppa;
				temp_iter=temp_iter->list[1];
			}
			skiplist_meta_free(temp_skip);
		}

		level_clear(src);
		level_clear(des);


		skiplist *t;
		while((t=skiplist_cut(temp,KEYN > temp->size? temp->size:KEYN))){
			//MS(&mem);
			Entry* temp_e=level_make_entry(t->start, t->end, write_meta_only(LSM,t,req,des->fpr));
			//ME(&mem,"mem");
			memcpy(&temp_e->bitset,t->bitset,sizeof(temp_e->bitset));
#ifdef BLOOM
			temp_e->filter=t->filter;
#endif
			level_insert(des,temp_e);
			skiplist_meta_free(t);
		}
		free(req->compt_headers);
		req->compt_headers=NULL;
		free(entries);
		skiplist_free(temp);
		return true;
	}
	else{
		skiplist *ent_data=(skiplist*)req->data;
		ent->pbn=skiplist_meta_write(ent_data,LSM->dfd,req,des->fpr);
		memcpy(&ent->bitset,ent_data->bitset,sizeof(ent->bitset));
#ifdef BLOOM
		ent->filter=ent_data->filter;
#endif
		skiplist_meta_free(ent_data);

		int src_cnt=src?src->n_num:1;
		int des_cnt=des->n_num;
		Entry *src_entries;
		Entry *des_entries;
		src_entries=(Entry*)malloc(sizeof(Entry)*src_cnt);
		des_entries=(Entry*)malloc(sizeof(Entry)*des_cnt);

		int counter=0;
		Iter *iter;
		if(src){
			iter=level_get_Iter(src);
			Entry *temp_ent;
			while((temp_ent=level_get_next(iter))!=NULL){
				memcpy(&src_entries[counter++],&temp_ent,sizeof(Entry));
			}
			free(iter);
		}
		else{
			memcpy(&src_entries[counter],&ent,sizeof(Entry));
		}

		counter=0;
		iter=level_get_Iter(des);
		Entry *temp_ent;
		while((temp_ent=level_get_next(iter))!=NULL){
			memcpy(&des_entries[counter++],&temp_ent,sizeof(Entry));
		}
		free(iter);

		level_clear(src);
		level_clear(des);

		int src_idx=0;
		int des_idx=0;
		bool isSrc=false;
		bool end_check=false;
		counter=0;
		while(counter!=src_cnt+des_cnt){
			if(src_cnt==src_idx){
				isSrc=false;
				end_check=true;
				break;
			}
			if(des_cnt==des_idx){
				isSrc=true;
				end_check=true;
				break;
			}

			if(src_entries[src_idx].key<des_entries[des_idx].key)
				isSrc=true;
			else
				isSrc=false;

			if(isSrc)
				level_insert(des,&src_entries[src_idx++]);
			else
				level_insert(des,&des_entries[des_idx++]);
			counter++;
		}
		if(end_check){
			if(isSrc){
				for(src_idx; src_idx!=src_cnt; src_idx++)
					level_insert(des,&src_entries[src_idx]);
			}
			else{
				for(des_idx; des_idx!=des_cnt; des_idx++)
					level_insert(des,&des_entries[des_idx]);
			}
		}

		free(src_entries);
		free(des_entries);
	}
	return true;
}

bool compaction(lsmtree *LSM,level *src, level *des, Entry *ent,lsmtree_gc_req_t* req){
	if(des->isTiering){
		return tiering(LSM,src,des,ent,req);
	}
	else{
		return leveling(LSM,src,des,ent,req);
	}
}

int lsmtree_getProcessing_inLevel(lsmtree *LSM, KEYT key,threading *input, char *ret, lsmtree_req_t *req, int start_level){
	keyset *temp_key=NULL;
	for(int i=start_level; i<LEVELN; i++){
		req->flag=i;
		temp_key=cache_level_find(&input->master->mycache,i,key);
		if(temp_key!=NULL){
			if(temp_key->ppa==DELETEDKEY){
				printf("[%u]deleted key\n",temp_key->key);
				return 6;
			}
			skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
			return 1;
		}

		Entry **temp=level_find(LSM->buf.disk[i],key);
		if(!temp) continue;

		for(int k=0; temp[k]!=NULL; k++){
			if(SEQUENCE){
				for(int j=0; j<WAITREQN; j++){
					if(input->pre_req[j]!=NULL){
						if(temp[k]==input->entry[j]){
							pthread_mutex_lock(&input->pre_req[j]->meta_lock);
							sktable *sk=(sktable*)input->pre_req[j]->keys;
							temp_key=skiplist_keyset_find(sk,key);
							pthread_mutex_unlock(&input->pre_req[j]->meta_lock);
							if(temp_key!=NULL){
								if(temp_key->ppa==DELETEDKEY){
									printf("[%u]deleted key\n",temp_key->key);
									return 6;
								}
								skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
								free(temp);
								return 3;
							}
							else
								break;
						}
					}
				}
				for(int j=0; j<WAITREQN; j++){
					if(input->pre_req[j]==NULL){
						input->pre_req[j]=req;
						input->entry[j]=temp[k];
						pthread_mutex_lock(&req->meta_lock);
						break;
					}
				}
			}
			input->header_read++;
			skiplist_meta_read_n(temp[k]->pbn,LSM->dfd,0,req);
			return 3;
		}
		free(temp);
	}
	return 0;
}
int thread_get(lsmtree *LSM, KEYT key,threading *input, char *ret, lsmtree_req_t *req){
	//search memtree
	snode *res=skiplist_find(LSM->memtree,key);
	if(res!=NULL){
		ret=res->value;
		if(!res->vflag){
			printf("[%u]deleted key\n",res->key);
		}
		return 2;
	}
	return lsmtree_getProcessing_inLevel(LSM,key,input,ret,req,0);
}
int thread_level_get(lsmtree *LSM, KEYT key, threading *input, char *ret, lsmtree_req_t *req, int l){
	if(l>=LEVELN)
		return 0;
	keyset *temp_key=NULL;
	if(SEQUENCE){
		for(int i=0; i<WAITREQN; i++){
			if(input->pre_req[i]==req){
				input->pre_req[i]=NULL;
				input->entry[i]=NULL;
				break;
			}
		}
	}
	sktable *sk=(sktable*)req->keys;
	temp_key=skiplist_keyset_find(sk,key);
	if(temp_key!=NULL){
		if(temp_key->ppa=DELETEDKEY){
			printf("[%u]deleted key\n",temp_key->key);
			return 6;
		}
		skiplist_keyset_read(temp_key,ret,LSM->dfd,req);
		cache_input(&input->master->mycache,l,sk,req->dmatag);
		return 1;
	}
#ifdef ENABLE_LIBFTL
	memio_free_dma(2,req->dmatag);
#else
	free(req->keys);
#endif
	return lsmtree_getProcessing_inLevel(LSM,key,input,ret,req,l+1);
}

bool is_compt_needed(lsmtree *input, KEYT level){
	if(input->buf.disk[level-1]->n_num<input->buf.disk[level-1]->m_num){
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
