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
uint64_t *oob;
delete_set *data_segment;
delete_set *header_segment;
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
	req->req=NULL;
	req->end_req=lr_end_req;
	return req;
}
void delete_init(){//for test
	oob=(uint64_t *)malloc((MAXPAGE)*sizeof(uint64_t));
	memset(oob,0,(MAXPAGE)*sizeof(uint64_t));
	data_segment=(delete_set*)malloc(sizeof(delete_set));
	header_segment=(delete_set *)malloc(sizeof(delete_set));
	segment_init(header_segment,0,1,false);//
	segment_init(data_segment,2,SEGNUM-3,true); // 0~SEGNUM-4, reserve SEGNUM-3
}
void delete_ppa(delete_set *set,KEYT input){
	int block_num=input/PAGENUM;
	int offset_meta=input%PAGENUM;

	int bit_num=offset_meta/8;
	int offset=offset_meta%8;

	block_num-=set->start_block_n;
	uint8_t target=1;
	target=target<<offset;
	target=~target;
	for(int i=0; i<set->size; i++){
		if(set->blocks[i].number!=block_num) continue;
		set->blocks[i].bitset[bit_num] &= target;
		set->blocks[i].invalid_n++;
		if(set->blocks[block_num].invalid_n > PAGENUM){
			printf("over block! : %d\n",set->blocks[block_num].number);	
		}
	}
}

int delete_get_victim(delete_set *set){
	int invalids=0;
	int block_num=0;
	for(int i=0; i<set->size; i++){
		if(invalids<set->blocks[i].invalid_n){
			invalids=set->blocks[i].invalid_n;
			block_num=i;
		}
	}
	if(invalids==0)
		return -1;
	return block_num;
}

