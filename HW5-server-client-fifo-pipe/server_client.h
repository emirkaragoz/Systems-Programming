#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>

/* Name for server's FIFO */
#define SERVER_FIFO "/tmp/seqnum_sv"
/* Template for client FIFO */
#define CLIENT_FIFO_TEMPLATE "/tmp/seqnum_cl.%ld"
/* Space required for client FIFO pathname*/
#define CLIENT_FIFO_NAME_LEN (sizeof(CLIENT_FIFO_TEMPLATE) + 8)

                           
struct request {               
    pid_t pid;                
};

struct response {
    int money;                 
};

struct log {               
    int money;
    int client_pid;
    double second;                 
};

void errExit(char * msg){
	printf("%s\n",msg);
	exit(EXIT_FAILURE);
}