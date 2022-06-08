#ifndef CHARQUEUE_H
#define CHARQUEUE_H

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

/*======================================== declarations ======================================== */
#define MSGBUFSIZE 32

typedef struct charqueue{
	char *buf[MSGBUFSIZE];

	size_t first;
	size_t end;
	size_t items;
} chq;


void chq_init(chq *q);
void chq_deinit(chq *q);

char *enqueue(chq *q, char *msg);
char *dequeue(chq *q);

size_t maxbufsize(){return MSGBUFSIZE;}

/*======================================== init - deinit ======================================== */
void chq_init(chq *q){
	q->first = q->items = 0;
	q->end = -1;

	size_t i;
	for(i = 0; i < MSGBUFSIZE; ++i){q->buf[i] = NULL;}
}

void chq_deinit(chq *q){
	size_t i;
	for(i = 0; i < MSGBUFSIZE; ++i){
		if(q->buf[i]){
			free(q->buf[i]);
			q->buf[i] = NULL;
			q->items--;
		}
	}
}

/*======================================== addition ======================================== */
char *enqueue(chq *q, char *msg){
	if(q->items == MSGBUFSIZE){return NULL;}

	if(q->end == MSGBUFSIZE - 1){q->end = -1;}
	q->end++;
	if(q->buf[q->end] != NULL){
		free(q->buf[q->end]);
	}
	size_t tmplen = strlen(msg);
	q->buf[q->end] = (char*)malloc(tmplen);
	if(q->buf[q->end] == NULL){return NULL;}
	
	strncpy(q->buf[q->end], msg, tmplen);
	q->items++;

	return q->buf[q->end];
}

/*======================================== substraction ======================================== */
char *dequeue(chq *q){
	if(q->items == 0){return NULL;}

	size_t tmplen = strlen(q->buf[q->first]);
	char *tmp = (char*)malloc(tmplen);
	if(tmp == NULL){return NULL;}

	strncpy(tmp, q->buf[q->first], tmplen);

	free(q->buf[q->first]);
	q->buf[q->first++] = NULL;
	q->first %= MSGBUFSIZE;
	q->items--;

	return tmp;
}

#endif /* CHARQUEUE_H */