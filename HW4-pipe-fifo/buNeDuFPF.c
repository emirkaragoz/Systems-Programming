#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <errno.h>
#include <time.h>

int flag;   //-z flag
int fifo;
int processCount[2];
char fifoName [] = "/tmp/151044052sizes";
void chandler(int signo);
void zhandler(int signo);
int sizepathfun(char *);  
int postOrderApply(char *, int pathfun (char *));  
void firstCheck(char *, int pathfun (char *));  

int main(int argc, char **argv){
	
    signal(SIGINT,chandler);
	signal(SIGTSTP,zhandler);

    if(argc == 3 && strcmp(argv[1],"-z")==0){
        flag=1;     //flag enable
        firstCheck(argv[2],sizepathfun);
    }
    else if (argc == 3 && strcmp(argv[1],"-z")!=0)
        printf("Invalid usage! It must be '%s -z pathname'\n",argv[0]);
    else if(argc == 2 && strcmp(argv[1],"-z")==0)
        printf("Invalid usage! It must be '%s -z pathname'\n",argv[0]);
    else if(argc == 2){
        flag=0;     //flag disable
        firstCheck(argv[1],sizepathfun);
    }
    else{ 
        printf("Invalid usage! It must be '%s -z pathname' or ",argv[0]);
        printf("'%s pathname'\n",argv[0]);
        return -1;
    }    
    
    unlink("/tmp/151044052sizes");
    
    return 0;
}

void chandler(int signo){
	printf("  SIGINT(^C) handled!\n");
	kill(0,SIGKILL);
}

void zhandler(int signo){
	printf("  SIGTSTP(^Z) handled!\n");
	kill(0,SIGKILL);
}


void firstCheck(char *path, int pathfun (char *)){
    struct stat fileStat;
    int pid,pCount;
    int i=1;
    char buf[1024];
	
    printf("PID       SIZE       PATH\n");

    if(mkfifo(fifoName,0666) < 0){
    	perror("Cannot create fifo");
    	exit(EXIT_FAILURE);
    }


	lstat(path,&fileStat);
	if((fileStat.st_mode & S_IRUSR)){       //permission check 
	    if((S_ISDIR(fileStat.st_mode))) {    //direction check
	    	if(pipe(processCount)<0){
	    		perror("Cannot create pipe");
	    		exit(EXIT_FAILURE);
	    	}
	    	pid = fork();
	    	if (pid == 0){		//child process
	    		write(processCount[1],&i,sizeof(i));

	        	postOrderApply(path,sizepathfun);
	        	_exit(0);
	        }
	        else if(pid >0){    //parent process  
	            
                fifo = open(fifoName,O_RDONLY);
                if(fifo <0){
            		perror("Cannot open fifo");
            		exit(EXIT_FAILURE);
                }

            	wait(NULL);
                while(read(fifo, buf, sizeof(buf)+1)>0);
               
                close(fifo);
            	 
            	close(processCount[1]);
                read(processCount[0], &pCount, sizeof(pCount));
                close(processCount[0]);

	            printf("%d child process created. Main process is %d\n",pCount,getpid());
	        }
	        else{
	            perror("Cannot fork");
	            exit(EXIT_FAILURE);
	        }      
	    }
	    
	    else if(S_ISREG(fileStat.st_mode)){  //regular file check
	        printf("%d      %-11d%s\n", getpid(), pathfun(path)/1024, path);
	        printf("%d child process created. Main process is %d\n",pCount,getpid());
	    }
	    else{                               //special file
	        printf("%-17d    Special file %s\n", getpid(), path);
	        printf("%d child process created. Main process is %d\n",pCount,getpid());
	    }
	}
	else                                
	    printf("Cannot read folder %s \n",path);  
    
    
}

