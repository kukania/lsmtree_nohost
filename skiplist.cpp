#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<limits.h>
#include<time.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/types.h>
#include<pthread.h>

#include"utils.h"
#include"skiplist.h"
#include"LR_inter.h"
#include"threading.h"
#include"delete_set.h"
#include"ppa.h"
#ifdef ENABLE_LIBFTL
#include "libmemio.h"
extern memio_t* mio;
#endif
extern threadset processor;
extern MeasureTime mt;
extern pthread_mutex_t pl;
extern pthread_mutex_t endR;
extern delete_set *header_segment;
extern delete_set *data_segment;
extern KEYT DELETEDKEY;
extern uint64_t *oob;
#ifdef THREAD
pthread_mutex_t dfd_lock;
#endif
snode *snode_init(snode *node){
	node->value=(char*)malloc(PAGESIZE);
	for(int i=0; i<MAX_L; i++)
		node->list[i]=NULL;
	node->vflag=true;
	node->req=NULL;
	return node;
}

skiplist *skiplist_init(skiplist *point){
	point->level=1;
	point->start=-1;
	point->end=point->size=0;
	point->header=(snode*)malloc(sizeof(snode));
	point->header->list=(snode**)malloc(sizeof(snode)*(MAX_L+1));
	for(int i=0; i<MAX_L; i++) point->header->list[i]=point->header;
	point->header->key=-1;
	point->bitset=NULL;
	return point;
}
snode *skiplist_pop(skiplist *list){
	if(list->size==0) return NULL;
	KEYT key=list->header->list[1]->key;
	int i;
	snode *update[MAX_L+1];
	snode *x=list->header;
	for(i=list->level; i>=1; i--){
		update[i]=list->header;
	}
	x=x->list[1];
	if(x->key ==key){
		for(i =1; i<=list->level; i++){
			if(update[i]->list[i]!=x)
				break;
			update[i]->list[i]=x->list[i];
		}

		while(list->level>1 && list->header->list[list->level]==list->header){
			list->level--;
		}
		list->size--;
		return x;
	}
	return NULL;	
}
snode *skiplist_find(skiplist *list, KEYT key){
	if(list==NULL) return NULL;
	if(list->size==0) return NULL;
	snode *x=list->header;
	for(int i=list->level; i>=1; i--){
		while(x->list[i]->key<key)
			x=x->list[i];
	}
	if(x->list[1]->key==key)
		return x->list[1];
	return NULL;
}
static snode * skiplist_find_level(KEYT key, int level, skiplist *list){
	snode *temp=list->header;
	int _level=level;
	while(temp->list[_level]->key<key){
		temp=temp->list[_level];
	}
	return temp;
}
static int getLevel(){
	int level=1;
	int temp=rand();
	while(temp%4==1){
		temp=rand();
		level++;
		if(level+1>=MAX_L) break;
	}
	return level;
}

void sktable_print(sktable *sk){
	for(int i=0; i<KEYN; i++){
		printf("%u:%u\n",sk->meta[i].key,sk->meta[i].ppa);
	}
}

int skiplist_delete(skiplist* list, KEYT key){
	if(list->size==0)
		return -1;
	snode *update[MAX_L+1];
	snode *x=list->header;
	for(int i=list->level; i>=1; i--){
		while(x->list[i]->key<key)
			x=x->list[i];
		update[i]=x;
	}
	x=x->list[1];

	if(x->key!=key)
		return -2; 

	for(int i=x->level; i>=1; i--){
		update[i]->list[i]=x->list[i];
		if(update[i]==update[i]->list[i])
			list->level--;
	}

	//free(x->value);
	if(x->req!=NULL){
		x->req->end_req(x->req);
	}
	free(x->list);
	free(x);
	list->size--;
	return 0;
}

