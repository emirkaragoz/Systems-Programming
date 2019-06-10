#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>

void lsf();


int main(int argc, char *argv[]){
    if(argc == 1)
	   lsf();
    else{
        printf("Invalid usage! It must be './lsf'\n");
        exit(EXIT_FAILURE);
    }
	return 0;
}


void lsf(){
    struct dirent *pDirent;
    DIR *pDir;
    struct stat fileStat;
    char *name;       //new path name
    char *path = ".";
  
    pDir = opendir (path);	//open directory
   
    if (pDir != NULL) 
        while ((pDirent = readdir(pDir)) != NULL){	//read all directory
            if(0!=strcmp(pDirent->d_name,".") && 0!=strcmp(pDirent->d_name,"..")){	//ignore . and .. path
            	name=(char*)malloc( (strlen(path)+strlen(pDirent->d_name)+2)*sizeof(char));
                strcpy(name,path);
                strcat(name,"/");
                strcat(name,pDirent->d_name);
                
                lstat(name,&fileStat);
                
                if(!(S_ISDIR(fileStat.st_mode))){    //is not directory
                    if(S_ISREG(fileStat.st_mode)) {    
                    	printf("R ");		//regular file
                    }
                    else {                     	
                        printf("S ");		//special files
                    }
   
                	//permissions
                	printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
        	        printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
        	        printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
        	        printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
        	        printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
        	        printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
        	        printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
        	        printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
        	        printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");

        	        printf("  %5d", (int)fileStat.st_size);	//size
                    printf( "  %s\n" , name);
                }

                free(name);  
            }
        }
    else{
    	perror("directory cannot open");
    	exit(EXIT_FAILURE);
    }

    closedir (pDir);
}