int delete_trim_process_header(delete_set *set){
	int block_num=delete_get_victim(set); //segment's order
	if(block_num==-1)
		return 0;
	int invalids=set->blocks[block_num].invalid_n;
	if(invalids==PAGENUM){
		segment_block_init(set,block_num);
		//send trim process
		return 1;
	}
	else{
		bool stop_flag=false;
		for(int i=0; i<PAGENUM/8; i++){
			for(int j=0; j<8; j++){ //each page
				uint8_t test=1<<j;
				KEYT temp_p_key=set->blocks[block_num].number*PAGENUM+i*8+j;
				if(!(set->blocks[block_num].bitset[i]&test)){//1 valid, 0 invalid
					continue;
				}
				lsmtree_req_t *req=delete_make_req(0);
				sktable *sk=(sktable*)malloc(sizeof(sktable));
				req->params[2]=(void*)sk;
				char *temp_p;
#ifdef ENABLE_LIBFTL
				KEYT temp_tag;
				req->dmatag=memio_alloc_dma(2,&temp_p);
				temp_tag=req->dmatag;
#else
				temp_p=(char*)malloc(PAGESIZE);
#endif
				req->data=temp_p;
				pthread_mutex_t *temp_mutex=&req->meta_lock;

#ifdef ENABLE_LIBFTL
				memio_read(mio,temp_p_key,(uint64_t)(PAGESIZE),(uint8_t *)req->data,1,req,req->dmatag);//target read
#else
				lseek64(LSM->dfd,((off64_t)PAGESIZE)*temp_p_key,SEEK_SET);
				read(LSM->dfd,temp_p,PAGESIZE);
				req->end_req(req);
#endif

				pthread_mutex_lock(temp_mutex);
				pthread_mutex_destroy(temp_mutex);
#ifdef ENABLE_LIBFTL
				memio_free_dma(2,temp_tag);
#else
				free(temp_p);
#endif
				uint64_t temp_oob=oob[temp_p_key];
				if(temp_oob==0){//not used
					stop_flag=true;
					free(sk);
					break;
				}
				KEYT key=KEYGET(temp_oob);
				Entry *header=NULL;
				for(int i=0; i<LEVELN; i++){
					header=level_find(LSM->buf.disk[i],key);
					if(header!=NULL && header->pbn==temp_p_key)
						break;
				}
				if(header==NULL){
					printf("??\n");
				}
				for(int i=0; i<LEVELN; i++){
					header=level_find(LSM->buf.disk[i],key);
					if(header!=NULL && header->pbn==temp_p_key)
						break;
				}
				uint64_t new_oob=0;

				req=delete_make_req(1);
				KEYT new_pba=getRPPA(set,(void*)req);
				KEYSET(new_oob,key);
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
			if(stop_flag)
				break;
		}
		segment_block_init(set,block_num);
		segment_block_change(set,block_num);
	}
	return 1;
}
int delete_trim_process_data(delete_set *set){
	int block_num=delete_get_victim(set);
	if(block_num==-1)
		return 0;
	int invalids=set->blocks[block_num].invalid_n;
	if(invalids==PAGENUM){
		segment_block_init(set,block_num);
		//send trim operation
		return 1;
	}
	else{
		bool stop_flag=false;
		for(int i=0; i<PAGENUM/8; i++){
			for(int j=0; j<8; j++){
				uint8_t test=1<<j;
				KEYT temp_p_key=set->blocks[block_num].number*PAGENUM+i*8+j;
				if(!(set->blocks[block_num].bitset[i]&test)){//1 valid, 0 invalid
					continue;
				}
				lsmtree_req_t *req=delete_make_req(0);
				sktable *sk=(sktable*)malloc(sizeof(sktable));
				req->params[2]=(void*)sk;
				char *temp_p;
#ifdef ENABLE_LIBFTL
				KEYT temp_tag;
				req->dmatag=memio_alloc_dma(2,&temp_p);
				temp_tag=req->dmatag;
#else
				temp_p=(char*)malloc(PAGESIZE);
#endif
				req->data=temp_p;
				pthread_mutex_t *temp_mutex=&req->meta_lock;

#ifdef ENABLE_LIBFTL
				memio_read(mio,temp_p_key,(uint64_t)(PAGESIZE),(uint8_t *)req->data,1,req,req->dmatag);//target read
#else
				lseek64(LSM->dfd,((off64_t)PAGESIZE)*temp_p_key,SEEK_SET);
				read(LSM->dfd,temp_p,PAGESIZE);
				req->end_req(req);
#endif

				pthread_mutex_lock(temp_mutex);
				pthread_mutex_destroy(temp_mutex);
#ifdef ENABLE_LIBFTL
				memio_free_dma(2,temp_tag);
#else
				free(temp_p);
#endif
				uint64_t temp_oob=oob[temp_p_key];
				if(temp_oob==0){
					printf("temp_p_key :%u-blocknumber:%d block ordered:%d stoped!\n",temp_p_key,set->blocks[block_num].number,block_num);
					stop_flag=true;
					free(sk);
					break;
				}
				KEYT key=KEYGET(temp_oob);
				Entry *header;
				KEYT new_ppa=getRPPA(set,NULL);
				for(int k=0; k<LEVELN; k++){ //header find
					header=level_find(LSM->buf.disk[k],key);
					if(header==NULL)
						continue;
					else{
						//header read
						req=delete_make_req(0);
						sktable *sk_header=(sktable*)malloc(sizeof(sktable));
						req->params[2]=(void*)sk_header;
#ifdef ENABLE_LIBFTL
						KEYT temp_tag;
						req->dmatag=memio_alloc_dma(2,&temp_p);
						temp_tag=req->dmatag;
#else
						temp_p=(char*)malloc(PAGESIZE);
#endif
						req->data=temp_p;
						pthread_mutex_t *temp_mutex=&req->meta_lock;

#ifdef ENABLE_LIBFTL
						memio_read(mio,header->pbn,(uint64_t)(PAGESIZE),(uint8_t *)req->data,1,req,req->dmatag);//target read
#else
						lseek64(LSM->dfd,((off64_t)PAGESIZE)*header->pbn,SEEK_SET);
						read(LSM->dfd,temp_p,PAGESIZE);
						req->end_req(req);
#endif

						pthread_mutex_lock(temp_mutex);
						pthread_mutex_destroy(temp_mutex);
#ifdef ENABLE_LIBFTL
						memio_free_dma(2,temp_tag);
#else
						free(temp_p);
#endif
						keyset *target;
						if((target=skiplist_keyset_find(sk_header,key))){
							if(target->ppa==temp_p_key){
								target->ppa=new_ppa;//update
								req=delete_make_req(1);
								uint64_t new_oob_pba=0;
								KEYT new_pba=getPPA(header_segment,(void*)req);
								KEYSET(new_oob_pba,header->key);
								FLAGSET(new_oob_pba,0);
#ifdef ENABLE_LIBFTL
								req->dmatag=memio_alloc_dma(1,&temp_p);
#else
								temp_p=(char*)malloc(PAGESIZE);
#endif
								req->data=temp_p;
								memcpy(temp_p,sk_header,PAGESIZE);
#ifdef ENABLE_LIBFTL
								memio_write(mio,new_pba,(uint64_t)PAGESIZE,(uint8_t*)req->data,1,req,req->dmatag);
#else
								lseek64(LSM->dfd,((off64_t)PAGESIZE)*new_pba,SEEK_SET);
								write(LSM->dfd,temp_p,PAGESIZE);
								req->end_req(req);				
#endif
								KEYT temp_pba=header->pbn;
								header->pbn=new_pba;
								oob[header->pbn]=new_oob_pba;
								delete_ppa(header_segment,temp_pba);
								free(sk_header);
								break;
							}
							else{
								free(sk_header);
								break; //deprecated data!!, old data
							}
						}
						else{
							free(sk_header);
							continue;
						}
					}
				}
				//write data
				uint64_t new_oob=0;
				req=delete_make_req(1);
				KEYSET(new_oob,key);
				FLAGSET(new_oob,1);
#ifdef ENABLE_LIBFTL
				req->dmatag=memio_alloc_dma(1,&temp_p);
#else
				temp_p=(char*)malloc(PAGESIZE);
#endif
				req->data=temp_p;
				memcpy(temp_p,sk,PAGESIZE);
#ifdef ENABLE_LIBFTL
				memio_write(mio,new_ppa,(uint64_t)PAGESIZE,(uint8_t*)req->data,1,req,req->dmatag);
#else
				lseek64(LSM->dfd,((off64_t)PAGESIZE)*new_ppa,SEEK_SET);
				write(LSM->dfd,temp_p,PAGESIZE);
				req->end_req(req);				
#endif
				oob[new_ppa]=new_oob;
				free(sk);
			}
			if(stop_flag)
				break;
		}
		segment_block_init(set,block_num);
		segment_block_change(set,block_num);
	}
}

