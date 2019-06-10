#include <unistd.h>
#include <stdio.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

void wc(char *);
int lineNumber(char *);
int lineNumberOfString(char *);

int main(int argc, char **argv){
	char *buf;

    if(argc == 2)
       wc(argv[1]);  //check usage
    else if(argc == 1){                               // valdgrid check
        buf = (char*)malloc(16384*sizeof(char));
        if (read(STDIN_FILENO, buf, 16384) == -1){
            perror("read");
            exit(EXIT_FAILURE);
        }
        if(buf[strlen(buf)-1] == '\n')
            buf[strlen(buf)-1] = '\0';  //erase '\n'
        wc(buf);
        free(buf);    
    }
    else{
        printf("Invalid usage! It must be '%s filename'\n", argv[0] );
        printf("With no FILE, read standard input (using with | wc)\n");
        exit(EXIT_FAILURE);
    }

    
    return 0;
}


void wc(char * name){
    struct stat fileStat;
    if(lstat(name,&fileStat) == -1){
        printf("%d\n",lineNumberOfString(name));
    }
    
    else if((S_ISDIR(fileStat.st_mode))){    //is directory
        printf("wc: %s: Is a directory\n",name);
    }

    else if(S_ISREG(fileStat.st_mode)){  // is regular file
        printf("%d %s\n",lineNumber(name),name);
    }
    else{                             
        printf("wc: %s: Is a Special File\n",name);  //special files
    }
}

int lineNumber(char * name){
    int lines=1;
    char ch;
   
    FILE *fptr = fopen(name, "r");
    if(fptr == NULL){
        perror("Could not open file!");
        exit(EXIT_FAILURE);
    }

    while(!feof(fptr)){
        ch = fgetc(fptr);
        if(ch == '\n') ++lines;
    }
    fclose(fptr);
    return lines;
}

int lineNumberOfString(char * str){
    int lines=1;

    for (int i = 0; i < strlen(str); ++i)
        if(str[i] == '\n') ++lines;
    
    return lines;
}