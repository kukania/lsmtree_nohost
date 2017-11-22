#include"cache.h"
#include"lsmtree.h"
#ifdef ENABLE_LIBFTL
#include"libmemio.h"
#endif
#include<stdlib.h>
#include<string.h>
extern lsmtree *LSM;
void cache_init(cache *c){
	c->m_size=CACHESIZE;
	c->n_size=0;
	c->top=NULL;
	c->bottom=NULL;
}

cache_entry * cache_insert(cache *c, Entry *ent, int dmatag){
	if(c->m_size==c->n_size){
		cache_delete(c,cache_get(c));
	}
	cache_entry *c_ent=(cache_entry*)malloc(sizeof(cache_entry));

	c_ent->entry=ent;
	if(c->bottom==NULL){
		c->bottom=c_ent;
		c->top=c_ent;
		c->bottom->down=NULL;
		c->top->up=NULL;
		c->n_size++;
		return c_ent;
	}

	c->top->up=c_ent;
	c_ent->down=c->top;

	c->top=c_ent;
	c_ent->up=NULL;
	c->n_size++;
	return c_ent;
}
int delete_cnt_check;
bool cache_delete(cache *c, Entry * ent){
	if(c->n_size==0) return false;
	cache_entry *c_ent=ent->c_entry;
	if(ent->data)
		free(ent->data);
	ent->data=NULL;
	c->n_size--;
	free(c_ent);
	ent->c_entry=NULL;
	return true;
}

bool cache_delete_entry_only(cache *c, Entry *ent){
	if(c->n_size==0) return false;
	cache_entry *c_ent=ent->c_entry;
	if(c_ent==NULL) return false;
	if(c->bottom==c->top && c->top==c_ent){
		c->top=c->bottom=NULL;
	}
	else if(c->top==c_ent){
		cache_entry *down=c_ent->down;
		down->up=NULL;
		c->top=down;
	}
	else if(c->bottom==c_ent){
		cache_entry *up=c_ent->up;	
		up->down=NULL;
		c->bottom=up;
	}
	else{
		cache_entry *up=c_ent->up;
		cache_entry *down=c_ent->down;
		
		up->down=down;
		down->up=up;
	}
	c->n_size--;
	free(c_ent);
	ent->c_entry=NULL;
	return true;
}
void cache_update(cache *c, Entry* ent){
	cache_entry *c_ent=ent->c_entry;
	if(c->top==c_ent){ 
		return;
	}
	if(c->bottom==c_ent){
		cache_entry *up=c_ent->up;
		up->down=NULL;
		c->bottom=up;
	}
	else{
		cache_entry *up=c_ent->up;
		cache_entry *down=c_ent->down;
		up->down=down;
		down->up=up;
	}

	c->top->up=c_ent;
	c_ent->up=NULL;
	c_ent->down=c->top;
	c->top=c_ent;
}

Entry* cache_get(cache *c){
	if(c->n_size==0) return NULL;
	cache_entry *res=c->bottom;
	cache_entry *up=res->up;

	if(up==NULL){
		c->bottom=c->top=NULL;
	}
	else{
		up->down=NULL;
		c->bottom=up;
	}
	if(!res->entry->c_entry){
		printf("hello\n");
	}
	return res->entry;
}

int print_number;
void cache_print(cache *c){
	cache_entry *start=c->top;
	print_number=0;
	while(start!=NULL){
		if(!start->entry->data)
			printf("nodata!!\n");
		if(start->entry->data){
			if(start->entry->c_entry!=start)
				printf("fuck\n");
		}
		print_number++;
		if(print_number>c->n_size)
			exit(1);
		start=start->down;
	}
}
