#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>

#include <pthread.h>

#include "config.h"
#include "charqueue.h"

/*======================================== misc ======================================== */
struct client{
	int fd;
	struct sockaddr_in clientaddr;
	socklen_t len;
};

struct client connections[CONNMAX];
size_t conn_i = 0;

void *thr_accept(void *arg);					/* accept connections */
void *thr_recieve(void *arg);					/* recieve messages */
void *thr_broadcast(void *arg); 				/* send messages to other clients*/
void purge_connection(size_t connection_ind);

void server_action();
int message_input(char* dest);                

chq messages;		/* message queue */

pthread_mutex_t conn_i_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t conn_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t chq_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t chq_cond = PTHREAD_COND_INITIALIZER;

enum thstate conn_flag, recv_flag, broad_flag; 

/*======================================== server ======================================== */
int main(int argc, char *argv[]){
	/*----------------------- connection -----------------------*/
	int fd = socket(PF_INET, SOCK_STREAM, 0);
	if(fd == -1){perror("Failed to appoint a file descriptor for server"); return 1;}

	struct sockaddr_in servaddr;

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) == -1){
		perror("Failed to set server fd to be nonblocking");
		exit(1);
	}

	if(bind(fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
		perror("Failed to bind server"); return 1; }
	if(listen(fd, SOMAXCONN) == -1){perror("Failed to listen to connections"); return 1;}

	/*----------------------- thread init -----------------------*/
	chq_init(&messages);

	pthread_t accept_th, receive_th;
	pthread_t broadcast_th[THREAD_POOL];
	conn_flag = broad_flag = recv_flag = thwork;

	if(pthread_create(&accept_th, NULL, &thr_accept, (void*)&fd) != 0){
		fprintf(stderr, "Failed to create accept_connection thread\n");
		return 1;
	}
	if(pthread_create(&receive_th, NULL, &thr_recieve, NULL) != 0){
		fprintf(stderr, "Failed to create receive_message thread\n");
		return 1;
	}
	size_t brdcst_i;
	for(brdcst_i = 0; brdcst_i < THREAD_POOL; ++brdcst_i){
		if(pthread_create(&broadcast_th[brdcst_i], NULL, &thr_broadcast, NULL) != 0){
			fprintf(stderr, "Failed to create broadcast thread %zu\n", brdcst_i);
			return 1;
		}
	}

	/*----------------------- work -----------------------*/
	server_action();

	/*----------------------- termination -----------------------*/
	conn_flag = recv_flag = broad_flag = thterm;

	pthread_mutex_lock(&conn_i_mtx);
		size_t fd_i, fd_end = conn_i;
		conn_i = 0;
	pthread_mutex_unlock(&conn_i_mtx);

	for(fd_i = 0; fd_i < fd_end; ++fd_i){
		close(connections[fd_i].fd);
	}
	close(fd);
	chq_deinit(&messages);

	pthread_exit(0);
}

/*======================================== defs ======================================== */
void *thr_accept(void *arg){
	pthread_detach(pthread_self());
	int fd = *(int*)arg;
	struct client temp;

	while(conn_flag != thterm) {
		if(conn_i < CONNMAX){
			temp.fd = accept(fd, (struct sockaddr*)&temp.clientaddr, &temp.len);
			if(temp.fd == -1){continue;}

			if(fcntl(temp.fd, F_SETFL, fcntl(temp.fd, F_GETFL, 0) | O_NONBLOCK) == -1){
				perror("Failed to set client fd to be nonblocking");
				exit(1);
			}
			else if(temp.fd > 0){
				printf(SERVMSG"Accepted connection on fd %d.\n", temp.fd);
				pthread_mutex_lock(&conn_i_mtx);
					connections[conn_i].fd = temp.fd;
					connections[conn_i].clientaddr = temp.clientaddr;
					connections[conn_i].len = temp.len;
					++conn_i;
				pthread_mutex_unlock(&conn_i_mtx);
			}
		}
	}

	return NULL;
}

void *thr_recieve(void *arg){
	pthread_detach(pthread_self());
	char buf[MSG_SIZE_MAX];
	int b_read;;

	while(recv_flag != thterm) {
		size_t i;
		for(i = 0; i < conn_i; ++i){
			b_read = recv(connections[i].fd, buf, MSG_SIZE_MAX, 0);
			if(b_read > 0){
				if(strcmp(buf, quit) == 0){
					pthread_mutex_lock(&conn_i_mtx);
						purge_connection(i);
						--conn_i;
					pthread_mutex_unlock(&conn_i_mtx);
				} else {
					pthread_mutex_lock(&chq_mtx);
						if(enqueue(&messages, buf) == NULL){fprintf(stderr, "Failed to add message to a queue\n");}
					pthread_mutex_unlock(&chq_mtx);
					pthread_cond_signal(&chq_cond);
				}
			}
		}
	}

	pthread_cond_broadcast(&chq_cond);
	return NULL;
}

void purge_connection(size_t connection_ind){
	printf(SERVMSG"Purged connection at %d.\n", connections[connection_ind].fd);
	close(connections[connection_ind].fd);
	if(connection_ind != conn_i-1 && conn_i != 1){
		connections[connection_ind].fd         = connections[conn_i-1].fd;
		connections[connection_ind].clientaddr = connections[conn_i-1].clientaddr;
		connections[connection_ind].len 	   = connections[conn_i-1].len;
	}
}

void *thr_broadcast(void *arg){
	pthread_detach(pthread_self());
	size_t i;
	int b_sent;
	char *buf;

	while(broad_flag != thterm) {
		pthread_mutex_lock(&chq_mtx);
			pthread_cond_wait(&chq_cond, &chq_mtx);
			buf = dequeue(&messages);
		pthread_mutex_unlock(&chq_mtx);

		if(buf){
			for(i = 0; i < conn_i; ++i){
				b_sent = send(connections[i].fd, buf, MSG_SIZE_MAX, 0);
			}
			printf(DELIMETER"%s\n", buf);
			free(buf);
		}
	}

	return NULL;
}

void server_action(void){
	char buf[MSG_SIZE_MAX];
	int b_inp;

	helpmsg_server();
	while(1){
		memset(buf, 0, MSG_SIZE_MAX);
		if((b_inp = message_input(buf)) > 0){

			if(strcmp(buf, quit) == 0){
				return;
			} 
			else if(strcmp(buf, help) == 0){
				helpmsg_server();
			}
			else if(strcmp(buf, conn) == 0){
				printf(SERVMSG"Current connections: %zu\n", conn_i);
			}
			else {
				printf("Unknown command.\n");
			}
		}
	}
}


