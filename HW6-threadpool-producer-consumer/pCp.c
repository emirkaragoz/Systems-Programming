#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#define RED   "\x1B[1;31m"
#define GRN   "\x1B[1;32m"
#define RESET "\x1B[0m"

static void * producer(void*);
static void * consumer(void*);
char * getSourceName(char*);
void threadErr(int,char*);
void handler(int);

typedef struct {
    int fd1,fd2;
    char filename[512];
} item;

item * buffer;
int bufferCount = 0;
int bufferSize;
int doneFlag = 0;
int statistics [3] = {0,0,0}; // 0 dir , 1 reg , 2 fifo

pthread_mutex_t stdoutm = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bm = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]){
    int err;
    float totalByte=0;
    void * eachByte;
    struct timeval  tv1,tv2;
    struct stat fileStat;
	pthread_t producerThread, *consumerThreads;

    if(argc != 5){
        printf("Invalid usage. It must be %s [consumer number] [buffer size] [source path] [destination path]\n", argv[0] );
        exit(EXIT_FAILURE);
    }
    if(atoi(argv[2]) <= 0 || atoi(argv[1]) <=0){
        printf("Invalid usage. Consumer number and buffer size must be possitive integer.\n");
        exit(EXIT_FAILURE);
    }
    
    signal(SIGINT,handler);

    size_t stacksize;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_getstacksize(&attr, &stacksize);
    pthread_attr_setstacksize(&attr, 5*stacksize);  //producer thread stack size increased

	buffer = (item *)malloc(atoi(argv[2]) * sizeof(item));
	bufferSize= atoi(argv[2]);	

	char** arguments = (char **)malloc(2 * sizeof(char *));
	for (int j=0; j<2; ++j) 
	    arguments[j] = (char *)malloc(512 * sizeof(char)); 
	
	strcpy(arguments[0],argv[3]);
	strcpy(arguments[1],argv[4]);

    strcat(arguments[1],"/");
    char * temp = getSourceName(arguments[0]);
    strcat(arguments[1],temp);
    free(temp);
    if(lstat(arguments[0],&fileStat) == -1){
        mkdir(arguments[1],0666);
    }
    else {
        mkdir(arguments[1],fileStat.st_mode);
    }

    gettimeofday(&tv1, NULL);
	err = pthread_create(&producerThread, &attr, producer, (void *)arguments);
	if(err) threadErr(err,"producer thread creation");
	

	consumerThreads = (pthread_t *)malloc(atoi(argv[1]) * sizeof(pthread_t));
	for (int i = 0; i < atoi(argv[1]); ++i){
		err = pthread_create(&consumerThreads[i], NULL, consumer, NULL);	//with no argument
		if(err) threadErr(err,"consumer thread creation");
	}
    pthread_attr_destroy(&attr);

	err = pthread_join (producerThread, NULL);	//null change with statistic
    if(err) threadErr(err,"producer thread join");

	doneFlag=1;		//producer finished own job
	pthread_cond_broadcast(&full);
	
	for (int j=0; j<2; ++j) 
        free(arguments[j]);
    free(arguments);

    for (int i = 0; i < atoi(argv[1]); ++i){
    	err = pthread_join (consumerThreads[i], &eachByte);	
        if(err) threadErr(err,"consumer thread join");
        totalByte += *(float *) eachByte ;
        free(eachByte);
    }

    gettimeofday(&tv2, NULL);
    
    printf("\n%d directory copied successfully.\n",statistics[0] );
    printf("%d regular file copied successfully.\n",statistics[1] );
    printf("%d FIFO copied successfully.\n",statistics[2] );
    printf("Total %.0f bytes copied.\n",totalByte );
    printf("Time: %.0lf microsecond\n", (double) (tv2.tv_usec - tv1.tv_usec)  + (double) (tv2.tv_sec - tv1.tv_sec) * 1000000);
    free(consumerThreads);
    free(buffer);
	return 0;
}

