#include"run_array.h"

Node *ns_run(level*input ,int n){
	return (Node*)&input->body[input->r_size*n];
}
Entry *ns_entry(Node *input, int n){
	return (Entry)&input->body[input->e_size*n];
}
level *level_init(level *input,int size){
#ifdef Tiering
	input->r_num=MUL;
#else
	input->r_num=1;
#endif
	input->r_size=sizeof(Node)-sizeof(char *)+sizeof(Entry)*size/input->r_num;
	input->size=input->r_size*input->r_num;
	input->body=(char*)malloc(input->size);
	input->n_num=0;
	input->m_num=size;
	for(int i=0; i<input->r_num; i++){
		Node *run=ns_run(input,i);
		run->start=(void*)run;
		run->e_size=sizeof(Entry);
		run->n_num=0;
		run->m_num=size/input->r_num;
		for(int j=0; j<m_num; j++){
			Entry *entry=ns_entry(run,j);
			entry->start=(void*)entry;
		}
	}
	return input;
}
Entry *level_find(level *input,KEYT key){
	for(int i=0; i<input->r_num; i++){
		Node *run=ns_run(input,i);
		if(run->n_num==0) continue;
		int start=0;
		int end=run->n_num;
		int mid;
		while(1){
			mid=(start+end)/2;
			Entry *mid_e=ns_entry(ns_run,mid);
			if(mid_e->key<= key && mid->end>=key)
				return mid;
			if(mid_e->key>key){
				end=mid-1;
			}
			else if(mid_e->end<key){
				start=mid+1
			}
			if(start>end)
				break;
		}
	}
	return NULL;
}
Node *level_insert(level *input,Entry *entry){//always sequential
	if(input->n_size==input->m_size) return NULL;
	int r=input->n_size/r_num;
	Node *temp_run=ns_run(input,r);
	int o=temp_run->n_size;
	Entry *temp_entry=ns_entry(temp_run,o);
	memcpy(temp_entry,entry);
	temp_run->n_size++;
	input->n_size++;
}
Entry *level_get_next(Iter *){
	
}
Iter *level_get_Iter(level *input){
	Iter *res=(Iter*)malloc(sizeof(Iter));
	res->now=ns_node(input,0)
}
void level_print(level *){

}
void level_free(levle *){

}

