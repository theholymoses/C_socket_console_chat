#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>

/*======================================== utils ======================================== */
#define PORT             8181
#define CONNMAX			   16
#define THREAD_POOL         8

enum thstate{thterm, thwork}; 		/* thread state */


#define MSG_SIZE_MAX      256		/* max size of a message */
#define PING_TRIES          6
#define DELIMETER "\n*********************\n" 
#define SERVMSG   "\n#####################\n"

/* client and server commands */
char *help = "-->help";
char *quit = "-->quit";

/* server commands */
char *conn = "-->conn";


int message_input(char* dest){
	int chr, b_read = 0;
	while((chr = getchar()) != '\n' && chr != EOF){
		if(b_read < MSG_SIZE_MAX){
			dest[b_read++] = chr;
		} else {
			break;
		}
	}
	dest[b_read++] = '\0';
	return b_read;
}


void helpmsg_client(void){
	printf("\n---------------------------------------------\n");
	printf("Available options:\n");
	printf("%s for printing this message.\n", help);
	printf("%s for quiting programm.\n", quit);
	printf(" - message with a command should not contain anything else and be of form -->command.\n");
	printf(" - your input will be read until end of the line symbol, not EOF.\n");
	printf(" - the size limit of the message is %d minus your login size\n", MSG_SIZE_MAX-1);
	printf("\n---------------------------------------------\n\n");
}

void helpmsg_server(void){
	printf("\n---------------------------------------------\n");
	printf("Available options:\n");
	printf("%s for printing this message.\n", help);
	printf("%s for stopping server.\n", quit);
	printf("%s for showing current connection amount.\n", conn);
	printf(" - message with a command should not contain anything else and be of form -->command.\n");
	printf(" - your input will be read until end of the line symbol, not EOF.\n");
	printf(" - the size limit of the message is %d\n", MSG_SIZE_MAX-1);
	printf("\n---------------------------------------------\n\n");
}


#endif /* CONFIG_H */