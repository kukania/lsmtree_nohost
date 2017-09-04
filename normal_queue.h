#ifndef __Q_HEADER__
#define __Q_HEADER__
#include"utils.h"
typedef struct q_node{
	struct q_node * next;
	KEYT data;
}q_node;
typedef struct{
	int size;
	q_node *header;
	q_node *tail;
}queue;

void queue_init(queue *q);
KEYT queue_pop(queue *q);
void queue_push(queue *q,KEYT input);
#endif
