#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <zmq.h>
#include <pthread.h>

#include "config.h"

/*==================================== utils ====================================*/
int check_ip(char *ip);
char *make_addr(char *trnsp, char *ip, char *port);

enum threadstate sendthreadstate = work;
enum threadstate recvthreadstate = work;
pthread_t sendthread, recvthread;

void *recvroutn(void *subscriber);
void *sendroutn(void *client);

/*==================================== client ====================================*/
int main(int argc, char *argv[]){
	if(argc < 3) {fprintf(stderr, "Provide 2 arguments: Your nickname and ip address of the server\n"); return 1;}

	/*--------------- setup ---------------*/
	size_t login_len = strlen(argv[1]);
	size_t ip_len = strlen(argv[2]);

	char *login = (char*)malloc(login_len);  	if(!login) {fprintf(stderr, "Failed to alloc memory for login.\n"); return 1;}
	char *ip = (char*)malloc(ip_len);    	 	if(!ip)    {fprintf(stderr, "Failed to alloc memory for ip.\n");    return 1;}

	strncpy(login, argv[1], login_len);
	strncpy(ip, argv[2], ip_len);				if(!check_ip(ip)){fprintf(stderr, "Provided invalid IP address.\n"); return 1;}

	char *servaddr = make_addr(transport, ip, servport);
	char *publaddr = make_addr(transport, ip, publport);

	/*--------------- zmq ---------------*/
	void *context, *client, *subscriber;
	context = zmq_ctx_new();				    if(!context){perror("Failed to create zmq context"); return 1;}
	client =  zmq_socket(context, ZMQ_CLIENT);  if(!client){perror("Failed to create zmq client socket"); return 1;}
	subscriber = zmq_socket(context, ZMQ_SUB);  if(!subscriber){perror("Failed to create zmq subscriber socket"); return 1;}

	if(zmq_connect(client, servaddr) == -1) {perror("Failed to connect client socket to server"); return 1;}
	if(zmq_connect(subscriber, publaddr) == -1){perror("Failed to connect subscriber socket to publisher"); return 1;}

	if(zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, msg, strlen(msg)) == -1){perror("Failed to set subscription"); return 1;}

	/*--------------- thread ---------------*/
	if(pthread_create(&recvthread, NULL, &recvroutn, subscriber) != 0){
		fprintf(stderr, "Failed to create recieve message thread.\n"; return 1;)
	}

}

/*==================================== thread routines ====================================*/
void *recvroutn(void *subscriber){
	char buffer[msg_buffer_size];
	int bytes;

	while(recvthreadstate != term){
		bytes = zmq_recv(subscriber, buffer, msg_buffer_size-1, ZMQ_DONTWAIT);

		if(bytes > 0){
			printf("%s\n", buffer);
			memset(buffer, 0, bytes);
		}
	}

	return NULL;
}


/*==================================== other routines ====================================*/
int check_ip(char *ip){
	struct in_addr inp;
	return inet_aton(ip, &inp);
}


char *make_addr(char *trnsp, char *ip, char *port){
	size_t len = strlen(trnsp) + strlen(ip) + strlen(port) + 1;
	char *addr = (char*)malloc(len);
	if(!addr){fprintf(stderr, "Failed to allocate memory for address string.\n"); exit(1);}
	size_t ind = 0;

	while(*trnsp){
		addr[ind++] = *trnsp++;
	}
	while(*ip){
		addr[ind++] = *ip++;
	}
	while(*port){
		addr[ind++] = *port++;
	}

	addr[ind] = 0;
	return addr;
}