snode *skiplist_insert(skiplist *list,KEYT key, char *value, lsmtree_req_t* req,bool flag){
	snode *update[MAX_L+1];
	snode *x=list->header;
	if(key==UINT_MAX) return NULL;
	if(list->start > key )list->start=key;
	if(list->end < key) list->end=key;
	for(int i=list->level; i>=1; i--){
		while(x->list[i]->key<key)
			x=x->list[i];
		update[i]=x;
	}
	x=x->list[1];

	if(key==x->key){
		if(x->vflag && !flag){ //delete
			delete_ppa(data_segment,x->ppa);
			if(x->req!=NULL && x->req->req!=NULL){
#ifdef ENABLE_LIBFTL
				memio_free_dma(1,x->req->req->dmaTag);	
#else
				free(x->req->req->value);
#endif
			}
			x->vflag=flag;
			x->req=NULL;
			x->value=NULL;
		}
		else{ //update || write after delete
			if(x->vflag){ //update
				if(x->req!=NULL){
#ifdef ENABLE_LIBFTL
					memio_free_dma(1,x->req->req->dmaTag);
#else
					free(x->req->req->value);
#endif
				}
				else{
					delete_ppa(data_segment,x->ppa);
				}
			}
			x->vflag=flag;
			x->value=value;
			x->req=req;
		}
		return x;
	}
	else{
		int level=getLevel();
		if(level>list->level){
			for(int i=list->level+1; i<=level; i++){
				update[i]=list->header;
			}
			list->level=level;
		}

		x=(snode*)malloc(sizeof(snode));
		x->vflag=flag;
		x->list=(snode**)malloc(sizeof(snode*)*(level+1));
		x->key=key;
		x->req=req;
		if(value !=NULL){
			x->value=value;
		}
		for(int i=1; i<=level; i++){
			x->list[i]=update[i]->list[i];
			update[i]->list[i]=x;
		}
		x->level=level;
		list->size++;
	}
	return x;
}

void skiplist_dump(skiplist *list){
	for(int i=list->level; i>=1; i--){
		printf("level dump - %d\n",i);
		snode *temp=list->header->list[i];
		while(temp!=list->header){
			printf("%u ",temp->key);
			temp=temp->list[i];
		}
		printf("\n\n");
	}
}
void skiplist_ex_value_free(skiplist *list){
	free(list->header->list);
	free(list->header);
	free(list);
}
void skiplist_meta_free(skiplist *list){
	snode *temp=list->header->list[1];
	while(temp!=list->header){
		snode *next_node=temp->list[1];
		free(temp->list);
		free(temp);
		temp=next_node;
	}
	skiplist_ex_value_free(list);
}
void skiplist_free(skiplist *list){
	snode *temp=list->header->list[1];
	while(temp!=list->header){
		snode *next_node=temp->list[1];
		free(temp->list);
		//free(temp->req->req->value);
		//free(temp->req->req);
		free(temp->req);
		free(temp);
		temp=next_node;
	}
	skiplist_ex_value_free(list);
}

