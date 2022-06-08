#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>

#include "config.h"

/*======================================== misc ======================================== */
#define LOGIN_SIZE_MIN      1
#define LOGIN_SIZE_MAX     16

#define IP_ADDR_MIN         7
#define IP_ADDR_MAX        15

#define TRY_CONN_MAX        3   	/* max connection tries */


void *thr_getmsg(void *arg);       					   			   /* thread routine for getting messages from server */
enum thstate flag_getmsg;				   			   			   /* flag of thread state see config.h */


void client_action(int fd, char *login, size_t log_sz);		   		   /* choose user action */
int message_input(char* dest);
void helpmsg_client(void);				   							   /* display available actions */

/*======================================== client ======================================== */
int main(int argc, char *argv[]){
	/*----------------------- login setter -----------------------*/
	char *login, *S_IP;
	size_t login_len, ip_len;

	if(argc != 3){
		fprintf(stderr, "Provide your nickname(at most %d letters) and IP of server\n", LOGIN_SIZE_MAX-1);
		return 1;
	}

	login_len = strlen(argv[1]);
	ip_len = strlen(argv[2]);

	if(login_len <= LOGIN_SIZE_MAX){
		login = (char*)malloc(login_len);
		strncpy(login, argv[1], login_len);
	}
	else if(login_len > LOGIN_SIZE_MAX){
		login = (char*)malloc(LOGIN_SIZE_MAX);
		strncpy(login, argv[1], LOGIN_SIZE_MAX);
	}

	S_IP = (char*)malloc(ip_len);
	strncpy(S_IP, argv[2], ip_len);

	/*----------------------- connection -----------------------*/
	int clientfd = socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in servaddr;

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = inet_addr(S_IP);

	int tries = 0;
	while(connect(clientfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0){
		if(tries == TRY_CONN_MAX){fprintf(stderr, "Terminating process.\n"); close(clientfd); return 1;}
		perror("Failed to connect to server");
		sleep(3);
		++tries;
	}

	/*----------------------- thread setup -----------------------*/
	pthread_t th_getter;
	flag_getmsg = thwork;   		/* setting thread flag to "work" */

	if(pthread_create(&th_getter, NULL, &thr_getmsg, (void*)&clientfd) != 0){
		fprintf(stderr, "Failed to create getmsg thread.\n");
		return 1;
	}

	/*----------------------- client control -----------------------*/
	client_action(clientfd, login, login_len);

	/*----------------------- termination -----------------------*/
	close(clientfd);
	free(login);
	free(S_IP);
	
	return 0;
}

/*======================================== defs ======================================== */
void client_action(int fd, char *login, size_t log_sz){
	char msg[MSG_SIZE_MAX], tempbuf[MSG_SIZE_MAX];
	int b_sent, b_input;
	size_t sending;

	helpmsg_client();
	while(1) {
		memset(msg, 0, MSG_SIZE_MAX);
		memset(tempbuf, 0, MSG_SIZE_MAX);

		b_input = message_input(tempbuf);

		if(strcmp(tempbuf, help) == 0){helpmsg_client();}
		else if(strcmp(tempbuf, quit) == 0){
			sending = b_input + 1;
			snprintf(msg, sending, "%s", tempbuf);
			b_sent = send(fd, msg, sending, 0);
			flag_getmsg = thterm;  
			return;

		} else {
			sending = log_sz + b_input + 2;
			snprintf(msg, sending, "%s:\n%s", login, tempbuf);
			b_sent = send(fd, msg, sending, 0);
		}
	}
}


void *thr_getmsg(void *arg){
	pthread_detach(pthread_self());
	int b_read, fd = *(int*)arg;
	char buffer[MSG_SIZE_MAX];

	while(flag_getmsg != thterm){
		b_read = recv(fd, buffer, MSG_SIZE_MAX, 0);

		if(b_read > 0){
			printf(DELIMETER"%s\n", buffer);
		}
	}
	return NULL;
}


