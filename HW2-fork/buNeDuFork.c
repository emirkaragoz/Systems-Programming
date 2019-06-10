#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>

int flag;   //-z flag
FILE *fptr; //global file pointer
int sizepathfun(char *);  
int postOrderApply(char *, int pathfun (char *));  
void firstCheck(char *, int pathfun (char *));  
int parser();
int lineNumber();
int min(int,int);
int isInArray(int [],int,int);

int main(int argc, char **argv){
	fptr = fopen("151044052sizes.txt","w");		//clear file
	if(fptr == NULL)
		exit(1);
	fclose(fptr);

    if(argc == 3 && strcmp(argv[1],"-z")==0){
        flag=1;     //flag enable
        firstCheck(argv[2],sizepathfun);
    }
    else if (argc == 3 && strcmp(argv[1],"-z")!=0)
        printf("Invalid usage! It must be './buNeDu -z pathname'");
    else if(argc == 2 && strcmp(argv[1],"-z")==0)
        printf("Invalid usage! It must be './buNeDu -z pathname'");
    else if(argc == 2){
        flag=0;     //flag disable
        firstCheck(argv[1],sizepathfun);
    }
    else{ 
        printf("Invalid usage! It must be './buNeDu -z pathname' or ");
        printf("'./buNeDu pathname'\n");
        return -1;
    }    
    
    return 0;
    
}

int parser(){ 
	int lineNum = lineNumber(),temp=0,j=0,processCount=0;
	char * oneLine = NULL;
	char * token;
	ssize_t read;
	size_t len=0;
    char **path = (char **)malloc(lineNum * sizeof(char *)); 	//path [lineNum][1024]
    for (int i=0; i<lineNum; i++) 
         path[i] = (char *)malloc(1024 * sizeof(char));  

    int pid [lineNum],sizes [lineNum];
   
    fptr = fopen("151044052sizes.txt","r");
    if(fptr == NULL)
    	exit(1);
   
    while ((read=getline(&oneLine, &len, fptr)) != -1) {
        token = strtok(oneLine, ","); 
          
        while (token != NULL) { 
            if(temp == 0){	//pid
            	if(isInArray(pid,atoi(token),j)<0)
            		++processCount;
            	pid[j]=atoi(token);

            }
            else if(temp==1){	//size
            	sizes[j]=atoi(token);
            }
            else{ 			//path
            	strcpy(path[j],token);	//\n var dikkat et
            }
            token = strtok(NULL, ",");
            ++temp; 
        } 
        temp = 0;
        ++j;
    }

    if (oneLine) free(oneLine);

    if(flag == 1){		// -z
	    for (int i = 0; i < lineNum; ++i){
	    	for (int j = 0; j < lineNum; ++j){
	    		if((pid[i]>pid[j]) && strncmp(path[i],path[j],min(strlen(path[i]),strlen(path[j]))-1) == 0)
	    			sizes[j]+=sizes[i];	
	    	}
	    }
	}

	for (int i = 0; i < lineNum; ++i){
		if(sizes[i] < 0)
			printf("%-17d    %s", pid[i], path[i]);	//special files and permission denied files
		else
			printf("%d     %-11d%s", pid[i], sizes[i], path[i]);
	}


    for (int i=0; i<lineNum; i++)
   		free(path[i]);  
   	free(path);

   	fclose(fptr);
    return processCount;
}

int isInArray(int arr [],int v ,int len){
	for (int i = 0; i < len; ++i)
		if(arr[i] == v)
			return 0;
	return -1;
}

int lineNumber(){
	int lines=0;
	char ch;
	fptr = fopen("151044052sizes.txt","r");
	if(fptr == NULL)
    	exit(1);

	while(!feof(fptr)){
	  	ch = fgetc(fptr);
	  	if(ch == '\n') ++lines;
	  
	}
	fclose(fptr);
	return lines;
}

int min(int i,int j){
	if(i<j)
		return i;
	return j;
}

void firstCheck(char *path, int pathfun (char *)){
    struct stat fileStat;
    int pid,pCount=0;

    printf("PID       SIZE       PATH\n");

	lstat(path,&fileStat);
	if((fileStat.st_mode & S_IRUSR)){       //permission check 
	    if((S_ISDIR(fileStat.st_mode))) {    //direction check
	    	pid = fork();
	    	if (pid == 0){		//child process
	        	postOrderApply(path,sizepathfun);
	        	_exit(0);
	        }
	        else if(pid >0){    //parent process  
	            wait(NULL); 
	            pCount = parser();
	            printf("%d child process created. Main process is %d\n",pCount,getpid());
	        }
	        else{
	            perror("fork");
	            exit(EXIT_FAILURE);
	        }      
	    }
	    
	    else if(S_ISREG(fileStat.st_mode)){  //regular file check
	        printf("%d      %-11d%s\n", getpid(), pathfun(path), path);
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
    int size=0,tmp;			//total size
    struct dirent *pDirent;
    DIR *pDir;
    struct stat fileStat;
    char name [512];       //new path name
    pid_t pid;
    struct flock lock = { F_WRLCK, SEEK_SET, 0, 0, 0 };

    memset(&lock,0,sizeof(lock));
    pDir = opendir (path);	//open directory
   	fptr = fopen("151044052sizes.txt","a");
    if (pDir != NULL) 
        while ((pDirent = readdir(pDir)) != NULL){	//read all directory
            if(0!=strcmp(pDirent->d_name,".") && 0!=strcmp(pDirent->d_name,"..")){	//ignore . and .. path
                strcpy(name,path);
                strcat(name,"/");
                strcat(name,pDirent->d_name);
                
                lstat(name,&fileStat);
                if((fileStat.st_mode & S_IRUSR)){		//read permission
                    if((S_ISDIR(fileStat.st_mode))){    //is directory
                        pid = fork();
                        if(pid == 0){   //child process
                        	closedir (pDir);
                        	fclose(fptr);
                            postOrderApply(name,pathfun);
                            _exit(0);
                        }
                        else if(pid > 0) {   // parent process
                            wait(NULL);
                        }
                        else{               // error
                            perror("fork");
                            exit(EXIT_FAILURE);
                        }     
                    }
                    else if(S_ISREG(fileStat.st_mode)){      //regular files
                    	tmp = pathfun(name);
                    	if(tmp > 0)
                        	size+=tmp;
                    }
                    else{                            		//special files
                    	fcntl(fileno(fptr),F_SETLKW,&lock);    //lock file
                    	
                        fprintf(fptr, "%d,-1,Special file %s\n",(int)getpid(), pDirent->d_name);
                        
                        
                        fcntl(fileno(fptr),F_UNLCK,&lock);      //unlock file
                      
                    }
                }
                else{
                	fcntl(fileno(fptr),F_SETLKW,&lock);    //lock file
                	
                    fprintf(fptr, "%d,-1,Cannot read folder %s\n",(int)getpid(),name);
                    
                    fcntl(fileno(fptr),F_UNLCK,&lock);     //unlock file
    
                }
            }
        }
    else{
    	return -1;
    }

	fcntl(fileno(fptr),F_SETLKW,&lock);    //lock file
    
    fprintf(fptr, "%d,%d,%s\n",(int)getpid(),size,path);

    fcntl(fileno(fptr),F_UNLCK,&lock);     //unlock file
    
    closedir (pDir);
    fclose(fptr);
   
    return size; 
}

int sizepathfun(char * path){
    struct stat fileStat;
    if(!(stat(path,&fileStat) < 0))
        return fileStat.st_size;
    
    return -1;
}


