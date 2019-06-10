#include "server_client.h"

int nextOne();
void alarmHandler(int);
int isBusy(int []);
void latency(double);
void afterSignal(int);
void running();

pid_t servers [5];	// 1 bank 4 service (using by main process)
int serversBusyOrFree [4] = {0,0,0,0}; 	// 0 free 1 busy (managing by main process)
int serveCount [4] = {0,0,0,0};	// child processes serve number (manage by main process)
FILE *f;	// log file pointer open write and close only main process
int own; 	// for each process own number
volatile int alarmKey = 1;	//alarm detection (only using by main process)
int serveTime; 		//bank serve time
int serverFd, clientFd;		// server and client file descripter for FIFOs
int pipeRead[4][2], pipeWrite[4][2]; //main and children processes comminication
struct timeval  tv1;	//starting time

int main(int argc, char *argv[]){
    int pid;

    if(argc != 2){
        errExit("Invalid usage. It must be ./Banka [ServisZamani]");
    }
    serveTime = atoi(argv[1]);
    if(serveTime == 0){
    	errExit("Invalid service time. It must be ./Banka [ServisZamani]");
    } 
   	
   	signal(SIGTERM, alarmHandler); 
   	signal(SIGINT, alarmHandler); 
   	signal(SIGALRM, alarmHandler); 	
    alarm(serveTime);		//main process set alarm
    gettimeofday(&tv1, NULL);

    // main process create and open fifo

    if (mkfifo(SERVER_FIFO, 0666) == -1 && errno != EEXIST)
        errExit("mkfifo server fifo");
    
    serverFd = open(SERVER_FIFO, O_RDONLY | O_NONBLOCK);
    if (serverFd == -1)
        errExit("open server fifo");
 

    servers[0] = getpid(); 	//main process
    own = -1;	//main process doesnt have own (detection -1)
    for (int i = 1; i < 5 ; ++i){	//main process created 4 child process
	    if(pipe(pipeWrite[i-1])<0 || pipe(pipeRead[i-1])<0){	// 8 pipe | 4 for write 4 for read
			errExit("Could not open pipe");
		}

    	pid = fork();
    	if(pid > 0){
    		int flagR = fcntl(pipeRead[i-1][0], F_GETFL, 0);
    		if(fcntl( pipeRead[i-1][0], F_SETFL, flagR |O_NONBLOCK) < 0){	//read pipe cant block
    			errExit("read pipe could not setted as nonblock");
    		}
    		servers[i] = pid;		//record pids
    	}
    	else if(pid == 0){	//child created and exit loop
    		own = i-1; 	//ticket, for process 1 own is 0 etc
    		break;
    	}
    	else{
    		errExit("fork");
    	}
    } 

    if(servers[0] == getpid()){		//main process
    	char buf[50];
    	time_t now = time(0);
    	struct tm now_t = *localtime(&now);
    	strftime (buf, 100, "%d.%m.%Y", &now_t);

    	f = fopen("Banka.log", "w");
    	if(f == NULL){
    		errExit("fopen");
    	}
    	fprintf(f, "%s tarihinde islem başladı. Bankamız %d saniye hizmet verecek.\n",buf, serveTime);	//düzeltilcek
    	fprintf(f, "clientPid   processNo   Para    islem bitis zamanı\n" );
    	fprintf(f, "---------   ---------   ----    ------------------\n" );
    }
 
   	// subservers created banks started
    running();
	// when time over main process exit loop does finishing tasks and kill all children and own
	if(servers[0] == getpid())
		afterSignal(serveTime);
	
	return 0;
}

