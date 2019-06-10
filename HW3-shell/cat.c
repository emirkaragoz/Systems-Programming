#include <unistd.h>
#include <stdio.h>
#include <fcntl.h> 
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

void cat(char *);

int main(int argc, char **argv){
    char *buf;
   
    if(argc == 2)
	   cat(argv[1]);  //check usage
    else if (argc == 1){                               // valdgrid check
        buf = (char*)malloc(16384*sizeof(char));       // 16kb buffer
        if (read(STDIN_FILENO, buf, 16384) == -1){
            perror("read");
            exit(EXIT_FAILURE);
        }
        if(buf[strlen(buf)-1] == '\n')
            buf[strlen(buf)-1] = '\0';  //erase '\n'
        cat(buf);
        free(buf);       
    }
    else{
        printf("Invalid usage! It must be '%s filename'\n", argv[0] );
        printf("With no FILE, read standard input (using with | cat)\n");
        exit(EXIT_FAILURE);
    }

	return 0;
}


void cat(char * name){
    int fd;
    char * buf;
    struct stat fileStat;
    if(lstat(name,&fileStat) == -1){
        printf("Invalid path!");
    }
    
    else if((S_ISDIR(fileStat.st_mode))){    //is directory
        printf("cat: %s: Is a directory\n",name);
    }

    else if(S_ISREG(fileStat.st_mode)){  // is regular file
        fd = open(name, O_RDONLY);
        if(fd<0){
            perror("Could not open file!");
            exit(EXIT_FAILURE);
        }

        buf = (char*)calloc( (fileStat.st_size+1),sizeof(char));
        if(read(fd, buf, fileStat.st_size*sizeof(char))>0)
            printf("%s",buf);

        free(buf);
        close(fd);
    }
    else{                             
        printf("cat: %s: Is a Special File\n",name);  //special files
    }
}