void skiplist_sktable_free(sktable *f){
	free(f->value);
	free(f);
}
sktable *skiplist_read(KEYT pbn, int hfd, int dfd){
	sktable *res=(sktable*)malloc(sizeof(sktable));
	return res;
}
sktable *skiplist_meta_read_c(KEYT pbn, int fd,int seq,lsmtree_gc_req_t *req){
	lsmtree_gc_req_t *temp=(lsmtree_gc_req_t*)malloc(sizeof(lsmtree_gc_req_t));
	temp->type=LR_DR_T;
	temp->req=NULL;
#ifndef ENABLE_LIBFTL
	temp->keys=(keyset*)malloc(PAGESIZE);
#else	
	char *temp_p;
	temp->dmatag=memio_alloc_dma(2,&temp_p);
	temp->keys=(keyset*)temp_p;
#endif
	temp->seq_number=seq;
	temp->end_req=lr_gc_end_req;
	temp->parent=req;

#ifndef ENABLE_LIBFTL
	pthread_mutex_lock(&dfd_lock);
	int number=0;
	if((number=lseek64(fd,((off64_t)PAGESIZE)*(pbn),SEEK_SET))==-1){
		printf("lseek error in meta read!\n");
		sleep(10);
	}
	read(fd,temp->keys,PAGESIZE);
	pthread_mutex_unlock(&dfd_lock);
	temp->end_req(temp);
#else
	temp->isgc=true;
	//pthread_mutex_lock(&endR);
	//MS(&mt);
	memio_read(mio,pbn,(uint64_t)(PAGESIZE),(uint8_t*)temp->keys,1,temp,temp->dmatag);
	//ME(&mt,"read");
#endif
	return NULL;
}
sktable *skiplist_meta_read_n(KEYT pbn, int fd,int seq,lsmtree_req_t *req){
	lsmtree_req_t *temp=(lsmtree_req_t*)malloc(sizeof(lsmtree_req_t));
	temp->type=LR_DR_T;
	temp->req=NULL;
#ifndef ENABLE_LIBFTL
	temp->keys=(keyset*)malloc(PAGESIZE);
#else	
	char *temp_p;
	temp->dmatag=memio_alloc_dma(2,&temp_p);
	//printf("meta! [%d]\n",temp->dmatag);
	temp->keys=(keyset*)temp_p;
#endif
	temp->seq_number=seq;
	temp->end_req=req->end_req;
	temp->parent=req;

#ifndef ENABLE_LIBFTL
	pthread_mutex_lock(&dfd_lock);
	int number=0;
	if((number=lseek64(fd,((off64_t)PAGESIZE)*(pbn),SEEK_SET))==-1){
		printf("lseek error in meta read!\n");
		sleep(10);
	}
	read(fd,temp->keys,PAGESIZE);
	pthread_mutex_unlock(&dfd_lock);
	temp->end_req(temp);
#else
	temp->isgc=false;
	measure_init(&temp->mt);
	//MS(&mt);
	memio_read(mio,pbn,(uint64_t)(PAGESIZE),(uint8_t*)temp->keys,1,temp,temp->dmatag);
	//ME(&mt,"read");
#endif
	return NULL;
}
sktable* skiplist_data_read(sktable *list, KEYT pbn, int fd){/*
																lseek(fd,SKIP_BLOCK*pbn,SEEK_SET);
																list->value=malloc(KEYN*PAGESIZE);
																read(fd,list->value,KEYN*PAGESIZE);*/
	return NULL;
}