void running(){
	int index=-1, subServerIndex=-1;
	char clientFifo[CLIENT_FIFO_NAME_LEN];
	struct request req,takeReq;
	struct response resp;
	struct log logInfo;

	while(alarmKey || isBusy(serversBusyOrFree)){                      
		if(servers[0] == getpid()){		// main process only
		    for(int i=0; i<4 ; ++i){								//main process takes log info from children
		        if (read(pipeRead[i][0], &logInfo, sizeof(struct log)) == sizeof(struct log)){		
		            serversBusyOrFree[i] = 0;		// read success means child done
		            if(logInfo.money != -999){
			            serveCount[i] += 1; 	//increase serve count for this process
			            fprintf(f, "%-10d  process%d     %-9d  %.0lf\n", logInfo.client_pid, i+1, logInfo.money, logInfo.second);
			        }
		        }
		    }
		    
		    index = nextOne();
			if(index >= 0 && alarmKey) {
				if (read(serverFd, &req, sizeof(struct request)) == sizeof(struct request)) {	//main process gets request
		            serversBusyOrFree[index] = 1; 	// mark busy
		        }
		        else{
		        	continue;
		        }
				
				if(write(pipeWrite[index][1],&index,sizeof(index)) != sizeof(index)){	//write index to pipe
					errExit("write pipe");
				}	

		        if (write(pipeWrite[index][1], &req, sizeof(struct request)) != sizeof(struct request)){	//main process send req to child
		            errExit("write pipe");
		        }
		        if (write(pipeWrite[index][1], &serveTime, sizeof(serveTime)) != sizeof(serveTime)){	//main process send req to child
		            errExit("write pipe");
		        }
			}
		}
		else {		//subservers
			read(pipeWrite[own][0] , &subServerIndex , sizeof(subServerIndex));	//read from own pipe 	(yazılmadıysa bloklancak)
			
			if(own == subServerIndex){ 	//is it me ?
				read(pipeWrite[own][0] , &takeReq , sizeof(struct request)); 
				read(pipeWrite[own][0] , &serveTime , sizeof(serveTime)); 
				// build client FIFO name and open
		        sprintf(clientFifo, CLIENT_FIFO_TEMPLATE, (long) takeReq.pid);
		        clientFd = open(clientFifo, O_WRONLY);
		        if (clientFd == -1) {        
		        	errExit("open client fifo");
		        }
		        
		        latency(1500);	// wait 1500 ms

		        // Send response and close FIFO 
		        srand(takeReq.pid);	//called with pid to make unique random values


		        logInfo.client_pid = takeReq.pid;

		        struct timeval  tv2;
		        gettimeofday(&tv2, NULL);
		        logInfo.second = (double) (tv2.tv_usec - tv1.tv_usec) / 1000 + (double) (tv2.tv_sec - tv1.tv_sec) * 1000;	// timerdan sonra ayarlancak
		        
		        if((logInfo.second - (double) serveTime * 1000) > 0){		//time over
		        	resp.money = -999;
		        }
		        else{
	        		resp.money = (rand()%100)+1;
	        	}
		        logInfo.money = resp.money;

		        
		        if(write(pipeRead[own][1],&logInfo,sizeof(struct log)) != sizeof(struct log)){	
					errExit("write pipe");
				}	
				if (write(clientFd, &resp, sizeof(struct response)) != sizeof(struct response)){
		            fprintf(stderr, "Error writing to FIFO %s\n", clientFifo);
				}
		        if (close(clientFd) == -1){
		            errExit("close");
		        }
		    }
		}
	}
}


void afterSignal(int serveTime){
	char clientFifo[CLIENT_FIFO_NAME_LEN];
	struct request req;
	struct response resp;
	while(read(serverFd, &req, sizeof(struct request)) == sizeof(struct request)) {	//behindhand client management
        sprintf(clientFifo, CLIENT_FIFO_TEMPLATE, (long) req.pid);
        clientFd = open(clientFifo, O_WRONLY);
        if (clientFd == -1) {           /* Open failed, fail client */
        	continue;
        }

        /* Send response and close FIFO */
        
        resp.money = -999;	//flag
        if (write(clientFd, &resp, sizeof(struct response)) != sizeof(struct response))
            fprintf(stderr, "Error writing to FIFO %s\n", clientFifo);
        if (close(clientFd) == -1)
            errExit("close");
    }
	   
    unlink(SERVER_FIFO);

	fprintf(f, "\n%d saniye dolmustur %d musteriye hizmet verdik.\n",
	serveTime,serveCount[0]+serveCount[1]+serveCount[2]+serveCount[3] );
	fprintf(f, "process1 %d musteriye hizmet sundu\n", serveCount[0]);
	fprintf(f, "process2 %d musteriye hizmet sundu\n", serveCount[1]);
	fprintf(f, "process3 %d musteriye hizmet sundu\n", serveCount[2]);
	fprintf(f, "process4 %d musteriye hizmet sundu ...\n", serveCount[3]);
	fclose(f);
    kill(0,SIGKILL);
}

void alarmHandler(int signo){
	alarmKey = 0;
	if(signo == SIGINT)
        printf("  SIGINT(^C) handled!\n");
        
    else if(signo == SIGTERM)
        printf("  SIGTERM handled!\n");
}

int nextOne() {
	for (int i = 0; i < 4; ++i) 
		if(serversBusyOrFree[i] == 0)
			return i;
	return -1; 	// all servers busy
}

int isBusy(int sv[]){
	for (int i = 0; i < 4; ++i)
		if (sv[i] == 1)
			return 1;
	return 0;
}

void latency(double msec){
	double dif = 0.0;
	struct timeval start, end;

	gettimeofday(&start, NULL);
	while(dif <= msec){
		gettimeofday(&end, NULL);
		dif = (double) (end.tv_usec - start.tv_usec) / 1000 + (double) (end.tv_sec - start.tv_sec)*1000;
	} 
}
