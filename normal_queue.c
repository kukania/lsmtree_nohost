#include"normal_queue.h"

void queue_init(queue *q){
	q->header=NULL;
	q->tail=NULL;
	q->size=0;
}

KEYT queue_pop(queue *q){
	if(q->size==0)
		return -1;
	q_node *temp=q->header;
	KEYT res=temp->data;
	q->header=temp->next;
	free(temp);
	q->size--;
	return res;
}
void queue_push(queue* q,KEYT input){
	if(q->header==NULL){
		q->header=(q_node*)malloc(sizeof(q));
		q->header->data=input;
		q->size++;
		return;
	}
	if(q->tail==NULL){
		q->tail=(q_node*)malloc(sizeof(q));
		q->header->next=q->tail;
		q->tail->next=NULL;
		q->tail->data=input;
		q->size++;
		return;
	}
	q_node *temp=(q_node *)malloc(sizeof(q_node));
	temp->data=input;
	q->tail->next=temp;
	temp->next=q->tail->next;
	q->tail=temp;
	q->size++;
	return;
}
