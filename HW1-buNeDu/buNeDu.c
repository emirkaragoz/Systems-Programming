#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

int flag;
int sizepathfun(char *);
int deepFirstApply(char *, int pathfun (char *));
void firstCheck(char *, int pathfun (char *));

int main(int argc, char **argv)
{
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

void firstCheck(char *path, int pathfun (char *)){
    struct stat fileStat;

    lstat(path,&fileStat);
    if((fileStat.st_mode & S_IRUSR)){       //permission check 
        
        if((S_ISDIR(fileStat.st_mode)))     //direction check
            deepFirstApply(path,sizepathfun);
        
        else if(S_ISREG(fileStat.st_mode))  //regular file check
            printf("%d   %s\n",pathfun(path)/1024,path);
        else                                //special file
            printf("Special file %s\n",path); 
    }

    else                                
        printf("Cannot read folder %s \n",path);    
}

int deepFirstApply(char *path, int pathfun (char *)){
    int size=0;			//total size
    struct dirent *pDirent;
    DIR *pDir;
    struct stat fileStat;
    char *name;        			//new path name

    pDir = opendir (path);	//open directory
   
    if (pDir != NULL) 
        while ((pDirent = readdir(pDir)) != NULL){	//read all directory
            if(0!=strcmp(pDirent->d_name,".") && 0!=strcmp(pDirent->d_name,"..")){	//ignore . and .. path
                name=(char*)malloc( (strlen(path)+strlen(pDirent->d_name)+2)*sizeof(char));
                strcpy(name,path);
                strcat(name,"/");
                strcat(name,pDirent->d_name);
                
                lstat(name,&fileStat);
                if((fileStat.st_mode & S_IRUSR)){		//read permission
                    if((S_ISDIR(fileStat.st_mode))){    //is directory
                        if (flag == 1)  //-z
                            size+=deepFirstApply(name,pathfun);
                        else
                            deepFirstApply(name,pathfun);
                    }
                    else if(S_ISREG(fileStat.st_mode))      //regular files
                        size+=pathfun(name);
                    else                            		//special files
                        printf("Special file %s\n",pDirent->d_name);
                }
                else
                    printf("Cannot read folder %s \n",name );
                free(name);
            }
        }
    else
    	return -1;

    printf("%d   %s\n",size/1024,path);
    closedir (pDir);
    return size; 
}

int sizepathfun(char * path){
    struct stat fileStat;
    if(!(stat(path,&fileStat) < 0))
        return fileStat.st_size;
    
    return -1;
}