keyset *skiplist_keyset_find(sktable *t, KEYT key){
	short now;
	KEYT start=0,end=KEYN;
	KEYT mid=(start+end)/2;
	if(t->meta[0].key > key)
		return NULL;
	if(t->meta[KEYN-1].key < key)
		return NULL;
	while(1){
		if(start>end) return NULL;
		if(start==end){
			if(key==t->meta[mid].key){
				return &t->meta[mid];
			}
			else
				return NULL;
		}
		if(key==t->meta[mid].key)
			return &t->meta[mid];
		else if(key<t->meta[mid].key){
			end=mid-1;
			mid=(start+end)/2;
		}
		else if(key> t->meta[mid].key){
			start=mid+1;
			mid=(start+end)/2;
		}
	}
}
bool skiplist_keyset_read_c(keyset *k,char *res, int fd, lsmtree_gc_req_t *req){
	if(k==NULL)
		return false;
	lsmtree_gc_req_t *temp=(lsmtree_gc_req_t*)malloc(sizeof(lsmtree_gc_req_t));
	temp->type=LR_DDR_T;
	temp->end_req=req->end_req;
	temp->params[2]=(void*)res;
	temp->parent=req;
	char *temp_p;
#ifndef ENABLE_LIBFTL
	temp_p=(char*)malloc(PAGESIZE);
	temp->data=temp_p;
	pthread_mutex_lock(&dfd_lock);
	if(lseek64(fd,((off64_t)PAGESIZE)*(k->ppa),SEEK_SET)==-1)
		printf("lseek error in meta read!\n");
	read(fd,res,PAGESIZE);
	pthread_mutex_unlock(&dfd_lock);
	temp->end_req(temp);
#else
	temp->dmatag=memio_alloc_dma(2,&temp_p);
	temp->data=temp_p;
	temp->isgc=true;
	memio_read(mio,k->ppa,(uint64_t)(PAGESIZE),(uint8_t*)res,1,temp,temp->dmatag);
#endif
}
bool skiplist_keyset_read(keyset* k,char *res,int fd,lsmtree_req_t *req){
	if(k==NULL)
		return false;
	lsmtree_req_t *temp=(lsmtree_req_t*)malloc(sizeof(lsmtree_req_t));

	temp->type=LR_DDR_T;
	temp->req=req->req;
	temp->end_req=req->end_req;
	temp->parent=req;
#ifndef ENABLE_LIBFTL
	pthread_mutex_lock(&dfd_lock);
	if(lseek64(fd,((off64_t)PAGESIZE)*(k->ppa),SEEK_SET)==-1)
		printf("lseek error in meta read!\n");
	read(fd,res,PAGESIZE);
	pthread_mutex_unlock(&dfd_lock);
	temp->end_req(temp);
#else
	temp->isgc=false;
	//MS(&mt);
	memio_read(mio,k->ppa,(uint64_t)(PAGESIZE),(uint8_t*)res,1,temp,temp->req->dmaTag);
	//ME(&mt,"read");
#endif
	return true;
}
KEYT skiplist_write(skiplist *data, lsmtree_gc_req_t * req,int hfd,int dfd,double fpr){
	skiplist_data_write(data,dfd,req);
	return skiplist_meta_write(data,hfd,req,fpr);
}
int seq_number=0;
KEYT skiplist_meta_write(skiplist *data,int fd, lsmtree_gc_req_t *req,double fpr){
	int now=0;
	int i=0;
	snode *temp=data->header->list[1];
	KEYT temp_pp;
	temp_pp=getPPA(header_segment,(void*)req);
	int test=0;
	for(int j=0;j<1; j++){
		lsmtree_gc_req_t *temp_req=(lsmtree_gc_req_t*)malloc(sizeof(lsmtree_gc_req_t));
		temp_req->isgc=true;
		//printf("-----------------------%p\n",temp_req);
		temp_req->type=LR_DW_T;
#ifndef ENABLE_LIBFTL
		temp_req->keys=(keyset*)malloc(PAGESIZE);
#else	
		char *temp_p;	
		temp_req->dmatag=memio_alloc_dma(1,&temp_p);
		temp_req->keys=(keyset*)temp_p;
#endif
		//temp_req->parent=req;
		temp_req->end_req=lr_gc_end_req;
		data->bitset=(uint8_t*)malloc(sizeof(uint8_t)*(KEYN/8));
		memset(data->bitset,0,sizeof(uint8_t)*(KEYN/8));
		//bloofilter init
#ifdef BLOOM
		data->filter=bf_init(KEYN, fpr);
#endif
		int temp_i=0;
		for(int i=0; temp!=data->header; i++){
			int bit_n=i/8;
			int offset=i%8;
			temp_req->keys[i].key=temp->key;
			if(test<temp->key){
				test=temp->key;
			}
			else{
				printf("??");
			}
			temp_req->keys[i].ppa=temp->ppa;
			if(temp->vflag){
				data->bitset[bit_n] |= (1<<offset);	
			}
#ifdef BLOOM
		 	bf_set(data->filter,temp_req->keys[i].key);
#endif
			temp=temp->list[1];
			temp_i=i;
		}
		temp_i++;
		for(int i=temp_i; i<KEYN; i++){
			temp_req->keys[i].key=NODATA;
			temp_req->keys[i].ppa=NODATA;
		}
		//oob
		sktable test_sk;
		memcpy(&test_sk,temp_req->keys,PAGESIZE);
		uint64_t temp_oob=0;
		KEYSET(temp_oob,temp_req->keys[0].key);
		FLAGSET(temp_oob,0);
		oob[temp_pp]=temp_oob;
#ifndef ENABLE_LIBFTL
		pthread_mutex_lock(&dfd_lock);
		if(lseek64(fd,((off64_t)PAGESIZE)*temp_pp,SEEK_SET)==-1)
			printf("lseek error in meta read!\n");
		write(fd,temp_req->keys,PAGESIZE);
		pthread_mutex_unlock(&dfd_lock);
		temp_req->end_req(temp_req);
#else	
		temp_req->seq_number=seq_number++;
		temp_req->isgc=true;
		memio_write(mio,temp_pp,(uint64_t)(PAGESIZE),(uint8_t*)temp_req->keys,1,(void*)temp_req,temp_req->dmatag);
#endif
	}
	return temp_pp;
}
KEYT skiplist_data_write(skiplist *data,int fd,lsmtree_gc_req_t * req){
	lsmtree_req_t *child_req;
	snode *temp=data->header->list[1];
	int cnt=0;
	int t_cnt=0;
	//MS(&mt);
	while(temp!=data->header){
		child_req=temp->req;
		child_req->type=LR_DDW_T;
		child_req->end_req=lr_end_req;
		//	child_req->parent=(lsmtree_req_t*)req;
		if(temp->vflag){
			KEYT temp_p=getPPA(data_segment,(void*)req);
			uint64_t temp_oob=0;
			KEYSET(temp_oob,temp->key);
			FLAGSET(temp_oob,1);
			//printf("oob:%u\n",temp_p);
			oob[temp_p]=temp_oob;
			temp->ppa=temp_p;
#ifndef ENABLE_LIBFTL
			pthread_mutex_lock(&dfd_lock);
			if(lseek64(fd,((off64_t)PAGESIZE)*(temp_p),SEEK_SET)==-1)
				printf("lseek error in meta read!\n");

			write(fd,temp->value,PAGESIZE);
			pthread_mutex_unlock(&dfd_lock);
			child_req->end_req(child_req);
#else
			child_req->seq_number=seq_number++;
			child_req->isgc=false;
			memio_write(mio,temp_p,(uint64_t)(PAGESIZE),(uint8_t*)temp->value,1,(void*)child_req,child_req->req->dmaTag);
#endif
		}
		else{
			temp->ppa=DELETEDKEY;
#ifndef ENABLE_LIBFTL
			child_req->end_req(child_req);
#endif
		}
		temp=temp->list[1];
	}
	//ME(&mt,"memio_write");
	return 0;
}
void skiplist_sk_data_write(sktable *sk, int fd, lsmtree_gc_req_t *req){
	for(int i=0; i<KEYN; i++){
		lsmtree_gc_req_t *temp=(lsmtree_gc_req_t *)malloc(sizeof(lsmtree_gc_req_t));
		temp->end_req=lr_gc_end_req;
		char *temp_p;
#ifdef ENABLE_LIBFTL
		temp->dmatag=memio_alloc_dma(1,&temp_p);
#else
		temp_p=(char*)malloc(PAGESIZE);
#endif
		memcpy(temp_p,&sk->value[i*PAGESIZE],PAGESIZE);
		temp->data=temp_p;
		temp->req=NULL;
		temp->type=LR_DDW_T;
		delete_ppa(data_segment,sk->meta[i].ppa);
		KEYT temp_pp=getPPA(data_segment,(void*)req);
		sk->meta[i].ppa=temp_pp;
		uint64_t temp_oob=0;
		KEYSET(temp_oob,sk->meta[i].key);
		FLAGSET(temp_oob,1);
		oob[temp_pp]=temp_oob;
#ifdef ENABLE_LIBFTL
		memio_write(mio,temp_pp,(uint64_t)PAGESIZE,(uint8_t*)temp_p,1,(void*)temp,temp->dmatag);
#else
		lseek64(fd,((off64_t)PAGESIZE)*temp_pp,SEEK_SET);
		write(fd,temp_p,PAGESIZE);
		temp->end_req(temp);
#endif
	}
}

