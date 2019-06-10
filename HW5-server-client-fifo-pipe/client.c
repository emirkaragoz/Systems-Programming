#include "server_client.h"

char clientFifo[CLIENT_FIFO_NAME_LEN];

void handler(int);

int main(int argc, char *argv[]){
    int serverFd, clientFd, clientNumber, pid;
    struct request req;
    struct response resp = {.money = -999};
    
    if(argc != 2){
        errExit("Invalid usage. It must be ./Client [musteriProcessSayisi]");
    }
    clientNumber = atoi(argv[1]);
    if(clientNumber == 0){
        errExit("Invalid client number. It must be ./Client [musteriProcessSayisi]");
    }     

    signal(SIGTERM, handler); 
    signal(SIGINT, handler); 

    for (int i = 0; i < clientNumber ; ++i){   
        pid = fork();
        if(pid > 0){
            if(i == clientNumber-1){
                while(wait(NULL)>0);    //cliet drove wait all clients done
                exit(EXIT_SUCCESS);  
            }    
        }
        else if(pid == 0){ 
            break;
        }
        else{
            errExit("fork");
        }
    } 
    
    sprintf(clientFifo, CLIENT_FIFO_TEMPLATE, (long) getpid());
    if (mkfifo(clientFifo, 0666) == -1 && errno != EEXIST)
        errExit("mkfifo client fifo");

    // open sv FIFO and send request to server
    serverFd = open(SERVER_FIFO, O_WRONLY);
    if (serverFd == -1)
        errExit("open server fifo");

    req.pid = getpid();
    if (write(serverFd, &req, sizeof(struct request)) != sizeof(struct request)){
        unlink(clientFifo);     //each client unlike own FIFO
        errExit("Can't write to server");
    }

    /* Open client FIFO, read response */

    clientFd = open(clientFifo, O_RDONLY);
    if (clientFd == -1){
        unlink(clientFifo);     //each client unlike own FIFO
        errExit("open client fifo");
    }

    read(clientFd, &resp, sizeof(struct response));

    if(resp.money > 0 && resp.money <= 100)  
        printf("Musteri %d %d lira aldı :)\n", getpid(),resp.money);
    else    //behindhand clients
        printf("Musteri %d parasını alamadi :(\n",getpid() );
    unlink(clientFifo);     //each client unlike own FIFO
    exit(EXIT_SUCCESS);
}

void handler(int signo){
    if(signo == SIGINT)
        printf("  SIGINT(^C) handled!\n");
        
    else if(signo == SIGTERM)
        printf("  SIGTERM handled!\n");

    unlink(clientFifo);     //each client unlike own FIFO
    exit(EXIT_SUCCESS);
}