static void * producer(void* p){
	struct dirent *pDirent;
    DIR *pDir;
    struct stat fileStat;
    int rfd,wfd;
    char readName [512];  
    char writeName [512];   
    char ** params = (char **) p;

    lstat(params[1],&fileStat);
    if(!(S_ISDIR(fileStat.st_mode))){
        doneFlag = 1;   //finish
        pthread_mutex_lock(&stdoutm);
        printf("Invalid destination directory %s is not a directory!\n" ,params[1]);
        pthread_mutex_unlock(&stdoutm);
        return NULL;
    }

	pDir = opendir (params[0]);

    if (pDir != NULL){
        while ((pDirent = readdir(pDir)) != NULL && doneFlag == 0){
            if(0!=strcmp(pDirent->d_name,".") && 0!=strcmp(pDirent->d_name,"..")){

            	strcpy(readName,params[0]);
                strcat(readName,"/");
                strcat(readName,pDirent->d_name);
  
                lstat(readName,&fileStat);
                if((S_ISDIR(fileStat.st_mode))){
                    statistics[0] +=1;  //dir
                	char** passArgv = (char **)malloc(2 * sizeof(char *));
            	    for (int j=0; j<2; ++j){
            	        passArgv[j] = (char *)malloc(512 * sizeof(char)); 	
                    }

                	strcpy(passArgv[0],readName);
                	strcpy(passArgv[1],params[1]);
                	strcat(passArgv[1],"/");
            		strcat(passArgv[1],pDirent->d_name);

                	mkdir(passArgv[1],fileStat.st_mode);

                	producer(passArgv);
                	
                	for (int j=0; j<2; ++j) 
            	        free(passArgv[j]);
            	    free(passArgv);
                }

                else { 
                    if(S_ISREG(fileStat.st_mode)){
            		    rfd = open(readName, O_RDONLY);
                    }
                    else if(S_ISFIFO(fileStat.st_mode)){
                        rfd = open(readName, O_RDONLY | O_NONBLOCK);
                    }
                    else {
                        continue;
                    }
            		if(rfd == -1){
                        pthread_mutex_lock(&stdoutm);
            			printf(RED " %s can not copied !\n" RESET,readName);
                        pthread_mutex_unlock(&stdoutm);
                        continue;
            		}
            		strcpy(writeName,params[1]);
            		strcat(writeName,"/");
            		strcat(writeName,pDirent->d_name);

            		wfd = open(writeName, O_WRONLY | O_TRUNC | O_CREAT , fileStat.st_mode);
            		if(wfd == -1){
            			pthread_mutex_lock(&stdoutm);
                        printf(RED " %s can not copied !\n" RESET, readName);
                        pthread_mutex_unlock(&stdoutm);
                        close(rfd);
                        continue;
            		}
                    else if(wfd > 1000){    // legal bound 1024 but just in case 1000
                        doneFlag = 1;   //finish
                        pthread_mutex_lock(&stdoutm);
                        printf("Reached per-process limit on the number of open file descriptors");
                        pthread_mutex_unlock(&stdoutm);
                        return NULL;
                    }
            		item produced;
            		produced.fd1 = rfd;
            		produced.fd2 = wfd;
            		strcpy(produced.filename,readName);
            
            		pthread_mutex_lock(&bm);
            		while(bufferCount == bufferSize){
        				pthread_cond_wait(&empty,&bm);
            		}
            		buffer[bufferCount] = produced;
            		++bufferCount;
                    pthread_cond_broadcast(&full);
            		pthread_mutex_unlock(&bm);
                }
            }
        }
    }
    else {
        doneFlag = 1;   //finish
        pthread_mutex_lock(&stdoutm);
        printf("Invalid source directory %s is not a directory!\n" ,params[0]);
        pthread_mutex_unlock(&stdoutm);
        return NULL;
    }

    closedir (pDir);
  
    return NULL;
}

static void * consumer(void* p){
    float* byte = (float*) malloc(sizeof(float));
    *byte = 0;
	struct stat fileStat;
	
    while(doneFlag == 0  || bufferCount != 0){
		pthread_mutex_lock(&bm);
		while(doneFlag == 0 && bufferCount == 0){
			
			pthread_cond_wait(&full,&bm);
		}
		if(bufferCount > 0){
			--bufferCount;
			item getProduced = buffer[bufferCount];
            pthread_cond_signal(&empty);
			pthread_mutex_unlock(&bm);

			lstat(getProduced.filename,&fileStat);
			char *content = (char*) malloc((int)fileStat.st_size * sizeof(char));
			if(read(getProduced.fd1 , content, (int)fileStat.st_size) == -1 ||
               write(getProduced.fd2 , content, (int)fileStat.st_size) == -1){
                pthread_mutex_lock(&stdoutm);
                printf(RED " %s can not copied !\n" RESET, getProduced.filename);
                pthread_mutex_unlock(&stdoutm);
            }
            else {
                *byte += (float)fileStat.st_size ;
                pthread_mutex_lock(&stdoutm);
                printf(GRN " %s is copied successfully!\n" RESET, getProduced.filename);
                pthread_mutex_unlock(&stdoutm);
                if(S_ISREG(fileStat.st_mode)) statistics[1] +=1;    //regular
                else statistics[2] +=1;     //fifo
            }

			free(content);

			close(getProduced.fd1);
			close(getProduced.fd2);  
		}
		else{
			pthread_mutex_unlock(&bm);
		}	
	}
	return (void *)byte; 	
}

char * getSourceName(char* sourcePath){
    char * result = (char *)malloc(512 * sizeof(char)); 
    char c;
    int slash = 1;
    for (int i = strlen(sourcePath)-1, j=0 ; i >=0 ; --i){
        if (slash==1 && sourcePath[strlen(sourcePath)-1] == '/'){
            slash =0;
            continue;
        }
        if(sourcePath[i] != '/'){
            result[j] = sourcePath[i];
            ++j;
        }
        else{
            result[j] = '\0';   //null termined
            break;
        }
    }

    for (int i = 0 , j=strlen(result)-1; i < strlen(result)/2; ++i ,--j){
        c=result[j];
        result[j] = result [i];
        result[i]=c;
    }
    return result;
}

void handler(int signo){
    printf("SIGINT(^C) handled.\n");
    doneFlag=1;
}

void threadErr(int err,char* msg){
    errno = err;
    perror(msg);
    exit(EXIT_FAILURE);
}