int postOrderApply(char *path, int pathfun (char *)){

    int size=0,tmp,tempSize,fd[2],i;			
    struct dirent *pDirent;
    DIR *pDir;
    struct stat fileStat;
    char name [512];       //new path name
    char buff [600];
    pid_t pid;
  
    pDir = opendir (path);	//open directory
   
    if (pDir != NULL) 
        while ((pDirent = readdir(pDir)) != NULL){	//read all directory
            if(0!=strcmp(pDirent->d_name,".") && 0!=strcmp(pDirent->d_name,"..")){	//ignore . and .. path
                strcpy(name,path);
                strcat(name,"/");
                strcat(name,pDirent->d_name);
                
                lstat(name,&fileStat);
                if((fileStat.st_mode & S_IRUSR)){		//read permission
                    if((S_ISDIR(fileStat.st_mode))){    //is directory
                        
                        if(pipe(fd)<0){
                        	perror("Cannot create pipe");
                        	exit(EXIT_FAILURE);
                        }
                        pid = fork();
                        if(pid == 0){   //child process
                        	closedir (pDir);

                            read(processCount[0], &i, sizeof(i));  	//read process count    
                            ++i;									//increase
                            write(processCount[1],&i,sizeof(i));	//write process count

                            tempSize = postOrderApply(name,pathfun);
                            
                            close(fd[0]);
                            write(fd[1],&tempSize,sizeof(tempSize));
                            close(fd[1]);
                            _exit(0);
                        }
                       
                        else if(pid > 0) {   // current parent process
                          
                            close(fd[1]);
                            read(fd[0], &tempSize, sizeof(tempSize));
                            close(fd[0]);
                            
                            if(flag == 1)	//-z
                            	size += tempSize;
                        }
                        else{               // error
                            perror("Cannot fork");
                            exit(EXIT_FAILURE);
                        }     
                    }
                    else if(S_ISREG(fileStat.st_mode)){      //regular files
                    	tmp = pathfun(name);
                    	if(tmp > 0)
                        	size+=tmp;
                    }
                    else{                            		//special files
                    	fifo = open(fifoName,O_WRONLY | O_APPEND);
                    	if(fifo<0){
                    		perror("Cannot open fifo");
                    		exit(EXIT_FAILURE);
                    	}
                        strcpy(buff,"");
                        sprintf(buff, "%-17d     Special file %s\n",(int)getpid(), pDirent->d_name);
                         if(write(fifo,buff,strlen(buff)+1)>=0){
                    		while(read(fifo, buff, sizeof(buff)+1)>0){};
                			printf("%s",buff );
                        }
                        close(fifo);
						
                    }
                }
                else{
            	    fifo = open(fifoName,O_WRONLY | O_APPEND); 
            	    if(fifo<0){
            	    	perror("Cannot open fifo");
            	    	exit(EXIT_FAILURE);
            	    }
                    strcpy(buff,"");
                    sprintf(buff, "%-17d     Cannot read folder %s\n",(int)getpid(), pDirent->d_name);
                    if(write(fifo,buff,strlen(buff)+1)>=0){
                   		while(read(fifo, buff, sizeof(buff)+1)>0);
                   		printf("%s",buff );
                    }
                    close(fifo);
                    
                }
            }
        }
    else{
    	return -1;
    }

	
    while(wait(NULL)>0);
    
    fifo = open(fifoName,O_WRONLY | O_APPEND );
    if(fifo<0){
    	perror("Cannot open fifo");
    	exit(EXIT_FAILURE);
    }
	
    strcpy(buff,"");
    sprintf(buff, "%d      %-11d%s\n",(int)getpid(),size/1024,path);

    if(write(fifo,buff,strlen(buff)+1)>=0){
    	while(read(fifo, buff, sizeof(buff)+1)>0);
    	printf("%s",buff );
    }

    close(fifo);
    closedir (pDir);
    return size; 
}

int sizepathfun(char * path){
    struct stat fileStat;
    if(!(stat(path,&fileStat) < 0))
        return fileStat.st_size;
    
    return -1;
}