skiplist *skiplist_cut(skiplist *list,KEYT num){
	if(num==0) return NULL;
	if(list->size<num) return NULL;
	skiplist *res=(skiplist*)malloc(sizeof(skiplist));
	res=skiplist_init(res);
	snode *h=res->header;
	res->start=-1;
	for(KEYT i=0; i<num; i++){
		snode *temp=skiplist_pop(list);
		if(temp==NULL) return NULL;
		res->start=temp->key>res->start?res->start:temp->key;
		res->end=temp->key>res->end?temp->key:res->end;
		h->list[1]=temp;
		temp->list[1]=res->header;
		h=temp;
	}
	res->size=num;
	return res;
}
KEYT sktable_meta_write(sktable *input,lsmtree_gc_req_t *req,int fd,void **ft,float fpr){
	KEYT temp_pp=getPPA(header_segment,(void*)req);
	BF *filter;
	lsmtree_gc_req_t *temp_req=(lsmtree_gc_req_t*)malloc(sizeof(lsmtree_req_t));
	temp_req->isgc=true;
	temp_req->type=LR_DW_T;

#ifndef ENABLE_LIBFTL
	temp_req->keys=(keyset*)malloc(PAGESIZE);
#else	
	char *temp_p;	
	temp_req->dmatag=memio_alloc_dma(1,&temp_p);
	temp_req->keys=(keyset*)temp_p;
#endif
	//temp_req->parent=req;
	temp_req->end_req=lr_gc_end_req;
#ifdef BLOOM
	filter=bf_init(KEYN, fpr);
#endif
	for(int i=0; i<KEYN; i++){
		bf_set(filter,input->meta[i].key);
	}
	memcpy(temp_req->keys,input,PAGESIZE);
#ifndef ENABLE_LIBFTL
	pthread_mutex_lock(&dfd_lock);
	if(lseek64(fd,((off64_t)PAGESIZE)*temp_pp,SEEK_SET)==-1)
			printf("lseek error in meta read!\n");
	write(fd,temp_req->keys,PAGESIZE);
	pthread_mutex_unlock(&dfd_lock);
	temp_req->end_req(temp_req);
#else	
	memio_write(mio,temp_pp,(uint64_t)(PAGESIZE),(uint8_t*)temp_req->keys,1,(void*)temp_req,temp_req->dmatag);
#endif
	BF **_ft=(BF**)ft;
	*_ft=filter;
	return temp_pp;
}
#ifdef DEBUG
int main(){
	skiplist * temp;

	char data[SNODE_SIZE]={0,};
	int dfd=open("data/test_data.skip",O_CREAT|O_RDWR|O_TRUNC,0666);
	int hfd=open("data/test_header.skip",O_CREAT|O_RDWR|O_TRUNC,0666);
	for(int i=0; i<2; i++){
		temp=(skiplist*)malloc(sizeof(skiplist));
		skiplist_init(temp);
		for(int j=0+i*KEYN; j<(i+1)*KEYN; j++){
			memcpy(data,&j,sizeof(j));
			skiplist_insert(temp,j,data,true);
		}
		skiplist_write(temp,i,hfd,dfd);
		skiplist_free(temp);
	}

	sktable *t;
	for(int i=0; i<2; i++){
		t=skiplist_read(i,hfd,dfd);
		for(int j=0; j<KEYN; j++){
			printf("%d\n",t->meta[j].key);
		}
	}
}
#endif
