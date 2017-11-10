#ifndef __DELETE_H__
#define __DELETE_H__
#include "utils.h"
#include"skiplist.h"
#include"ppa.h"
#include<limits.h>
void delete_init();//for oob
void delete_ppa(delete_set *set,KEYT ppa);
int delete_trim_process_header(delete_set *);
int delete_trim_process_data(delete_set *);
void delete_free(delete_set *);
void delete_dirty_set(delete_set *,KEYT ppa);
int delete_get_victim(delete_set *);
inline uint64_t KEYSET(uint64_t &des,uint32_t src){
	uint64_t temp=src;
	des=des | (temp<<32);
	return des;
}

inline uint64_t FLAGSET(uint64_t &des, bool flag){
	des=des | flag;
	return des;
}/*
inline uint64_t LEVELSET(uint64_t &des, uint8_t level){//for header
	uint64_t temp=level;
	des=des | (temp<<8);
	return des;
}*/
inline KEYT KEYGET(uint64_t des){
	KEYT temp=UINT_MAX;
	return temp&(des>>32);
}
inline uint8_t LEVELGET(uint64_t des){//for header
	uint64_t temp=UCHAR_MAX;
	return temp&(des>>8);
}
inline bool FLAGGET(uint64_t &des){
	return des & 1;
}
#endif
